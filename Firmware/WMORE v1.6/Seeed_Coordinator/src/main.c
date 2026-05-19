/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*************************************************************************************
 * WMORE Coordinator firmware (XIAO BLE):
 * 03.11.22
 *
 * - Sends sync packets to OLA via ESB.
 * - Each ESB payload includes:
 *     [SOF] [Command] [t3] [t2] [t1] [t0] [hh] [CHK]
 *   where:
 *     SOF   : start-of-frame marker
 *     Command: TX_CMD_SYNC or TX_CMD_STOP
 *     t3..t0: UNIX time in uint32_t (MSB first)
 *     hh    : hundredths
 *     CHK   : CRC-8 over bytes 0..6
 *
 * Original version was written for OLA + Arduino Nano and later adapted to XIAO BLE.
 *************************************************************************************/

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <irq.h>
#include <logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <zephyr/usb/usb_device.h>
#include <drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

// Define boards
#define XIAO

/* -------------------------------------------------------------------------- */
/*  Compile-time configuration                                                 */
/* -------------------------------------------------------------------------- */

#define TX_PAYLOAD_LEN      8      // Total ESB payload length: [SOF][CMD][5 data][CHK]
#define TX_CMD_SYNC         0      // Sample synchronisation
#define TX_CMD_STOP         1      // Stop sampling
#define SOF                 0xAA   // Start-of-frame marker
#define DATA_LEN            5      // [t3][t2][t1][t0][hh]
#define SAMPLE_INTERVAL_MS  10     // ESB sync period (10 ms)
#define DEBOUNCE            50000  // Button debounce delay in µs

/* -------------------------------------------------------------------------- */
/*  Devicetree node labels (from .dts / .overlay)                             */
/* -------------------------------------------------------------------------- */

#define START_INPUT  DT_NODELABEL(start_in)
#define STOP_INPUT   DT_NODELABEL(stop_in)
#define SYNC_OUTPUT  DT_NODELABEL(sync_out)
#define STOP_OUTPUT  DT_NODELABEL(stop_out)

#if defined(XIAO)
#define LED          DT_NODELABEL(led0)
#endif

#define _RADIO_SHORTS_COMMON                                                \
    (RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |          \
     RADIO_SHORTS_ADDRESS_RSSISTART_Msk |                                   \
     RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

/* -------------------------------------------------------------------------- */
/*  Devicetree-backed GPIO / device handles                                   */
/* -------------------------------------------------------------------------- */

static const struct gpio_dt_spec start_input = GPIO_DT_SPEC_GET(START_INPUT, gpios);
static const struct gpio_dt_spec sync_output = GPIO_DT_SPEC_GET(SYNC_OUTPUT, gpios);
static const struct gpio_dt_spec stop_output = GPIO_DT_SPEC_GET(STOP_OUTPUT, gpios);
static const struct gpio_dt_spec led         = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *uart             = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* -------------------------------------------------------------------------- */
/*  Global variables                                                          */
/* -------------------------------------------------------------------------- */

/* UART RX buffer used to assemble a 5-byte RTC frame from OLA */
static uint8_t uart_rx_buf[DATA_LEN] = {0};

/* Last complete RTC frame received from OLA (used in ESB payload) */
static uint8_t latest_rtc[DATA_LEN] = {0};

/* Count how many bytes have been received into uart_rx_buf */
static volatile size_t rx_count = 0;

/* Periodic timer generating SAMPLE_INTERVAL_MS “ticks” */
struct k_timer periodic_timer;

/* Stop flag: set when STOP command is sent, breaks inner loop */
bool stop_flag = false;

/* Timer flag: set by periodic_timer_cb(), consumed in main loop */
volatile bool timer_event = false;

/* ESB TX payload (initialised with dummy bytes) */
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(
    0,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
);

/* -------------------------------------------------------------------------- */
/*  Helper functions / callbacks                                              */
/* -------------------------------------------------------------------------- */

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

/*
 * ESB event callback.
 * Currently only distinguishes the event types; logging is commented out.
 */
void esb_cb(struct esb_evt const *event)
{
    switch (event->evt_id) {
    case ESB_EVENT_TX_SUCCESS:
        // LOG_DBG("TX SUCCESS EVENT");
        break;
    case ESB_EVENT_TX_FAILED:
        // LOG_DBG("TX FAILED EVENT");
        break;
    case ESB_EVENT_RX_RECEIVED:
        // RX not used in this application (PTX only)
        break;
    default:
        break;
    }
}

/*
 * Timer callback: simply raises timer_event,
 * which is later consumed in the main loop.
 */
void periodic_timer_cb(struct k_timer *dummy)
{
    ARG_UNUSED(dummy);
    timer_event = true;
}

/*
 * UART IRQ callback (interrupt-driven RX).
 *
 * - Continuously reads incoming bytes from UART FIFO.
 * - Fills uart_rx_buf[] until DATA_LEN bytes are received.
 * - On each full frame, copies uart_rx_buf[] into latest_rtc[] and resets rx_count.
 * - No blocking and no disabling of RX; we always keep capturing the latest frame.
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

/* -------------------------------------------------------------------------- */
/*  Initialisation functions                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Initialise high-frequency clock.
 * Required by ESB/radio.
 */
int clocks_start(void)
{
    int err;
    int res;
    struct onoff_manager *clk_mgr;
    struct onoff_client clk_cli;

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

/*
 * Initialise ESB in PTX mode with:
 * - 3-byte addresses
 * - 2 Mbps bitrate
 * - 16-bit CRC
 * - manual TX mode (esb_start_tx() required)
 * - selective auto-ack enabled
 */
int esb_initialize(void)
{
    int err;

    /* Default addresses.
     * These should be unique per device / network.
     */
    uint8_t base_addr_0[2] = {0xE7, 0xE7};
    uint8_t base_addr_1[2] = {0xC2, 0xC2};
    uint8_t addr_prefix[8] = {
        0xE7, 0xC2, 0xC3, 0xC4,
        0xC5, 0xC6, 0xC7, 0xC8
    };

    struct esb_config config = ESB_DEFAULT_CONFIG;

    config.protocol            = ESB_PROTOCOL_ESB_DPL;  // Dynamic payload length
    config.mode                = ESB_MODE_PTX;
    config.event_handler       = esb_cb;
    config.bitrate             = ESB_BITRATE_2MBPS;
    config.crc                 = ESB_CRC_16BIT;
    config.tx_output_power     = ESB_TX_POWER_4DBM;
    config.retransmit_delay    = 600;
    config.retransmit_count    = 0;
    config.tx_mode             = ESB_TXMODE_MANUAL;     // Use esb_start_tx()
    config.payload_length      = TX_PAYLOAD_LEN;
    config.selective_auto_ack  = true;

    err = esb_init(&config);
    if (err) {
        return err;
    }

    /* 3-byte address to reduce transmit time. */
    err = esb_set_address_length(3);
    if (err) {
        return err;
    }

    err = esb_set_base_address_0(base_addr_0);
    if (err) {
        return err;
    }

    err = esb_set_base_address_1(base_addr_1);
    if (err) {
        return err;
    }

    err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
    if (err) {
        return err;
    }

    err = esb_set_rf_channel(0);
    if (err) {
        return err;
    }

    return 0;
}

/*
 * Initialise GPIO pins:
 * - start_input : button (input)
 * - sync_output : SYNC pulse to OLA/loggers (output)
 * - stop_output : STOP pulse (output)
 * - led         : status LED (output)
 */
int pins_init(void)
{
    if (!device_is_ready(sync_output.port)) return -ENODEV;
    if (!device_is_ready(stop_output.port)) return -ENODEV;
    if (!device_is_ready(start_input.port)) return -ENODEV;
    if (!device_is_ready(led.port))         return -ENODEV;

    int r;

    r = gpio_pin_configure_dt(&start_input, GPIO_INPUT);             if (r) return r;
    r = gpio_pin_configure_dt(&sync_output, GPIO_OUTPUT_INACTIVE);   if (r) return r;
    r = gpio_pin_configure_dt(&stop_output, GPIO_OUTPUT_INACTIVE);   if (r) return r;
    r = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);           if (r) return r;

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
	uint8_t i;
    int err;

    err = pins_init();
    if (err) {
        return;
    }

    /* Power-on blink to prove LED mapping */
    for (int j = 0; j < 6; ++j) {
        gpio_pin_set_dt(&led, true);
        k_msleep(150);
        gpio_pin_set_dt(&led, false);
        k_msleep(150);
    }

    err = clocks_start();
    if (err) {
        return;
    }

    err = esb_initialize();
    if (err) {
        // LOG_ERR("ESB initialization failed, err %d", err);
        return;
    }

    err = uart_init();
    if (err) {
        return;
    }

    k_timer_init(&periodic_timer, periodic_timer_cb, NULL);
    k_timer_start(&periodic_timer,
                  K_MSEC(SAMPLE_INTERVAL_MS),
                  K_MSEC(SAMPLE_INTERVAL_MS));

    // LOG_INF("Initialization complete");
    // LOG_INF("Sending test packet");

    tx_payload.noack = true;

    /*
     * Initialise the DATA section [2..6] of tx_payload to zero.
     * This ensures that before the first UART frame arrives,
     * payload contains deterministic values.
     */
    for (i = 0; i < DATA_LEN; i++) {
        tx_payload.data[2 + i] = 0;
    }

    /* Main loop: wait for button press, then periodically send ESB sync packets
     * until STOP is requested, then wait for button release and loop again.
     */
    while (true) {
        stop_flag = false;

        /* Wait for button press (active-low) */
        while (gpio_pin_get_dt(&start_input) == false)
            ;
        k_busy_wait(DEBOUNCE); // Debounce: wait for button to stabilise

        /* Wait for button release */
        while (gpio_pin_get_dt(&start_input) == true)
            ;
        k_busy_wait(DEBOUNCE); // Debounce

        /* Logging / sync phase: run until stop_flag is set */
        while (stop_flag == false) {
            if (timer_event) {
                timer_event = false;

                /* ---- Build payload header ---- */
                tx_payload.data[0] = SOF;

                if (gpio_pin_get_dt(&start_input) == false) {
                    /* Button active-low pressed: keep syncing */
                    tx_payload.data[1] = TX_CMD_SYNC;
                } else {
                    /*
                     * Button released: request STOP once,
                     * assert local STOP output and exit loop.
                     */
                    tx_payload.data[1] = TX_CMD_STOP;
                    gpio_pin_set_dt(&stop_output, true);
                    stop_flag = true;
                }

                /*
                 * ---- Copy latest RTC into bytes 2..6 ----
                 * If no frame has arrived yet, latest_rtc[] is still all zeros.
                 * We never block waiting on UART here.
                 */
                gpio_pin_set_dt(&led, true);
                for (i = 0; i < DATA_LEN; i++) {
                    tx_payload.data[2 + i] = latest_rtc[i];
                }
                gpio_pin_set_dt(&led, false);

                /* ---- Compute CHK over bytes [0..6] and place at [7] ---- */
                tx_payload.data[TX_PAYLOAD_LEN - 1] =
                    crc8_0x07(tx_payload.data, TX_PAYLOAD_LEN - 1);

                /* ---- Transmit over ESB ---- */
                (void)esb_flush_tx();
                gpio_pin_set_dt(&sync_output, true);   // Assert local SYNC while sending
                (void)esb_write_payload(&tx_payload);
                (void)esb_start_tx();
                gpio_pin_set_dt(&sync_output, false);  // De-assert SYNC

            }
        }

        /* After STOP: wait for button release again before restarting */
        k_busy_wait(DEBOUNCE); // Debounce
        while (gpio_pin_get_dt(&start_input) == true);
        k_busy_wait(DEBOUNCE); // Debounce

        /* De-assert local STOP output */
        gpio_pin_set_dt(&stop_output, false);
    }
}
