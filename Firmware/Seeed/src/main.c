/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*************************************************************************************
 * WMORE Seeed XIAO BLE Firmware
 * 14.11.22
 *
 * Overview:
 *   - Any device can operate as either:
 *       • Logger/Coordinator (ST_COORD), or
 *       • Logger only (ST_LOGGER).
 *
 *   - At power-up, the device enters ST_IDLE and waits for:
 *       • a local button press        → becomes Logger/Coordinator (ST_COORD), or
 *       • a valid ESB START_TICK EV   → becomes Logger (ST_LOGGER).
 *
 * Behaviour:
 *   • Logger/Coordinator (ST_COORD):
 *       - Receives RTC timestamps from the OLA via UART (5 bytes: t3 t2 t1 t0 hh).
 *       - Broadcasts sync packets to all Loggers via ESB (CMD_START_TICK).
 *       - Sends periodic “ticks” to its own OLA via the SYNC output.
 *       - Periodically polls individual Loggers (CMD_POLL) to allow them to send
 *         back ACK payloads (e.g., CMD_STOP_REQ).
 *
 *   • Logger (ST_LOGGER):
 *       - Listens for ESB sync packets from the Coordinator (CMD_START_TICK).
 *       - Uses each received packet as a timing “tick” to drive its SYNC output.
 *       - Can request a global STOP by pressing the local button, which queues a
 *         CMD_STOP_REQ ACK payload that will be delivered when the Coordinator polls it.
 *
 *   • Global STOP:
 *       - If the Coordinator’s button is pressed, it broadcasts CMD_STOP, pulses
 *         the STOP output and returns both devices to ST_IDLE.
 *       - If a Logger’s button is pressed, it queues CMD_STOP_REQ; when the
 *         Coordinator polls that Logger and receives CMD_STOP_REQ, it behaves as
 *         if its own button had been pressed (broadcasts CMD_STOP, pulses STOP, etc.).
 *
 * ESB payload structure:
 *     [SOF] [Command] [t3] [t2] [t1] [t0] [hh] [CHK]
 *
 *     SOF     : Start-of-frame marker (0xAA)
 *     Command : CMD_START_TICK, CMD_STOP, CMD_STOP_REQ or CMD_POLL
 *     t3..t0  : UNIX timestamp (uint32_t, MSB first)
 *     hh      : Hundredths of a second
 *     CHK     : CRC-8 computed over bytes 0..6 (polynomial 0x07)
 *
 * Notes:
 *   - The original firmware was written for the OLA + Arduino Nano.
 *   - This version targets the Seeed XIAO BLE (nRF52840).
 *   - Historically, Logger and Coordinator were separate firmware builds; this
 *     version unifies both roles into a single firmware base selected at run-time.
 *************************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/sys/atomic.h>
#include <hal/nrf_ficr.h>
#include <esb.h>
#include <string.h>
#include <irq.h>

// Target board
#define XIAO

/* -------------------------------------------------------------------------- */
/*  Compile-time configuration                                                 */
/* -------------------------------------------------------------------------- */

#define RXTX_LEN            8      // Total ESB payload length: [SOF][CMD][5-byte data][CHK]
#define CMD_START_TICK      0x00   // Start / sync tick: Coordinator → Loggers
#define CMD_STOP            0x01   // Global STOP command: Coordinator → Loggers
#define CMD_STOP_REQ        0x02   // Logger → Coordinator via ACK payload: “please stop”
#define CMD_POLL            0x03   // Coordinator → Logger: request ACK so payload can ride back
#define SOF                 0xAA   // Start-of-frame marker for ESB packets
#define DATA_LEN            5      // Timestamp fields: [t3][t2][t1][t0] + [hh]
#define SAMPLE_INTERVAL_MS  10     // Tick period (ms) used by Coordinator
#define DEBOUNCE_MS         50     // Button debounce delay (ms)
#define POLL_EVERY_N_TICKS  5      // Poll next ESB pipe every N sync ticks

/* -------------------------------------------------------------------------- */
/*  Devicetree-backed GPIO / device handles (from .dts / .overlay)            */
/* -------------------------------------------------------------------------- */

static const struct gpio_dt_spec led_red        = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static const struct gpio_dt_spec led_green      = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);
static const struct gpio_dt_spec led_blue       = GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios);
static const struct gpio_dt_spec start_stop_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(start_in), gpios);
static const struct gpio_dt_spec sync           = GPIO_DT_SPEC_GET(DT_NODELABEL(sync_out), gpios);
static const struct gpio_dt_spec stop           = GPIO_DT_SPEC_GET(DT_NODELABEL(stop_out), gpios);
static const struct device *uart                = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* -------------------------------------------------------------------------- */
/*  Global state and constants                                                */
/* -------------------------------------------------------------------------- */

/* Special 5-byte packet so the OLA can identify the COORDINATOR.
 * Pattern: 0x00 0x00 0x00 0x00 0xFF (hundredths = 0xFF = "I'm the coordinator").
 */
static const uint8_t COORD_SIG[DATA_LEN] = { 0x00, 0x00, 0x00, 0x00, 0xFF };

/* UART RX buffer used to assemble a 5-byte RTC frame from OLA */
static uint8_t uart_rx_buf[DATA_LEN] = {0};

/* Last complete RTC frame received from OLA (used in ESB payload) */
static uint8_t latest_rtc[DATA_LEN] = {0};

/* Count how many bytes have been received into uart_rx_buf */
static volatile size_t rx_count = 0;

/* Periodic timer generating SAMPLE_INTERVAL_MS “ticks” for the Coordinator */
static struct k_timer periodic_timer;

/* ESB RX payload (used to read incoming packets from Coordinator) */
static struct esb_payload rx_p;

/* Keep last time payload from RX so LOGGER can forward via UART (currently disabled) */
static uint8_t g_last_time5[DATA_LEN] = {0};

/* Events bitmask: raised by ISRs / callbacks, consumed in the main loop */
enum {
    EVT_BTN            = BIT(0),   // debounced button press
    EVT_ESB_START_TICK = BIT(1),   // ESB CMD_START_TICK received
    EVT_ESB_STOP       = BIT(2),   // ESB CMD_STOP or CMD_STOP_REQ received
    EVT_TICK           = BIT(3),   // periodic timer tick (Coordinator only)
    EVT_TX_DONE        = BIT(4),   // ESB TX (success or fail) completed
};
static atomic_t g_events;

/* State machine */
typedef enum { ST_IDLE, ST_COORD, ST_LOGGER } state_t;
static volatile state_t g_state = ST_IDLE;

/* Next logger pipe to poll (1..7; pipe 0 is broadcast) */
static uint8_t g_poll_pipe = 1;

/* Tick counter used only in ST_COORD to decide when to poll loggers */
static uint32_t tick_count = 0;

/* Current ESB mode (PRX / PTX), tracked so we can guard TX calls */
static enum esb_mode g_esb_mode = ESB_MODE_PRX;

/* ESB TX payload (initialised with dummy bytes) */
static struct esb_payload tx_p = ESB_CREATE_PAYLOAD(
    0,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
);

/* -------------------------------------------------------------------------- */
/*  Helper functions / forward declarations                                   */
/* -------------------------------------------------------------------------- */

static inline void set_sync(bool on)       { gpio_pin_set_dt(&sync, on); }
static inline void set_stop(bool on)       { gpio_pin_set_dt(&stop, on); }
static inline void set_led_red(bool on)    { gpio_pin_set_dt(&led_red, on); }
static inline void set_led_green(bool on)  { gpio_pin_set_dt(&led_green, on); }
static inline void set_led_blue(bool on)   { gpio_pin_set_dt(&led_blue, on); }

static void btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void btn_debounce_work_handler(struct k_work *work);
static int  esb_switch_mode(enum esb_mode mode);

static struct gpio_callback     btn_gpio_cb;
static struct k_work_delayable  btn_debounce_work;
static bool                     btn_pressed = false;  // debounced logical state

/* -------------------------------------------------------------------------- */
/*  Utility helpers                                                           */
/* -------------------------------------------------------------------------- */

/* Helper to wait for an ESB TX completion event (success or fail) */
static bool wait_tx_done(int timeout_ms)
{
    int64_t dl = k_uptime_get() + timeout_ms;

    for (;;) {
        atomic_val_t ev = atomic_get(&g_events); // read without clearing
        if (ev & EVT_TX_DONE) {
            atomic_and(&g_events, ~EVT_TX_DONE); // clear just that bit
            return true;
        }
        if (k_uptime_get() > dl) {
            return false;
        }
        k_msleep(1);
    }
}

/*
 * CRC-8 computation with polynomial 0x07.
 * Used to compute CHK over ESB bytes [0..6].
 */
static inline uint8_t crc8_0x07(const uint8_t *buf, uint8_t len)
{
    uint8_t crc = 0x00;

    while (len--) {
        crc ^= *buf++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
        }
    }
    return crc;
}

/* Read the physical pin (ACTIVE_LOW button -> 0 means pressed) */
static inline bool raw_button_is_pressed(void)
{
    return gpio_pin_get_dt(&start_stop_btn) == 0;
}

/* Derive ESB pipe number from the chip’s unique ID (1..7). Pipe 0 is reserved for broadcast. */
static uint8_t my_pipe(void)
{
    uint32_t id = NRF_FICR->DEVICEID[0] ^ NRF_FICR->DEVICEID[1];
    uint8_t p   = (id % 7) + 1;  // pipes 1..7
    return p;
}

/* Queue a CMD_STOP_REQ ACK payload on the logger (PRX side) */
static int prx_queue_ack_stop_req(void)
{
    struct esb_payload a = {0};

    a.noack  = false;               // must be acked
    a.pipe   = my_pipe();           // only this pipe will carry our ACK
    a.length = RXTX_LEN;            // full 8-byte frame

    a.data[0] = SOF;
    a.data[1] = CMD_STOP_REQ;

    // For now, no extra data → zero-fill D0..D4
    memset(&a.data[2], 0, DATA_LEN);

    // CRC over bytes [0..6]
    a.data[RXTX_LEN - 1] = crc8_0x07(a.data, RXTX_LEN - 1);

    return esb_write_payload(&a);   // queues the ACK payload in PRX
}

/* PTX-side helper: unicast CMD_POLL to a specific logger pipe */
static int ptx_poll_pipe(uint8_t pipe)
{
    struct esb_payload p = {0};

    p.noack  = false;                 // request ACK so STOP_REQ can ride back
    p.pipe   = pipe;                  // unicast to specific logger
    p.length = RXTX_LEN;              // full 8-byte frame

    p.data[0] = SOF;
    p.data[1] = CMD_POLL;

    // No data needed → zero-fill D0..D4
    memset(&p.data[2], 0, DATA_LEN);

    // CRC
    p.data[RXTX_LEN - 1] = crc8_0x07(p.data, RXTX_LEN - 1);

    int err = esb_write_payload(&p);
    if (err) {
        return err;
    }

    err = esb_start_tx();         // PTX only
    return err;
}

/*
 * ESB send frame (CMD_START_TICK / CMD_STOP).
 * If with_time == true, fills t3..t0,hh from latest_rtc[]; otherwise sends zeros.
 */
static int esb_send_cmd(uint8_t cmd, bool with_time)
{
    if (g_esb_mode != ESB_MODE_PTX) {
        return -EPERM;
    }

    /* Defensive: ensure TX FIFO is empty before queuing a new broadcast frame.
     * We are not using TX queuing semantics for this protocol, so it's safe
     * (and safer) to flush on each send.
     */
    (void)esb_flush_tx();

    tx_p.noack  = true;     // broadcast (no ACK requested)
    tx_p.pipe   = 0;        // broadcast pipe
    tx_p.length = RXTX_LEN;

    tx_p.data[0] = SOF;
    tx_p.data[1] = cmd;

    if (with_time) {
        for (int i = 0; i < DATA_LEN; i++) {
            tx_p.data[2 + i] = latest_rtc[i];
        }
    } else {
        memset(&tx_p.data[2], 0, DATA_LEN);
    }

    /* Compute CRC over bytes [0..6] and place CHK at [7] */
    tx_p.data[RXTX_LEN - 1] = crc8_0x07(tx_p.data, RXTX_LEN - 1);

    int err = esb_write_payload(&tx_p);
    if (err) {
        /* If TX FIFO is full or ESB is busy, report the error so
         * the caller / main loop can decide how to recover.
         */
        return err;
    }

    err = esb_start_tx();
    return err;
}

/* -------------------------------------------------------------------------- */
/*  Callbacks: ESB, timer, UART, button                                       */
/* -------------------------------------------------------------------------- */

/*
 * ESB event callback.
 * - Raises EVT_TX_DONE for both TX success and TX failure.
 * - Parses RX payloads and raises EVT_ESB_START_TICK / EVT_ESB_STOP accordingly.
 *
 * NOTE: Even if part of the logic is currently minimal, this callback is
 *       required so the ESB driver can notify the application and we can
 *       drive the event-based state machine.
 */
static void esb_cb(const struct esb_evt *evt)
{
    /* TX completion (success or failure) */
    if (evt->evt_id == ESB_EVENT_TX_SUCCESS) {
        atomic_or(&g_events, EVT_TX_DONE);
        // May also have RX payloads queued, so don't return yet.
    }
    
    if (evt->evt_id != ESB_EVENT_RX_RECEIVED) {
        return;
    }

    /* Process all queued RX payloads (including ACK payloads) */
    while (esb_read_rx_payload(&rx_p) == 0) {

        if (rx_p.length != RXTX_LEN) {
            continue;                   // we only accept full frames
        }
        if (rx_p.data[0] != SOF) {
            continue;                   // bad SOF
        }

        // Validate CRC: bytes [0..6] vs [7]
        uint8_t calc = crc8_0x07(rx_p.data, RXTX_LEN - 1);
        if (calc != rx_p.data[RXTX_LEN - 1]) {
            continue;                   // drop corrupted frame
        }

        uint8_t cmd = rx_p.data[1];

        switch (cmd) {
        case CMD_START_TICK:
            // Cache the time field if present
            for (int i = 0; i < DATA_LEN; i++) {
                g_last_time5[i] = rx_p.data[2 + i];
            }
            atomic_or(&g_events, EVT_ESB_START_TICK);
            break;

        case CMD_STOP:
            // Coordinator broadcasted STOP
            atomic_or(&g_events, EVT_ESB_STOP);
            break;

        case CMD_STOP_REQ:
            // Logger requested STOP via ACK payload
            atomic_or(&g_events, EVT_ESB_STOP);
            break;

        case CMD_POLL:
            // Coordinator is just polling; logger doesn't need to do anything here.
            // ACK payload (if queued) is handled by the ESB hardware.
            break;

        default:
            // Unknown command → ignore
            break;
        }
    }
}

/*
 * Timer callback: raises EVT_TICK for the Coordinator.
 * The main loop consumes EVT_TICK inside ST_COORD to send START_TICK and poll loggers.
 */
static void periodic_timer_cb(struct k_timer *dummy)
{
    ARG_UNUSED(dummy);
    atomic_or(&g_events, EVT_TICK);
}

/*
 * UART IRQ callback (interrupt-driven RX).
 *
 * - Continuously reads incoming bytes from UART FIFO.
 * - Fills uart_rx_buf[] until DATA_LEN bytes are received.
 * - On each full frame, copies uart_rx_buf[] into latest_rtc[] and resets rx_count.
 * - Non-blocking: we always keep capturing the latest frame and never stall the main loop.
 */
static void uart_irq_cb(const struct device *dev, void *user_data)
{
    uint8_t ch;

    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        while (uart_fifo_read(dev, &ch, 1) == 1) {
            /* Accumulate into uart_rx_buf */
            uart_rx_buf[rx_count++] = ch;

            if (rx_count >= DATA_LEN) {
                /* We have a full [t3 t2 t1 t0 hh] frame from OLA */
                for (size_t i = 0; i < DATA_LEN; ++i) {
                    latest_rtc[i] = uart_rx_buf[i];
                }
                /* Reset count for next frame */
                rx_count = 0;
            }
        }
    }
}

/* Button ISR: schedule debounce work after DEBOUNCE_MS of stable input */
static void btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    k_work_reschedule(&btn_debounce_work, K_MSEC(DEBOUNCE_MS));
}

/*
 * Debounce worker: runs after the line has been stable for DEBOUNCE_MS.
 * Converts raw pin transitions into a single EVT_BTN on the press edge.
 */
static void btn_debounce_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    bool pressed = raw_button_is_pressed();

    if (pressed && !btn_pressed) {
        /* Transition: idle -> pressed (this matches the "wait for press" moment) */
        btn_pressed = true;
        atomic_or(&g_events, EVT_BTN);
    } else if (!pressed && btn_pressed) {
        /* Transition: pressed -> released (no specific event generated here) */
        btn_pressed = false;
    }
}

/* -------------------------------------------------------------------------- */
/*  State entry actions                                                       */
/* -------------------------------------------------------------------------- */

static void enter_idle(void)
{
    /* IDLE = red LED solid */
    set_led_red(true);
    set_led_green(false);
    set_led_blue(false);

    g_state = ST_IDLE;
    set_sync(false);
    set_stop(false);

    /* Ensure we’re listening for ESB packets as PRX */
    (void)esb_switch_mode(ESB_MODE_PRX);
}

static void enter_coord(void)
{
    /* Coordinator = green LED solid */
    set_led_red(false);
    set_led_green(true);
    set_led_blue(false);

    g_state = ST_COORD;

    /* Become PTX */
    if (esb_switch_mode(ESB_MODE_PTX)) {
        enter_idle();
        return;
    }

    /* Initial START_TICK with no time (zeros) to signal start of logging */
    (void)esb_send_cmd(CMD_START_TICK, false);

    /* Local SYNC pulse */
    set_sync(true);
    k_busy_wait(50);
    set_sync(false);

    /* Start periodic ticks (subsequent START_TICK + polling are driven by EVT_TICK) */
    k_timer_start(&periodic_timer,
                  K_MSEC(SAMPLE_INTERVAL_MS),
                  K_MSEC(SAMPLE_INTERVAL_MS));
}

static void enter_logger(void)
{
    /* Logger = blue LED solid */
    set_led_red(false);
    set_led_green(false);
    set_led_blue(true);

    g_state = ST_LOGGER;

    /* Local SYNC pulse to start logging on the attached OLA */
    set_sync(true);
    k_busy_wait(50);
    set_sync(false);
}

/* -------------------------------------------------------------------------- */
/*  Initialisation functions                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Initialise all GPIO pins:
 *
 * Outputs:
 *   - led_red, led_green, led_blue : RGB status LEDs (inactive at boot)
 *   - sync                         : SYNC pulse output to OLA / loggers
 *   - stop                         : STOP pulse output
 *
 * Input:
 *   - start_stop_btn               : Start/Stop button with edge-triggered interrupt
 *
 * Additional setup:
 *   - Configures GPIO interrupt for both edges (press + release)
 *   - Installs ISR callback (btn_cb)
 *   - Sets up delayed work for software debounce
 *   - Initialises the debounced button state based on current pin level
 */
int pins_init(void)
{
    int r;

    /* --- OUTPUTS --- */
    if (!device_is_ready(led_red.port))   return -ENODEV;
    if (!device_is_ready(led_green.port)) return -ENODEV;
    if (!device_is_ready(led_blue.port))  return -ENODEV;
    if (!device_is_ready(sync.port))      return -ENODEV;
    if (!device_is_ready(stop.port))      return -ENODEV;

    r = gpio_pin_configure_dt(&led_red,  GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&led_green,GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&sync,     GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&stop,     GPIO_OUTPUT_INACTIVE); if (r) return r;

    /* --- INPUT (Button) --- */
    if (!device_is_ready(start_stop_btn.port)) return -ENODEV;

    r = gpio_pin_configure_dt(&start_stop_btn, GPIO_INPUT);
    if (r) return r;

    /* Configure interrupt on both edges (press + release) */
    r = gpio_pin_interrupt_configure_dt(&start_stop_btn, GPIO_INT_EDGE_BOTH);
    if (r) return r;

    /* Attach ISR */
    gpio_init_callback(&btn_gpio_cb, btn_cb, BIT(start_stop_btn.pin));
    gpio_add_callback(start_stop_btn.port, &btn_gpio_cb);

    /* Initialise debounce work item */
    k_work_init_delayable(&btn_debounce_work, btn_debounce_work_handler);

    /* Initialise debounced state to the current level */
    btn_pressed = raw_button_is_pressed();

    return 0;
}

/*
 * Initialise high-frequency clock.
 * Required by ESB / radio.
 */
int clocks_init(void)
{
    int err;
    int res;
    struct onoff_manager *clk_mgr;
    struct onoff_client  clk_cli;

    clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
    if (!clk_mgr) {
        // LOG_ERR("Unable to get the Clock manager");
        return -ENXIO;
    }

    sys_notify_init_spinwait(&clk_cli.notify);

    err = onoff_request(clk_mgr, &clk_cli);
    if (err < 0) {
        // LOG_ERR("Clock request failed: %d", err);
        return err;
    }

    do {
        err = sys_notify_fetch_result(&clk_cli.notify, &res);
        if (!err && res) {
            // LOG_ERR("Clock could not be started: %d", res);
            return res;
        }
    } while (err);

    // LOG_DBG("HF clock started");
    return 0;
}

/* ESB runtime mode switch helpers (PRX <-> PTX), no higher-level ACK logic here */
static void esb_fill_common(struct esb_config *cfg)
{
    *cfg = (struct esb_config)ESB_DEFAULT_CONFIG;  // cast makes it an expression

    cfg->protocol           = ESB_PROTOCOL_ESB_DPL;
    cfg->bitrate            = ESB_BITRATE_2MBPS;
    cfg->event_handler      = esb_cb;
    cfg->tx_output_power    = ESB_TX_POWER_4DBM;
    cfg->selective_auto_ack = false;      // ACK payloads still used via noack=false
    cfg->crc                = ESB_CRC_16BIT;
    cfg->retransmit_delay   = 600;
    cfg->retransmit_count   = 0;
    cfg->payload_length     = 3;          // ignored with DPL
}

static int esb_apply_addresses(void)
{
    int err;
    /* Use the same address scheme as earlier versions */
    uint8_t base0[2] = {0xE7, 0xE7};
    uint8_t base1[2] = {0xC2, 0xC2};
    uint8_t pref[8]  = {0xE7,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8};

    if ((err = esb_set_address_length(3)))       return err;
    if ((err = esb_set_base_address_0(base0)))   return err;
    if ((err = esb_set_base_address_1(base1)))   return err;
    if ((err = esb_set_prefixes(pref, ARRAY_SIZE(pref)))) return err;
    if ((err = esb_set_rf_channel(0)))           return err;

    return 0;
}

/*
 * Switch ESB mode at runtime (PRX <-> PTX).
 *
 * - Stops RX and flushes RX/TX FIFOs.
 * - Applies a common ESB configuration and addresses.
 * - Starts RX automatically when entering PRX mode.
 */
static int esb_switch_mode(enum esb_mode mode)
{
    int err;
    struct esb_config cfg;

    (void)esb_stop_rx();
    (void)esb_flush_rx();
    (void)esb_flush_tx();

    esb_fill_common(&cfg);
    cfg.mode = mode;

    if ((err = esb_init(&cfg)))         return err;
    if ((err = esb_apply_addresses()))  return err;

    g_esb_mode = mode;

    if (mode == ESB_MODE_PRX) {
        if ((err = esb_start_rx()))     return err;
    }
    return 0;
}

/*
 * Initialise UART in interrupt-driven mode:
 * - Sets ISR callback.
 * - Enables RX interrupts so uart_irq_cb() is called on incoming data.
 */
int uart_init(void)
{
    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

    uart_irq_callback_set(uart, uart_irq_cb);
    uart_irq_rx_enable(uart);
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  Main application loop                                                     */
/* -------------------------------------------------------------------------- */

void main(void)
{
    int err;

    err = pins_init();
    if (err) {
        return;
    }

    /* Power-on blink (green) to prove LED mapping and that the firmware is running */
    for (int j = 0; j < 6; ++j) {
        set_led_green(true);
        k_msleep(150);
        set_led_green(false);
        k_msleep(150);
    }

    err = clocks_init();
    if (err) {
        return;
    }

    /* ESB is configured lazily inside enter_idle() / enter_coord() via esb_switch_mode() */

    err = uart_init();
    if (err) {
        return;
    }

    /* Timer init (used only while in ST_COORD) */
    k_timer_init(&periodic_timer, periodic_timer_cb, NULL);

    /* Start in IDLE, listening as PRX */
    enter_idle();

    for (;;) {
        /* Atomically fetch and clear pending events for this loop iteration */
        const uint32_t ev = atomic_set(&g_events, 0);

        switch (g_state) {

        case ST_IDLE:
            if (ev & EVT_BTN) {
                /* This device pressed the button first → become Coordinator (PTX) */
                enter_coord();
                break;
            }
            if (ev & EVT_ESB_START_TICK) {
                /* Someone else already started → become Logger (stay PRX) */
                enter_logger();
                break;
            }
            /* No events of interest: short sleep to avoid busy-waiting */
            k_sleep(K_MSEC(1));
            break;

        case ST_COORD:
            if (ev & (EVT_BTN | EVT_ESB_STOP)) {
                /* Local button: broadcast STOP, pulse STOP line, go back to IDLE */
                (void)esb_send_cmd(CMD_STOP, false);
                (void)wait_tx_done(10);
                /* Extra redundancy to make sure everyone heard */
                (void)esb_send_cmd(CMD_STOP, false);
                (void)wait_tx_done(10);

                set_stop(true);
                k_busy_wait(50);
                set_stop(false);

                k_timer_stop(&periodic_timer);
                enter_idle();
                break;
            }
            if (ev & EVT_TICK) {
                /* Periodic tick:
                 *  - broadcast a START_TICK (currently without time),
                 *  - pulse SYNC to its own OLA,
                 *  - periodically poll loggers to collect ACK payloads.
                 */
                (void)esb_send_cmd(CMD_START_TICK, true);

                set_sync(true);
                k_busy_wait(50);
                set_sync(false);

                /* Sends the coordinator signature */
                for (int i = 0; i < DATA_LEN; ++i) {
                    uart_poll_out(uart, COORD_SIG[i]);
                    k_busy_wait(50);
                }

                if ((++tick_count % POLL_EVERY_N_TICKS) == 0) {
                    (void)ptx_poll_pipe(g_poll_pipe);
                    g_poll_pipe = (g_poll_pipe >= 7) ? 1 : (g_poll_pipe + 1);
                }
                break;
            }
            break;

        case ST_LOGGER:
            if (ev & EVT_BTN) {
                /* Logger button: queue STOP_REQ ACK payload.
                 * Coordinator will act on it when it next polls this pipe.
                 */
                (void)prx_queue_ack_stop_req();
                break;
            }
            if (ev & EVT_ESB_STOP) {
                /* Coordinator (or another Logger via Coordinator) requested global STOP */
                set_stop(true);
                k_busy_wait(50);
                set_stop(false);
                enter_idle();
                break;
            }
            if (ev & EVT_ESB_START_TICK) {
                /* Coordinator sent a START_TICK: generate a local SYNC pulse.
                 * Also forward the cached global time to UART.
                 */

                for (int i = 0; i < DATA_LEN; ++i) {
                  uart_poll_out(uart, g_last_time5[i]);
                  k_busy_wait(50);
                }

                set_sync(true);
                k_busy_wait(50);
                set_sync(false);
                break;
            }
            /* Keep listening in PRX mode */
            k_sleep(K_MSEC(1));
            break;
        }
    }
}
