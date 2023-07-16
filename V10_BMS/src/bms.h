/*
 * bms.h
 *
 * Created: 16/07/2023 20:00:06
 *  Author: David Pye
 */ 


#ifndef BMS_H_
#define BMS_H_

#include "asf.h"

#include "board.h"
#include "bq7693.h"
#include "serial.h"
#include "leds.h"


void pins_init(void);

void bms_init(void);
void bms_mainloop(void);

void bms_handle_idle(void);
void bms_handle_sleep(void);

enum BMS_STATE {
	BMS_IDLE,
	BMS_CHARGER_CONNECTED,
	BMS_CHARGING,
	BMS_CHARGING_FAULT,
	BMS_CHARGER_CONNECTED_NOT_CHARGING,
	BMS_CHARGER_UNPLUGGED,
	BMS_TRIGGER_PULLED,
	BMS_DISCHARGING,
	BMS_DISCHARGE_FAULT,
	BMS_SLEEP
};

bool bms_is_safe_to_discharge(void);

#endif /* BMS_H_ */