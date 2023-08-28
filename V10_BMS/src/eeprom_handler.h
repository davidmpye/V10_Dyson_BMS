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

#include "eeprom.h"
#include "nvm.h"

#include "leds.h"

int eeprom_init(void);

int eeprom_read(uint8_t buffer, size_t len);
int eeprom_write(uint8_t buffer, size_t len);

int eeprom_fuses_set();

#endif /* EEPROM_H_ */