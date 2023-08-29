/*
 * bms.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 


#ifndef BMS_H_
#define BMS_H_

#include "asf.h"
#include "board.h"

#include "bq7693.h"
#include "serial.h"
#include "leds.h"
#include "eeprom_handler.h"
#include "serial_debug.h"
#include "config.h"

void pins_init(void);

void bms_init(void);
void bms_mainloop(void);

void bms_handle_idle(void);
void bms_handle_sleep(void);
void bms_handle_trigger_pulled(void);
void bms_handle_discharging(void);
void bms_handle_fault(void);
void bms_handle_charger_connected(void);
void bms_handle_charger_connected_not_charging(void);
void bms_handle_charging(void);
void bms_handle_charger_unplugged(void);


bool bms_is_pack_full(void);
bool bms_is_safe_to_discharge(void);
bool bms_is_safe_to_charge(void);

enum BMS_STATE {
	BMS_IDLE,
	BMS_CHARGER_CONNECTED,
	BMS_CHARGING,
	BMS_CHARGER_CONNECTED_NOT_CHARGING,
	BMS_CHARGER_UNPLUGGED,
	BMS_TRIGGER_PULLED,
	BMS_DISCHARGING,
	BMS_FAULT, //error code should be logged to explain why!
	BMS_SLEEP
};

enum BMS_ERROR_CODE {
	BMS_ERR_NONE,			//All good!
	BMS_ERR_PACK_OVERTEMP,	//Pack thermistor reading exceeded MAX_PACK_TEMPERATURE - default 60'C
	BMS_ERR_PACK_UNDERTEMP, //Pack is below -40 if attempting discharge, or -0 if attempting charge.
	BMS_ERR_CELL_FAIL,		//A cell voltage is below safe minimum.
	BMS_ERR_SHORTCIRCUIT,	//BMS detected short circuit
	BMS_ERR_OVERCURRENT,	//BMS detected overcurrent fault
	BMS_ERR_OVERVOLTAGE,	//BMS detected overvoltage state
	BMS_ERR_I2C_FAIL,		//Unable to talk to the BQ7693 IC - very bad!
	BMS_ERR_PACK_DISCHARGED,//Pack is flat - not really a bad error....
	BMS_ERR_UNDERVOLTAGE,	//BMS detected undervoltage state - flat pack, but detected by the BQ.
};



#endif /* BMS_H_ */