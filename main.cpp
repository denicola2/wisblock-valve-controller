#include "app.h"

/** Define the version of your SW */
#define SW_VERSION_1 1 // major version increase on API change / not backwards compatible
#define SW_VERSION_2 0 // minor version increase on API change / backward compatible
#define SW_VERSION_3 0 // patch version increase on bugfix, no affect on API

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-VLVC";

/** Send Fail counter **/
uint8_t send_fail = 0;

/** Flag for low battery protection */
bool low_batt_protection = false;

/** Battery level union */
batt_s batt_level;

/** Valve interval state union */
valve_s valve_state;

/** LPWAN packet */
lpwan_data_s g_lpwan_data;

/** RAK13003 GPIO Expander */
Adafruit_MCP23X17 mcp;

/** Global settings for valve state */
s_valve_settings g_valve_settings;

/**
 * Optional hard-coded LoRaWAN credentials for OTAA and ABP.
 * It is strongly recommended to avoid duplicated node credentials
 * Options to setup credentials are
 * - over USB with AT commands
 * - over BLE with My nRF52 Toolbox
 */
uint8_t node_device_eui[8] = {0x00, 0x0D, 0x75, 0xE6, 0x56, 0x4D, 0xC1, 0xF3};
uint8_t node_app_eui[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x02, 0x01, 0xE1};
uint8_t node_app_key[16] = {0x2B, 0x84, 0xE0, 0xB0, 0x9B, 0x68, 0xE5, 0xCB, 0x42, 0x17, 0x6F, 0xE7, 0x53, 0xDC, 0xEE, 0x79};
uint8_t node_nws_key[16] = {0x32, 0x3D, 0x15, 0x5A, 0x00, 0x0D, 0xF3, 0x35, 0x30, 0x7A, 0x16, 0xDA, 0x0C, 0x9D, 0xF5, 0x3F};
uint8_t node_apps_key[16] = {0x3F, 0x6A, 0x66, 0x45, 0x9D, 0x5E, 0xDC, 0xA6, 0x3C, 0xBC, 0x46, 0x19, 0xCD, 0x61, 0xA1, 0x1E};

/** Flag showing if TX cycle is ongoing */
bool lora_busy = false;

/** Prototypes */
int setValve(int state, int sec);

/** User timer */
TimerEvent_t valveTimer;

void valve_interval_expiry_handler(void)
{
	MYLOG("APP", "Timer handling valve interval expiry, closing valve");
	setValve(VALVE_STATE_CLOSED, g_valve_settings.oper_time_sec);
	g_valve_settings.valve_interval_started = false;
}

uint32_t app_timers_init()
{
	TimerInit(&valveTimer, valve_interval_expiry_handler);
	return 0;
}

/**
   @brief Application specific setup functions
*/
void setup_app(void)
{
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, 1);

	// Reset device
	pinMode(WB_IO4, OUTPUT);
	digitalWrite(WB_IO4, 1);
	delay(10);
	digitalWrite(WB_IO4, 0);
	delay(10);
	digitalWrite(WB_IO4, 1);
	delay(10);

	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
	digitalWrite(LED_GREEN, LOW);

	MYLOG("APP", "WisBlock Valve Controller");

#ifdef NRF52_SERIES
	// Enable BLE
	g_enable_ble = true;
#endif

	// Set firmware version
	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);

	// Setup GPIO Expander
	MYLOG("APP", "Initializing RAK13003 GPIO Expander");
	mcp.begin_I2C(); // Use default address 0.
	mcp.pinMode(VPIN_OPEN, OUTPUT);
	mcp.pinMode(VPIN_CLOSED, OUTPUT);

	MYLOG("APP", "Initializing LoRaWAN settings");
	// Setup LoRaWAN credentials hard coded
	// It is strongly recommended to avoid duplicated node credentials
	// Options to setup credentials are
	// -over USB with AT commands
	// -over BLE with My nRF52 Toolbox

	// Read LoRaWAN settings from flash
	api_read_credentials();

	// Change LoRaWAN settings
	g_lorawan_settings.lorawan_enable = true;
	g_lorawan_settings.auto_join = true;							// Flag if node joins automatically after reboot
	g_lorawan_settings.otaa_enabled = true;							// Flag for OTAA or ABP
	memcpy(g_lorawan_settings.node_device_eui, node_device_eui, 8); // OTAA Device EUI MSB
	memcpy(g_lorawan_settings.node_app_eui, node_app_eui, 8);		// OTAA Application EUI MSB
	memcpy(g_lorawan_settings.node_app_key, node_app_key, 16);		// OTAA Application Key MSB
	memcpy(g_lorawan_settings.node_nws_key, node_nws_key, 16);		// ABP Network Session Key MSB
	memcpy(g_lorawan_settings.node_apps_key, node_apps_key, 16);	// ABP Application Session key MSB
	g_lorawan_settings.node_dev_addr = 0x26021FB4;					// ABP Device Address MSB
	g_lorawan_settings.send_repeat_time = 300000;					// Send repeat time in milliseconds: 5 * 60 * 1000 => 5 minutes
	g_lorawan_settings.adr_enabled = false;							// Flag for ADR on or off
	g_lorawan_settings.public_network = true;						// Flag for public or private network
	g_lorawan_settings.duty_cycle_enabled = false;					// Flag to enable duty cycle (validity depends on Region)
	g_lorawan_settings.join_trials = 10;							// Number of join retries
	g_lorawan_settings.tx_power = TX_POWER_0;						// TX power 0 .. 15 (validity depends on Region)
	g_lorawan_settings.data_rate = DR_3;							// Data rate 0 .. 15 (validity depends on Region)
	g_lorawan_settings.lora_class = CLASS_A;						// LoRaWAN class 0: A, 2: C, 1: B is not supported
	g_lorawan_settings.subband_channels = 2;						// Subband channel selection 1 .. 9
	g_lorawan_settings.app_port = LORAWAN_APP_PORT;					// Data port to send data
	g_lorawan_settings.confirmed_msg_enabled = LMH_CONFIRMED_MSG;	// Flag to enable confirmed messages
	g_lorawan_settings.resetRequest = true;							// Command from BLE to reset device
	g_lorawan_settings.lora_region = LORAMAC_REGION_US915;			// LoRa region

	// Save LoRaWAN settings
	api_set_credentials();

	// Create a user timer to periodically check valve interval
	MYLOG("APP", "Initializing valve timer");
	app_timers_init();

	// Initialize valve settings
	MYLOG("APP", "Initializing valve settings");
	g_valve_settings.valve_interval_started = false;
	g_valve_settings.oper_time_sec = DEFAULT_VALVE_OPER_TIME_SEC;
	g_valve_settings.state = VALVE_STATE_CLOSED;
}

/**
   @brief Application specific initializations

   @return true Initialization success
   @return false Initialization failure
*/
bool init_app(void)
{
	// MYLOG("APP", "init_app");
#if 0
	Serial.println("================================================");
	Serial.println("WisBlock Valve Controller");
	Serial.println("================================================");
	api_log_settings();
	Serial.println("================================================");
#endif
	// Ensure our valve is closed at initialization
	MYLOG("APP", "Setting default state: Closing valve");
	setValve(VALVE_STATE_CLOSED, g_valve_settings.oper_time_sec);

#ifdef BLE_ADVERTISE_FOREVER
	// Start bluetooth to run forever
	restart_advertising(0);
#endif

	return true;
}

/**
   @brief Application specific event handler
		  Requires as minimum the handling of STATUS event
		  Here you handle as well your application specific events
*/
void app_event_handler(void)
{
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");

#ifndef BLE_ADVERTISE_FOREVER
#ifdef NRF52_SERIES
		// If BLE is enabled, restart Advertising
		if (g_enable_ble)
		{
			if (g_lorawan_settings.auto_join)
			{
				restart_advertising(15);
			}
		}
#endif
#endif

		if (!low_batt_protection)
		{
			// Read the current remaining valve interval and state
			if (g_valve_settings.valve_interval_started)
			{
				int curMillis = millis();
				int diff = curMillis - g_valve_settings.valve_interval_begin_millis;
				int remain = g_valve_settings.valve_interval_millis - diff;
				valve_state.valve_ts16 = (uint16_t)(remain / 1000);
			}
			else
			{
				valve_state.valve_ts16 = 0;
			}

			g_lpwan_data.valve_inteval_1 = valve_state.valve_ts8[1];
			g_lpwan_data.valve_inteval_2 = valve_state.valve_ts8[0];

			// Current valve state (single bit)
			g_lpwan_data.valve_opened = (g_valve_settings.state == VALVE_STATE_OPENED);
		}

		if (lora_busy)
		{
			MYLOG("APP", "LoRaWAN TX cycle not finished, skip this event");
		}
		else
		{
			// Get battery level
			batt_level.batt16 = read_batt() / 10;
			g_lpwan_data.batt_1 = batt_level.batt8[1];
			g_lpwan_data.batt_2 = batt_level.batt8[0];

			// Protection against battery drain
			if (batt_level.batt16 < 290)
			{
				// Battery is very low, change send time to 1 hour to protect battery
				low_batt_protection = true;			   // Set low_batt_protection active
				api_timer_restart(1 * 60 * 60 * 1000); // Set send time to one hour
				MYLOG("APP", "Battery protection activated");
			}
			else if ((batt_level.batt16 > 380) && low_batt_protection)
			{
				// Battery is higher than 4V, change send time back to original setting
				low_batt_protection = false;
				api_timer_restart(g_lorawan_settings.send_repeat_time);
				MYLOG("APP", "Battery protection deactivated");
			}

			lmh_error_status result = send_lora_packet((uint8_t *)&g_lpwan_data, LPWAN_DATA_LEN);
			switch (result)
			{
			case LMH_SUCCESS:
				MYLOG("APP", "Packet enqueued");
				// Set a flag that TX cycle is running
				lora_busy = true;
				break;
			case LMH_BUSY:
				MYLOG("APP", "LoRa transceiver is busy");
				break;
			case LMH_ERROR:
				MYLOG("APP", "Packet error, too big to send with current DR");
				break;
			}
		}
	}
}

#ifdef NRF52_SERIES
/**
   @brief Handle BLE UART data

*/
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		// BLE UART data handling
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			/** BLE UART data arrived */
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}
#endif

/**
   @brief Handle received LoRa Data

*/
void lora_data_handler(void)
{
	// AD_DEBUG - handle downlink events here (valve controls)

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo LoRa data arrived
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		char log_buff[g_rx_data_len * 3] = {0};
		uint8_t log_idx = 0;
		for (int idx = 0; idx < g_rx_data_len; idx++)
		{
			sprintf(&log_buff[log_idx], "%02X ", g_rx_lora_data[idx]);
			log_idx += 3;
		}
		lora_busy = false;
		MYLOG("APP", "%s", log_buff);
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		MYLOG("APP", "LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");

		if (!g_rx_fin_result)
		{
			// Increase fail send counter
			send_fail++;

			if (send_fail == 10)
			{
				// Too many failed sendings, reset node and try to rejoin
				delay(100);
				api_reset();
			}
		}

		// Clear the LoRa TX flag
		lora_busy = false;
	}

	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");
		}
		else
		{
			MYLOG("APP", "Join network failed");
			/// \todo here join could be restarted.
			// lmh_join();
		}
	}
}