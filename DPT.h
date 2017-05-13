/**
 * esp-knx-ip library for KNX/IP communication on an ESP8266
 * Author: Nico Weichbrodt <envy>
 * License: Not yet decided on one...
 */

typedef enum __dpt_1_001
{
	DPT_1_001_OFF = 0x00,
	DPT_1_001_ON  = 0x01,
} dpt_1_001_t;

typedef enum __dpt_3_001
{
	DPT_3_001_DECREASE_STOP = 0x00,
	DPT_3_001_DECREASE_100  = 0x01,
	DPT_3_001_DECREASE_50   = 0x02,
	DPT_3_001_DECREASE_25   = 0x03,
	DPT_3_001_DECREASE_12   = 0x04,
	DPT_3_001_DECREASE_6    = 0x05,
	DPT_3_001_DECREASE_3    = 0x06,
	DPT_3_001_DECREASE_1    = 0x07,
	DPT_3_001_INCREASE_STOP = 0x08,
	DPT_3_001_INCREASE_100  = 0x09,
	DPT_3_001_INCREASE_50   = 0x0A,
	DPT_3_001_INCREASE_25   = 0x0B,
	DPT_3_001_INCREASE_12   = 0x0C,
	DPT_3_001_INCREASE_6    = 0x0D,
	DPT_3_001_INCREASE_3    = 0x0E,
	DPT_3_001_INCREASE_1    = 0x0F,
} dpt_3_001_t;