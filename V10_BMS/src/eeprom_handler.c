/*
 *  eeprom_handler.c
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#include "eeprom_handler.h"

int eeprom_init() {
	enum status_code error_code = eeprom_emulator_init();
	if (error_code == STATUS_ERR_NO_MEMORY) {
		//We are here because the fuses are set to 0x07, meaning eeprom is not enabled.
		//Show a few slow flashes to make it clear we're up to something, then reprogram fuses and reset 
		//the mcu.
		for (int i=0; i<4; ++i) {
			leds_blink_error_led(2000);
		}
		//This will update the fuses then reset the MCU
		eeprom_fuses_set();
	}
	else if (error_code != STATUS_OK) {
		//Init/format the eeprom
		eeprom_emulator_erase_memory();
		error_code = eeprom_emulator_init();
		//Write an initial guestimate of what a pack capacity might look like, we'll fine tune this by charging and discharging.
		eeprom_data.total_pack_capacity = 2000000;  //in microAmpHours - equiv of 2000mAh.
		eeprom_data.current_charge_level = 1000000; //half charged.
		eeprom_write();
		eeprom_emulator_commit_page_buffer();
	}
	
	return error_code;
}

int eeprom_read() {
	volatile uint8_t buffer[EEPROM_PAGE_SIZE];
	eeprom_emulator_read_page(0, buffer);
	memcpy(&eeprom_data, buffer, sizeof(eeprom_data));
	return 0;
}

int eeprom_write() {
	uint8_t buffer[EEPROM_PAGE_SIZE];
	memcpy(buffer, &eeprom_data, sizeof(eeprom_data));
	eeprom_emulator_write_page(0, buffer);	
	eeprom_emulator_commit_page_buffer();
	return 0;
}

int eeprom_fuses_set() {
	//Set the the NVM
	struct nvm_config config_nvm;
	nvm_get_config_defaults(&config_nvm);
	nvm_set_config(&config_nvm);
	
	uint32_t temp;
	uint32_t data[2];
	
	/* Wait for NVM command to complete */
	while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));
	  
	/* Read the fuse settings in the user row, 64 bit */
	data[0] = *((uint32_t *)NVMCTRL_AUX0_ADDRESS);
	data[1] = *(((uint32_t *)NVMCTRL_AUX0_ADDRESS) + 1);

	//Configure the fuse bits to enable EEPROM 1024bytes - minimal size for the ASF eeprom library to use.
	//Bits 4-6 specify eeprom size.
	//Clear bits 4-6.
	data[0] &= ~0x00000070;
	//Eeprom to 1024 bytes / 4 rows (EEPROM bits 0x04)
	data[0] |=  0x00000040;

	//Writeback sequence from https://microchip.my.site.com/s/article/SAMD20-SAMD21-Programming-the-fuses-from-application-code
	/* Disable Cache */
	temp = NVMCTRL->CTRLB.reg;
	NVMCTRL->CTRLB.reg = temp | NVMCTRL_CTRLB_CACHEDIS;
	
	/* Clear error flags */
	NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;

	/* Set address, command will be issued elsewhere */
	NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS/2;
	
	/* Erase the user page */
	NVMCTRL->CTRLA.reg = NVM_COMMAND_ERASE_AUX_ROW | NVMCTRL_CTRLA_CMDEX_KEY;
	
	/* Wait for NVM command to complete */
	while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));
	
	/* Clear error flags */
	NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
	
	/* Set address, command will be issued elsewhere */
	NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS/2;
	
	/* Erase the page buffer before buffering new data */
	NVMCTRL->CTRLA.reg = NVM_COMMAND_PAGE_BUFFER_CLEAR | NVMCTRL_CTRLA_CMDEX_KEY;

	/* Wait for NVM command to complete */
	while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));
	
	/* Clear error flags */
	NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
	
	/* Set address, command will be issued elsewhere */
	NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS/2;
	
	// Write back the updated fuse bits.
	*((uint32_t *)NVMCTRL_AUX0_ADDRESS) = data[0];
	*(((uint32_t *)NVMCTRL_AUX0_ADDRESS) + 1) = data[1];
	
	/* Write the user page */
	NVMCTRL->CTRLA.reg = NVM_COMMAND_WRITE_AUX_ROW | NVMCTRL_CTRLA_CMDEX_KEY;
	
	/* Restore the settings */
	NVMCTRL->CTRLB.reg = temp;
	
	//Reset the MCU
	NVIC_SystemReset();
}