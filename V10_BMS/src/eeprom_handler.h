/*
 * eeprom_handler.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include <ctype.h>
#include <inttypes.h>
#include <string.h> //for memcpy
 
#include "eeprom.h"
#include "nvm.h"

#include "config.h"
#include "leds.h"
#include "serial_debug.h"

//A struct to represent the stored eeprom data
volatile struct eeprom_data {
	int32_t total_pack_capacity; //micro-amp-hours
	int32_t current_charge_level;	//micro-amp-hours
} eeprom_data;

int eeprom_init(void);

int eeprom_read();
int eeprom_write();

int eeprom_fuses_set(void);

#endif /* EEPROM_H_ */