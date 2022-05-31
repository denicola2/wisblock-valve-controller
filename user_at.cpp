#include "app.h"

extern Adafruit_MCP23X17 mcp;
extern s_valve_settings g_valve_settings;
extern TimerEvent_t valveTimer;

/**
 * @brief Example how to show the last LoRa packet content
 *
 * @return int always 0
 */
static int at_query_packet()
{
	snprintf(g_at_query_buf, ATQUERY_SIZE, "Packet: %02X%02X%02X%02X%02X",
			 g_lpwan_data.batt_1,
			 g_lpwan_data.batt_2,
			 g_lpwan_data.valve_inteval_1,
			 g_lpwan_data.valve_inteval_2,
			 g_lpwan_data.valve_opened);
	return 0;
}

/**
 * @brief Returns the current value of the valve controller
 *
 * @return int always 0
 */
static int at_query_valve()
{
	snprintf(g_at_query_buf, ATQUERY_SIZE, "Valve State: %d", g_valve_settings.state);
	return 0;
}

void setValve(int state, int sec)
{
	if (!sec)
		sec = DEFAULT_VALVE_OPER_TIME_SEC;

	// Only modify valve state in here!
	if (state)
	{
		mcp.digitalWrite(VPIN_OPEN, HIGH);
		delay(sec * 1000);
		mcp.digitalWrite(VPIN_OPEN, LOW);
		g_valve_settings.state = VALVE_STATE_OPENED;
		MYLOG("APP", "Valve opened");
	}
	else
	{
		mcp.digitalWrite(VPIN_CLOSED, HIGH);
		delay(sec * 1000);
		mcp.digitalWrite(VPIN_CLOSED, LOW);
		g_valve_settings.state = VALVE_STATE_CLOSED;
		MYLOG("APP", "Valve closed");
	}
}

void beginValveInterval(int sec)
{
	// Open the valve for sec seconds starting now
	setValve(VALVE_STATE_OPENED, g_valve_settings.oper_time_sec);
	g_valve_settings.valve_interval_millis = sec * 1000;
	g_valve_settings.valve_interval_begin_millis = millis();
	g_valve_settings.valve_interval_started = true;
	TimerSetValue(&valveTimer, g_valve_settings.valve_interval_millis);
	TimerStart(&valveTimer);
	MYLOG("APP", "Valve interval of %d sec started", sec);
}

/**
 * @brief Command to begin the valve OPEN interval
 *
 * @param str the new value for the variable without the AT part
 * @return int 0 if the command was succesfull, 5 if the parameter was wrong
 */
static int at_query_valve_interval()
{
	if (g_valve_settings.valve_interval_started)
	{
		int curMillis = millis();
		int diff = curMillis - g_valve_settings.valve_interval_begin_millis;
		int remain = g_valve_settings.valve_interval_millis - diff;
		snprintf(g_at_query_buf, ATQUERY_SIZE, "Sec remaining: %d", remain / 1000);
	}
	else
	{
		snprintf(g_at_query_buf, ATQUERY_SIZE, "No interval running");
	}
	return 0;
}

/**
 * @brief Command to begin the valve OPEN interval, provided seconds
 *
 * @param str the new value for the variable without the AT part
 * @return int 0 if the command was succesfull, 5 if the parameter was wrong
 */
static int at_exec_valve_interval(char *str)
{
	int sec = strtol(str, NULL, 0);
	if (g_valve_settings.valve_interval_started)
	{
		MYLOG("APP", "Valve interval already running");
		return 5;
	}
	else
	{
		beginValveInterval(sec);
		return 0;
	}
}

/**
 * @brief Command to set the valve operational interval
 * i.e. How long the signal is held high to close/open the valve
 *
 * @param str the new value for the variable without the AT part
 * @return int 0 if the command was succesfull, 5 if the parameter was wrong
 */
static int at_exec_valve_oper_time(char *str)
{
	int sec = strtol(str, NULL, 0);
	g_valve_settings.oper_time_sec = sec;
	MYLOG("APP", "Valve oper time set to %d sec", sec);
	return 0;
}

/**
 * @brief Returns the current value of the valve operational interval
 *
 * @return int always 0
 */
static int at_query_valve_oper_time()
{
	snprintf(g_at_query_buf, ATQUERY_SIZE, "Valve oper time: %d", g_valve_settings.oper_time_sec);
	return 0;
}

/**
 * @brief Command to set the valve control
 *
 * @param str the new value for the variable without the AT part
 * @return int 0 if the command was succesfull, 5 if the parameter was wrong
 */
static int at_exec_valve(char *str)
{
	int valve_time = g_valve_settings.oper_time_sec;
	int valve_state;

	if (g_valve_settings.valve_interval_started)
	{
		MYLOG("APP", "Valve interval is already started, overriding it with manual control");
		g_valve_settings.valve_interval_started = false;
	}

	if (strstr(str, ":"))
	{
		if (sscanf(str, "%d:%d", &valve_state, &valve_time) == 2)
		{
			MYLOG("APP", "Valve State %d for %d sec", valve_state, valve_time);
		}
		else
		{
			MYLOG("APP", "Invalid arguments provided");
			return 5;
		}
	}
	else
	{
		valve_state = strtol(str, NULL, 0);
		MYLOG("APP", "Valve State %d", valve_state);
	}

	if (valve_state)
	{
		// Open the valve
		setValve(VALVE_STATE_OPENED, valve_time);
	}
	else
	{
		// Close the valve
		setValve(VALVE_STATE_CLOSED, valve_time);
	}

	return 0;
}

/**
 * @brief Command to Reboot the device
 *
 * @param str must be 1 to begin reboot
 * @return int 0 if the command was succesfull, 5 if the parameter was wrong
 */
static int at_exec_reboot(char *str)
{
	int confirm = strtol(str, NULL, 0);
	if (confirm)
	{
		MYLOG("APP", "Rebooting...");
		delay(1000);
		system("reboot");
		return 0;
	}
	else
	{
		MYLOG("APP", "Reboot command not confirmed");
		return 5;
	}
}

/**
 * @brief List of all available commands with short help and pointer to functions
 *
 *  Example AT commands:
 *  AT+LIST=?   - List the last lorwan packet content
 *  AT+REBOOT=1 - Reboot the WisBlock
 *  AT+VLVS=1   - Set valve state to open
 *  AT+VLVS=?   - Get current valve state
 *  AT+VLVO=10  - Set the valve operational time to 10 sec (how long it takes the valve to fully open/close)
 *  AT+VLVO=?   - Get the current valve operational time
 *  AT+VLVI=600 - Open the valve for 10 minutes (60 sec & 10), the valve will automatically close after expiry
 *  AT+VLVI=?   - Get how many seconds remain before the valve closes
 * 
 */
atcmd_t g_user_at_cmd_list_example[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  |*/
	// GNSS commands
	{"+LIST", "Show last packet content", at_query_packet, NULL, NULL},
	{"+REBOOT", "Reboot the device", NULL, at_exec_reboot, NULL},
	{"+VLVS", "Get/Set the valve state (optional :sec)", at_query_valve, at_exec_valve, NULL},
	{"+VLVO", "Get/Set the valve operational time", at_query_valve_oper_time, at_exec_valve_oper_time, NULL},
	{"+VLVI", "Start valve open interval sec/Get remaining", at_query_valve_interval, at_exec_valve_interval, NULL},
};

/** Number of user defined AT commands */
uint8_t g_user_at_cmd_num = sizeof(g_user_at_cmd_list_example) / sizeof(atcmd_t);

/** Pointer to the AT command list */
atcmd_t *g_user_at_cmd_list = g_user_at_cmd_list_example;