#ifndef APP_H
#define APP_H

#include <Arduino.h>
/** Add you required includes after Arduino.h */

#include <Wire.h>
#include <Adafruit_MCP23X17.h> //click here to get the library: http://librarymanager/All#Adafruit_MCP23017

/** Include the WisBlock-API */
#include <WisBlock-API.h>

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

// Enable perpetual BLE advertising
#ifndef BLE_ADVERTISE_FOREVER
#define BLE_ADVERTISE_FOREVER 1
#endif

/** GPIO pins for valve control */
#define VPIN_OPEN 6
#define VPIN_CLOSED 7
#define VALVE_STATE_CLOSED 0
#define VALVE_STATE_OPENED 1

/** How long to hold the relay input signal HIGH to allow for valve
 * to fully transition between open/closed. May be tweaked per valve. */
#define DEFAULT_VALVE_OPER_TIME_SEC 6

/** User defined structure for storing valve state */
struct s_valve_settings
{
	uint8_t state;					 // Current valve state
	uint8_t oper_time_sec;			 // How long it takes to open/close the valve
	bool valve_interval_started;	 // Is the valve interval running?
	int valve_interval_begin_millis; // When did the valve interval begin? (timestamp)
	int valve_interval_millis;		 // How long the current valve interval is
};

#ifdef NRF52_SERIES
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                     \
	do                                      \
	{                                       \
		if (tag)                            \
			PRINTF("[%s] ", tag);           \
		PRINTF(__VA_ARGS__);                \
		PRINTF("\n");                       \
		if (g_ble_uart_is_connected)        \
		{                                   \
			g_ble_uart.printf(__VA_ARGS__); \
			g_ble_uart.printf("\n");        \
		}                                   \
	} while (0)
#else
#define MYLOG(...)
#endif
#endif
#ifdef ARDUINO_ARCH_RP2040
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0)
#else
#define MYLOG(...)
#endif
#endif

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

// LoRaWan functions (TBD - more efficient bit packing)
struct lpwan_data_s
{
	uint8_t batt_1 = 0;
	uint8_t batt_2 = 0;
	uint8_t valve_inteval_1 = 0;
	uint8_t valve_inteval_2 = 0;
	bool valve_opened = false;
};
extern lpwan_data_s g_lpwan_data;
#define LPWAN_DATA_LEN sizeof(lpwan_data_s)

/** Battery level uinion */
union batt_s
{
	uint16_t batt16 = 0;
	uint8_t batt8[2];
};

/** Valve info union */
union valve_s
{
	uint16_t valve_ts16 = 0; // Interval remaining in seconds
	uint8_t valve_ts8[2];
};

#endif