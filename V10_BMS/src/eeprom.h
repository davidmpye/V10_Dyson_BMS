/*
 * eeprom.h
 *
 * Created: 28/08/2023 14:20:53
 *  Author: david
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include <ctype.h>

void eeprom_init(void);

int eeprom_read(uint8_t buffer, size_t len);
int eeprom_write(uint8_t buffer, size_t len);

#endif /* EEPROM_H_ */