 /*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/******************************************************************************
 * sync_tx_rtc
 * 02.11.22
 * Send sync packets to OLA.
 * Includes command byte and seven byte RTC value.
 * Written for WMORE Coordinator based on OLA + Arduino Nano. 
 ******************************************************************************/
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/gpio.h>
#include <irq.h>
#include <logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <device.h>
#include <devicetree.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <drivers/uart.h>

//LOG_MODULE_REGISTER(esb_ptx, CONFIG_ESB_PTX_APP_LOG_LEVEL);

#define TX_PAYLOAD_LEN 8 // Command byte plus seven RTC bytes 
#define TX_CMD_SYNC 0 // Sample synchronisation
#define TX_CMD_STOP 1 // Stop sampling
#define SAMPLE_INTERVAL_MS 10
#define RTC_FIRST_BYTE 1 // First RTC byte index in tx_payload.data[]
#define RTC_LAST_BYTE 7 // Last RTC byte index in tx_payload.data[]
#define DEBOUNCE 50000 // Button debounce delay in us

// Define supported boards
#define NANO

// Nodelabels are defined in .dts and .overlay files
#define START_INPUT DT_NODELABEL(start_in)
#define STOP_INPUT DT_NODELABEL(stop_in)
#define SYNC_OUTPUT DT_NODELABEL(sync_out)
#define STOP_OUTPUT DT_NODELABEL(stop_out)
#if defined(NANO)
#define LED0 DT_NODELABEL(led0)
#endif

#define _RADIO_SHORTS_COMMON                                               \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |         \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk |                                  \
	 RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

// Devicetree data structures
static const struct gpio_dt_spec start_input = GPIO_DT_SPEC_GET(START_INPUT, gpios);
static const struct gpio_dt_spec stop_input = GPIO_DT_SPEC_GET(STOP_INPUT, gpios); // Not used ATM
static const struct gpio_dt_spec sync_output = GPIO_DT_SPEC_GET(SYNC_OUTPUT, gpios);
static const struct gpio_dt_spec stop_output = GPIO_DT_SPEC_GET(STOP_OUTPUT, gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0, gpios);
static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

// Variables ------------------------------------------------------------------

struct k_timer periodic_timer;
bool stop_flag = false; // Stop input asserted.
volatile bool timer_event = false; // Timer flag.
volatile bool uart_event = false; // UART has received 7 bytes
//static struct esb_payload rx_payload;
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
static uint8_t uart_rx_buf[7] = {0};

// Callbacks ------------------------------------------------------------------

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
	    //
		break;
	}
}

// UART callback
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		// do something
		break;
	case UART_TX_ABORTED:
		// do something
		break;
	case UART_RX_RDY:
		// Should trigger when uart_rx_buf is full
		uart_event = true;
		break;
	case UART_RX_BUF_REQUEST:
		// do something
		break;
	case UART_RX_BUF_RELEASED:
		// do something
		break;
	case UART_RX_DISABLED:
	    // Must re-enable after buffer full
		// 22.12.22 moved this to main loop due to serial sync problem
	    //uart_rx_enable(uart, uart_rx_buf, sizeof uart_rx_buf, SYS_FOREVER_US);
		break;
	case UART_RX_STOPPED:
		// do something
		break;
	default:
		break;
	}
}

// Timer callback
void periodic_timer_cb(struct k_timer *dummy)
{
    timer_event = true;
}

// Functions ------------------------------------------------------------------

// Initialise clocks
int clocks_start(void)
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

	//LOG_DBG("HF clock started");
	return 0;
}

// Initialise ESB
int esb_initialize(void)
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

	struct esb_config config = ESB_DEFAULT_CONFIG;

	//config.protocol = ESB_PROTOCOL_ESB; 
	config.protocol = ESB_PROTOCOL_ESB_DPL; // ESB Dynamic Packet Length
	config.mode = ESB_MODE_PTX;
	config.event_handler = esb_cb;
	//config.bitrate = ESB_BITRATE_1MBPS; 
	config.bitrate = ESB_BITRATE_2MBPS;
    //config.crc = ESB_CRC_OFF; 
	config.crc = ESB_CRC_16BIT;
    config.tx_output_power = ESB_TX_POWER_4DBM;
	config.retransmit_delay = 600;
	config.retransmit_count = 0;
    config.tx_mode = ESB_TXMODE_MANUAL; // packet not sent until esb_start_tx is called
    //config.tx_mode = ESB_TXMODE_AUTO; 
    config.payload_length = TX_PAYLOAD_LEN; 
	//config.selective_auto_ack = false; // disables selective ACK
	config.selective_auto_ack = true; // enables selective ACK
	//config.payload_length = TX_PAYLOAD_LEN;

	err = esb_init(&config);
	if (err) {
		return err;
	}

	err = esb_set_address_length(3); // 3 byte address to reduce transmit time
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

// Initialise GPIO pins
int pins_init(void)
{
    if (!device_is_ready(start_input.port)) return;
    if (!device_is_ready(stop_input.port)) return; // Not used ATM
    if (!device_is_ready(sync_output.port)) return;
    if (!device_is_ready(stop_output.port)) return;
    if (!device_is_ready(led.port)) return;

    gpio_pin_configure_dt(&start_input, GPIO_INPUT);
    gpio_pin_configure_dt(&stop_input, GPIO_INPUT); // Not used ATM
    gpio_pin_configure_dt(&sync_output, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&stop_output, GPIO_OUTPUT_INACTIVE);	
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    return 0;
}

// Initialise UART
int uart_init(void)
{
	int err = 0;

    uart_callback_set(uart, uart_cb, NULL);
	uart_rx_enable(uart, uart_rx_buf, sizeof uart_rx_buf, SYS_FOREVER_US);

	return 0;
}

// Main -----------------------------------------------------------------------

void main(void)
{
	int err;
    uint8_t i; // Counter for Tx payload bytes

	err = clocks_start();
	if (err) {
		return;
	}

	err = pins_init();
	if (err) {
		return;
	}

	err = esb_initialize();
	if (err) {
		//LOG_ERR("ESB initialization failed, err %d", err);
		return;
	}

	err = uart_init(); 
	if (err) {
		return;
	}	

    k_timer_init(&periodic_timer, periodic_timer_cb, NULL);
    k_timer_start(&periodic_timer, K_MSEC(SAMPLE_INTERVAL_MS), K_MSEC(SAMPLE_INTERVAL_MS));

	//LOG_INF("Initialization complete");
	//LOG_INF("Sending test packet");

	tx_payload.noack = true;

    // Initialise tx_payload.data[]
	for (i = RTC_FIRST_BYTE; i <= RTC_LAST_BYTE; i++) {
		tx_payload.data[i] = 0;
	}

	while (true) {
	    stop_flag = false;
	//while (stop_flag == false) 
	//{
        while (gpio_pin_get_dt(&start_input) == false); // Wait for button press
		k_busy_wait(DEBOUNCE); // Wait for button to stabilise
        while (gpio_pin_get_dt(&start_input) == true); // Wait for button release
		k_busy_wait(DEBOUNCE); // Wait for button to stabilise		
        while (stop_flag == false) {
	        if (timer_event) {
	            timer_event = false;														
	            esb_flush_tx();                      
                gpio_pin_set_dt(&sync_output, true); // Assert local sync
				// 22.12.22 uart_rx_enable moved here from UART callback due to serial sync problem
				uart_rx_enable(uart, uart_rx_buf, sizeof uart_rx_buf, SYS_FOREVER_US);
                if (gpio_pin_get_dt(&start_input) == false) { // select appropriate Tx command
                    tx_payload.data[0] = TX_CMD_SYNC; // Assert remote sync
                } else {
                    tx_payload.data[0] = TX_CMD_STOP; // Stop remote logging
					gpio_pin_set_dt(&stop_output, true); // Assert local stop
                    stop_flag = true; // Last transmission before end of logging
                }
	            err = esb_write_payload(&tx_payload);
                if (err) {
	                //LOG_ERR("esb_write_payload() failed, err %d", err);
                }
                err = esb_start_tx(); // required for manual tx mode
                gpio_pin_set_dt(&sync_output, false); // De-assert sync when transmission complete
                if (err) {
		            //LOG_ERR("esb_tx_start() failed, err %d", err);
                }
				while (uart_event != true); // Wait for UART to receive 7 bytes
				uart_event = false;
                gpio_pin_set_dt(&led, true); // Turn on LED
				// Read the RTC value from the OLA via the UART
			    for (i = RTC_FIRST_BYTE; i <= RTC_LAST_BYTE; i++) {
				    tx_payload.data[i] = uart_rx_buf[i - 1];
			    }
				gpio_pin_set_dt(&led, false); // Turn off LED
            }
	    }
		k_busy_wait(DEBOUNCE); // Wait for button to stabilise
        while (gpio_pin_get_dt(&start_input) == true); // Wait for button release
		k_busy_wait(DEBOUNCE); // Wait for button to stabilise			
		gpio_pin_set_dt(&stop_output, false); // De-assert local stop
    }
	// Power saving
	//k_timer_stop(&periodic_timer); // Stop the timer
	// ToDo: Turn off ESB and UART
	//k_cpu_idle(); // Put CPU to sleep - power cycle to resume
}