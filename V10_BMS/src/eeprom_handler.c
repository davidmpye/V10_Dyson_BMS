/*
 * eeprom.c
 *
 * Created: 28/08/2023 14:20:37
 *  Author: david
 */ 

#include "eeprom_handler.h"


void eeprom_init() {
	enum status_code error_code = eeprom_emulator_init();
	if (error_code == STATUS_ERR_NO_MEMORY) {
		while (true) {
			//Need to update the fuses and reset the device.
			
		}
	}
	else if (error_code != STATUS_OK) {
		//Init/format the eeprom
		eeprom_emulator_erase_memory();
		eeprom_emulator_init();
		//Write an initial guestimate of what a pack capacity might look like.
		
	}
}

int eeprom_read(uint8_t buffer, size_t len) {
	
	return 0;
}

int eeprom_write(uint8_t buffer, size_t len) {
	
	return 0;
}