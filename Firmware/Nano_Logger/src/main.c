/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
 /******************************************************************************
 * Nano_Logger
 * 03.11.22
 * Based on ESB samples provided with Nordic nRF Connect SDK.
 * Receive sync packet and forward to OLA via UART.
 * Sync packet includes command byte and 7-byte RTC value.
 ******************************************************************************/

// Includes
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/gpio.h>
#include <irq.h>
#include <logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <drivers/uart.h>

// Target board
#define NANO
//#define DONGLE
//#define XIAO

//LOG_MODULE_REGISTER(esb_prx, CONFIG_ESB_PRX_APP_LOG_LEVEL);

#define RX_PAYLOAD_LEN 8 // Command byte plus 7-byte RTC value. 
#define RX_CMD_SYNC 0
#define RX_CMD_STOP 1

// Nodelabels are defined in .dts and .overlay files
#define SYNC DT_NODELABEL(sync_out)
#define STOP DT_NODELABEL(stop_out)
#if defined(DONGLE)
#define led DT_NODELABEL(led0_green)
#elif defined(NANO)
#define LED DT_NODELABEL(led0)
#endif

// Devicetree data structures
static const struct gpio_dt_spec sync = GPIO_DT_SPEC_GET(SYNC, gpios);
static const struct gpio_dt_spec stop = GPIO_DT_SPEC_GET(STOP, gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

// ESB data structures
static struct esb_payload rx_payload; // ESB received data.
// tx_payload may be used for streaming in a later version.
// Initialise to appropriate length.
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x00, 0x01, 0x02, 0x03, 0x04); 

// Global flag for ISR
volatile bool received = false; // Packet received flag

// Initialise GPIO pins
int pins_init(void)
{
    if (!device_is_ready(sync.port)) return;
    if (!device_is_ready(stop.port)) return;
    if (!device_is_ready(led.port)) return;

    gpio_pin_configure_dt(&sync, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&stop, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    return 0;
}

// ESB callback
void esb_cb(struct esb_evt const *event)
{
	switch (event->evt_id) {
	case ESB_EVENT_TX_SUCCESS:
		//LOG_DBG("TX SUCCESS EVENT");
		break;
	case ESB_EVENT_TX_FAILED:
		//LOG_DBG("TX FAILED EVENT");
		break;
	case ESB_EVENT_RX_RECEIVED:
		if (esb_read_rx_payload(&rx_payload) == 0) { // 0 = normal return	
			gpio_pin_set_dt(&sync, true); // Assert the sync output
            received = true; // Signal that a packet has been received
		} else {
			//LOG_ERR("Error while reading rx packet");
		}
		break;
	}
}

// Initialise clocks
int clocks_init(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		//LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		//LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			//LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err);

	// //LOG_DBG("HF clock started");
	return 0;
}

// Initialise ESB
int esb_initialise(void)
{
	int err;
	/* These are arbitrary default addresses. In end user products
	 * different addresses should be used for each set of devices.
	 */
	//uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
	//uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
	uint8_t base_addr_0[2] = {0xE7, 0xE7};
	uint8_t base_addr_1[2] = {0xC2, 0xC2};
	uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

    // Declare and initialise ESB configuration data structure
	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol = ESB_PROTOCOL_ESB_DPL;
	config.bitrate = ESB_BITRATE_2MBPS;
	config.mode = ESB_MODE_PRX;
	config.event_handler = esb_cb;
    config.tx_output_power = ESB_TX_POWER_4DBM;
	config.selective_auto_ack = true;
    config.crc = ESB_CRC_16BIT;
	config.retransmit_delay = 600;
	config.retransmit_count = 0;
    config.payload_length = 3;

    // Initialise ESB
	err = esb_init(&config);
	if (err) {
		return err;
	}

    //err = esb_set_address_length(5);
    err = esb_set_address_length(3); // 3-byte address
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

void main(void)
{
	int err;
    uint8_t i;

	//LOG_INF("Enhanced ShockBurst prx sample");
	err = pins_init();
	if (err) {
		return;
	}

	err = clocks_init();
	if (err) {
		return;
	}

	err = esb_initialise();
	if (err) {
		//LOG_ERR("ESB initialization failed, err %d", err);
		return;
	}

	//LOG_INF("Initialization complete");

	err = esb_write_payload(&tx_payload);
	if (err) {
		//LOG_ERR("Write payload, err %d", err);
		return;
	}

	//LOG_INF("Setting up for packet receiption");

	err = esb_start_rx();
	if (err) {
		//LOG_ERR("RX setup failed, err %d", err);
		return;
	}

    while (true) {
        if (received == true) {           
            received = false; // De-assert ESB flag
			gpio_pin_set_dt(&led, true); // Turn on the LED
            if (rx_payload.data[0] == RX_CMD_STOP) {
				gpio_pin_set_dt(&stop, true); // Assert the stop output
            }
            for (i = 1; i <= (RX_PAYLOAD_LEN - 1); i++) {
                uart_poll_out(uart, rx_payload.data[i]); // Send RTC to OLA in receive order
                k_usleep(50); // Give the OLA time to keep up
            }
			gpio_pin_set_dt(&sync, false); // De-assert the sync output
			gpio_pin_set_dt(&stop, false); // De-assert the stop output
			gpio_pin_set_dt(&led, false); // Turn off the LED
        }
    }
	/* return to idle thread */
	return;
}
