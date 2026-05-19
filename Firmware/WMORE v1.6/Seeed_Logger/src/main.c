/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*************************************************************************************
 * WMORE Logger firmware (XIAO BLE):
 * 03.11.22
 *
 * - Receives sync packets from the Coordinator over ESB.
 * - Each packet includes:
 *     [SOF] [CMD] [t3] [t2] [t1] [t0] [hh] [CHK]
 * - Valid packets:
 *     * Assert SYNC output
 *     * Forward [t3 t2 t1 t0 hh] to OLA via UART
 *     * If CMD == RX_CMD_STOP, assert STOP output
 *
 * Original version was written for OLA + Arduino Nano and later adapted to XIAO BLE.
 *************************************************************************************/

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/gpio.h>
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

// Define boards
#define XIAO

/* -------------------------------------------------------------------------- */
/*  Compile-time configuration                                                 */
/* -------------------------------------------------------------------------- */

#define RX_PAYLOAD_LEN  8     // ESB payload: [SOF][CMD][t3][t2][t1][t0][hh][CHK]
#define RX_CMD_SYNC     0     // Synchronisation packet
#define RX_CMD_STOP     1     // Stop command
#define SOF             0xAA  // Start-of-frame marker

/* -------------------------------------------------------------------------- */
/*  Devicetree node labels (from .dts / .overlay)                             */
/* -------------------------------------------------------------------------- */

#define SYNC DT_NODELABEL(sync_out)
#define STOP DT_NODELABEL(stop_out)

#if defined(XIAO)
#define LED  DT_NODELABEL(led0)
#endif

/* -------------------------------------------------------------------------- */
/*  Devicetree-backed GPIO / device handles                                   */
/* -------------------------------------------------------------------------- */

static const struct gpio_dt_spec sync = GPIO_DT_SPEC_GET(SYNC, gpios);
static const struct gpio_dt_spec stop = GPIO_DT_SPEC_GET(STOP, gpios);
static const struct gpio_dt_spec led  = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *uart      = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* -------------------------------------------------------------------------- */
/*  Global variables                                                          */
/* -------------------------------------------------------------------------- */

/* ESB RX payload (used to read incoming packets from Coordinator) */
static struct esb_payload rx_payload;

/* TX payload (currently unused; reserved for future streaming features) */
/* Initialised to an arbitrary pattern */
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(
    0, 0x00, 0x01, 0x02, 0x03, 0x04
);

/* Global flag set from ESB callback when a packet is received */
volatile bool received = false;

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
 * handle_esb_payload()
 *
 * Process a single ESB packet:
 *  - Validate length, SOF and CRC.
 *  - If valid:
 *      * Assert SYNC and LED.
 *      * If CMD == RX_CMD_STOP, assert STOP.
 *      * Forward [t3 t2 t1 t0 hh] (bytes 2..6) out over UART.
 *      * De-assert SYNC, STOP and LED.
 */
void handle_esb_payload(const struct esb_payload *p)
{
    /* Basic sanity checks */
    if (p->length != RX_PAYLOAD_LEN) {
        return;
    }
    if (p->data[0] != SOF) {
        return;
    }

    /* CRC check over bytes [0..6], compare with CHK in [7] */
    uint8_t calc = crc8_0x07(p->data, RX_PAYLOAD_LEN - 1);
    if (calc != p->data[RX_PAYLOAD_LEN - 1]) {
        return;
    }

    /* Valid frame → assert SYNC and LED */
    gpio_pin_set_dt(&sync, true);
    gpio_pin_set_dt(&led,  true);

    /* Command handling */
    if (p->data[1] == RX_CMD_STOP) {
        gpio_pin_set_dt(&stop, true);
    }

    /* Forward [t3 t2 t1 t0 hh] = indices 2..6 via UART to OLA */
    for (uint8_t i = 2; i <= 6; i++) {
        uart_poll_out(uart, p->data[i]);
        k_usleep(50); // Simple pacing between bytes
    }

    /* De-assert outputs */
    gpio_pin_set_dt(&sync, false);
    gpio_pin_set_dt(&stop, false);
    gpio_pin_set_dt(&led,  false);
}

/*
 * ESB event callback:
 * - For RX: sets 'received' flag so main loop knows to drain RX FIFO.
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
        /* Signal that at least one packet is ready in the RX FIFO */
        received = true;
        break;
    }
}

/* -------------------------------------------------------------------------- */
/*  Initialisation functions                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Initialise GPIOs:
 *  - sync : SYNC output towards OLA/loggers
 *  - stop : STOP output towards OLA/loggers
 *  - led  : status LED
 */
int pins_init(void)
{
    if (!device_is_ready(sync.port)) return -ENODEV;
    if (!device_is_ready(stop.port)) return -ENODEV;
    if (!device_is_ready(led.port))  return -ENODEV;

    int r;
    r = gpio_pin_configure_dt(&sync, GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&stop, GPIO_OUTPUT_INACTIVE); if (r) return r;
    r = gpio_pin_configure_dt(&led,  GPIO_OUTPUT_INACTIVE); if (r) return r;

    return 0;
}

/*
 * Initialise high-frequency clock.
 * Required by ESB/radio.
 */
int clocks_init(void)
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
 * Initialise ESB in PRX (primary receiver) mode:
 *  - 3-byte addresses
 *  - 2 Mbps bitrate
 *  - 16-bit CRC
 *  - selective auto-ack enabled
 */
int esb_initialise(void)
{
    int err;

    /* Default addresses (should be customised for products). */
    uint8_t base_addr_0[2] = {0xE7, 0xE7};
    uint8_t base_addr_1[2] = {0xC2, 0xC2};
    uint8_t addr_prefix[8] = {
        0xE7, 0xC2, 0xC3, 0xC4,
        0xC5, 0xC6, 0xC7, 0xC8
    };

    /* ESB configuration */
    struct esb_config config = ESB_DEFAULT_CONFIG;

    config.protocol           = ESB_PROTOCOL_ESB_DPL;
    config.bitrate            = ESB_BITRATE_2MBPS;
    config.mode               = ESB_MODE_PRX;
    config.event_handler      = esb_cb;
    config.tx_output_power    = ESB_TX_POWER_4DBM;
    config.selective_auto_ack = true;
    config.crc                = ESB_CRC_16BIT;
    config.retransmit_delay   = 600;
    config.retransmit_count   = 0;
    config.payload_length     = 3;   // Not strictly used for DPL, kept for legacy

    /* Initialise ESB */
    err = esb_init(&config);
    if (err) {
        return err;
    }

    // err = esb_set_address_length(5);
    err = esb_set_address_length(3);     // 3-byte address to match Coordinator
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

    /* Power-on blink to prove LED mapping */
    for (int j = 0; j < 6; ++j) {
        gpio_pin_set_dt(&led, true);
        k_msleep(150);
        gpio_pin_set_dt(&led, false);
        k_msleep(150);
    }

    err = clocks_init();
    if (err) {
        return;
    }

    err = esb_initialise();
    if (err) {
        // LOG_ERR("ESB initialization failed, err %d", err);
        return;
    }

    // LOG_INF("Initialization complete");

    /* Start ESB in RX mode */
    err = esb_start_rx();
    if (err) {
        // LOG_ERR("RX setup failed, err %d", err);
        return;
    }

    /* Main loop: wait for ESB RX events, then drain RX FIFO and handle packets. */
    while (true) {
        if (!received) {
            k_yield();
            continue;
        }
        received = false;

        /* Drain the RX FIFO completely */
        while (esb_read_rx_payload(&rx_payload) == 0) {
            handle_esb_payload(&rx_payload);
        }
    }

    return;
}
