/*
 * bms.c
 *
 * Created: 16/07/2023 19:59:42
 *  Author: David Pye
 */ 

#include "bms.h"

//We start off idle.
enum BMS_STATE bms_state = BMS_IDLE;

//If a fault occurs, it'll be lodged here.
enum BMS_ERROR_CODE bms_error = BMS_ERR_NONE;

void pins_init() {
	//Set up the output charge pin
	struct port_config charge_pin_config;
	port_get_config_defaults(&charge_pin_config);
	charge_pin_config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(ENABLE_CHARGE_PIN, &charge_pin_config);
	port_pin_set_output_level(ENABLE_CHARGE_PIN, false);
	
	//Two input pins, CHARGER and TRIGGERs
	struct port_config sense_pin_config;
	port_get_config_defaults(&sense_pin_config);
	sense_pin_config.direction = PORT_PIN_DIR_INPUT;
	sense_pin_config.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(CHARGER_CONNECTED_PIN, &sense_pin_config);
	port_pin_set_config(TRIGGER_PRESSED_PIN, &sense_pin_config);
}

	
void bms_init() {
	//sets up clocks/IRQ handlers etc.
	system_init();
	//Initialise the delay system
	delay_init();
	//Set up the pins
	pins_init();
	//BQ7693 init
	bq7693_init();
	//Init the LEDs
	leds_init();
	//Do pretty welcome sequence
	leds_sequence();
	//Initialise the USART we need to talk to the vacuum cleaner
	serial_init();
}
	
bool bms_is_safe_to_discharge() {
	//Clear error status.
	bms_error = BMS_ERR_NONE;
	
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	//Check any cells undervolt.
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] < CELL_LOWEST_DISCHARGE_VOLTAGE) {
			bms_error = BMS_ERR_PACK_DISCHARGED;
		}
	}
	
	//Check pack temperature remains in acceptable range	
	int temp = bq7693_read_temperature();
	if (temp/10  > MAX_PACK_TEMPERATURE) {
		bms_error = BMS_ERR_PACK_OVERTEMP;
	}
	else if (temp/10 < MIN_PACK_DISCHARGE_TEMP) {
		bms_error = BMS_ERR_PACK_UNDERTEMP;
	}
	
	//Check sys_stat	
	uint8_t sys_stat;
	bq7693_read_register(SYS_STAT, 1, &sys_stat);

	if (sys_stat & 0x01) 	{
		bms_error = BMS_ERR_OVERCURRENT;
		bq7693_write_register(SYS_STAT, 0x01);
	}
	else if (sys_stat & 0x02) {
		bms_error = BMS_ERR_SHORTCIRCUIT;
		bq7693_write_register(SYS_STAT, 0x02);
	}
	else if (sys_stat & 0x08) {
		bms_error = BMS_ERR_UNDERVOLTAGE;
		bq7693_write_register(SYS_STAT, 0x08);
	}	

	if (bms_error == BMS_ERR_NONE) {
		return true;
	}
	else return false;
	
}

bool bms_is_safe_to_charge() {
	//Clear error status.
	bms_error = BMS_ERR_NONE;
	
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	//Check no cells are so flat they cannot be charged.
	for (int i=0; i<7;++i) {
		if ( cell_voltages[i] < CELL_LOWEST_CHARGE_VOLTAGE ) {
			bms_error = BMS_ERR_CELL_FAIL;	
		}
	}

	//Check pack temperature acceptable (<=60'C)	
	int temp = bq7693_read_temperature();
	if (temp/10  > MAX_PACK_TEMPERATURE) {
		bms_error = BMS_ERR_PACK_OVERTEMP;
	}
	else if (temp/10 < MIN_PACK_CHARGE_TEMP) {
		bms_error = BMS_ERR_PACK_UNDERTEMP;
	}
	
	//Check sys_stat
	uint8_t sys_stat;
	bq7693_read_register(SYS_STAT, 1, &sys_stat);
	if (sys_stat & 0x01) 	{
		bms_error = BMS_ERR_OVERCURRENT;
		bq7693_write_register(SYS_STAT, 0x01);
	}
	else if (sys_stat & 0x04) {
		bms_error = BMS_ERR_OVERVOLTAGE;
		bq7693_write_register(SYS_STAT, 0x04);
	}
	
	if (bms_error == BMS_ERR_NONE) {
		return true;
	}
	else return false;
}

bool bms_is_pack_full() {
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	//If any cells are at their full charge voltage, we are full.
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] >= CELL_FULL_CHARGE_VOLTAGE ) {
			return true;
		}
	}
	return false;
}


void bms_handle_idle() {
	//Three potential ways out of this state - someone pulls the trigger, plugs in a charger, or the IDLE_TIME is exceeded and we go to sleep.
	for (int i=0; i<  (IDLE_TIME * 1000) / 50; ++i) {
		if (port_pin_get_input_level(CHARGER_CONNECTED_PIN) == true) {
			bms_state = BMS_CHARGER_CONNECTED;
			return;
		}
		else if (port_pin_get_input_level(TRIGGER_PRESSED_PIN) == true) {
			bms_state = BMS_TRIGGER_PULLED;
			return;
		}
		delay_ms(50);
	}
	
	//Reached the end of our wait loop, with nobody pulling the trigger, or plugging in charger.
	//Transit to sleep state
	bms_state = BMS_SLEEP;
}

void bms_handle_trigger_pulled() {
	//Check if it's safe to discharge or not.
	if (bms_is_safe_to_discharge()) {
		//All go - unleash the power!
		bms_state = BMS_DISCHARGING;
	}
	else {
		bms_state = BMS_FAULT;
	}
}

void bms_handle_sleep() {
	//Goodbye LED sequence
	leds_sequence();
	bq7693_enter_sleep_mode();
	//We are about to get powered down.
	while(1);
}

void bms_handle_discharging() {		
	
	//Show the battery voltage on the LEDs.
	leds_display_battery_voltage(bq7693_get_pack_voltage());
	
	if (bms_is_safe_to_discharge()) {
		//Sanity check, hopefully already checked prior to here!
		bq7693_enable_discharge();
		//Reset the UART message counter;
		serial_reset_message_counter();
		//Brief pause to allow vac to wake up before we start sending serial data at it.
		delay_ms(260);
	}
	
	while (1) {
		if (!port_pin_get_input_level(TRIGGER_PRESSED_PIN)) {
			//Trigger released.
			bq7693_disable_discharge();
			//Clear the battery status etc.
			leds_off();
			bms_state = BMS_IDLE;
			return;
		}
		if (!bms_is_safe_to_discharge()) {
			//A fault has occurred.
			bq7693_disable_discharge();
			bms_state = BMS_FAULT;
			return;
		}
		
		//No errors, and trigger pressed, so we continue to discharge.
		//Show the battery voltage on the LEDs.
		leds_display_battery_voltage(bq7693_get_pack_voltage());
		
		//Send the USART traffic we need to supply to keep the cleaner running
		serial_send_next_message();
		delay_ms(60);
	}
}

void bms_handle_fault() {
	//Turn all the LEDs off.
	leds_off();
	
	//Show the error status and continue to show it, until trigger released and charger unplugged.
	do {
		if (bms_error == BMS_ERR_PACK_DISCHARGED || bms_error == BMS_ERR_UNDERVOLTAGE ) {
			//If the problem is just a flat pack, blink the lowest battery segment three times.
			leds_show_pack_flat();
		}
		else {
			//Flash the red error led the number of times indicated by the fault code.
			for (int i=0; i<bms_error; ++i) {
				leds_blink_error_led(500);
			}
			delay_ms(2000);
		}
	} 
	while (port_pin_get_input_level(TRIGGER_PRESSED_PIN) || port_pin_get_input_level(CHARGER_CONNECTED_PIN));
		
	//Return to idle
	bms_state = BMS_IDLE;	
}

void bms_handle_charger_connected() {
	if (bms_is_pack_full()) {
		//If the pack is full, transit to idle.
		bms_state = BMS_IDLE;
	}
	else if (bms_is_safe_to_charge()) {
		bms_state = BMS_CHARGING;
	}
	else {
		bms_state = BMS_FAULT;
	}
}

void bms_handle_charger_connected_not_charging() {
	//Wait up to 30 seconds to see if someone unplugs the charger.
	//If so, to idle.
	//If not, to sleep.
	for (int i=0; i<30; ++i) {
		if (!port_pin_get_input_level(CHARGER_CONNECTED_PIN)) {
			bms_state = BMS_IDLE;
			return;
		}		
		delay_ms(1000);
	}
	//Sleep then!
	bms_state = BMS_SLEEP;
}

void bms_handle_charging() {
	//Sanity check...
	if (!bms_is_safe_to_charge()) {
		bms_state = BMS_FAULT;
		return;
	}
	//Enable charging.
	port_pin_set_output_level(ENABLE_CHARGE_PIN, true);
	 //Enable the charge FET in the BQ7693.
	bq7693_enable_charge();
	
	int charge_pause_counter = 0;
	while (1) {
		//Charging now in progress.		
		//Show flashing LED segment to indicate we are charging.
		leds_flash_charging_segment(bq7693_get_pack_voltage());
	
		
		if (!bms_is_safe_to_charge()) {
			//Safety error.
			port_pin_set_output_level(ENABLE_CHARGE_PIN, false);
			bq7693_disable_charge();

			leds_off();
			bms_state = BMS_FAULT;
			return;
		}
				
		if ( !port_pin_get_input_level(CHARGER_CONNECTED_PIN)) {
			//Charger unplugged.
			//Turn off charging
			port_pin_set_output_level(ENABLE_CHARGE_PIN, false);
			bq7693_disable_charge();

			leds_off();
			bms_state = BMS_CHARGER_UNPLUGGED;
			return;
		}
				
		if (bms_is_pack_full()) {
			//Pause the charging.
			port_pin_set_output_level(ENABLE_CHARGE_PIN, false);
			bq7693_disable_charge();
		
			//Delay for 30 seconds, then go and try again.	
			for (int i=0; i<30; ++i) {
				//This function takes a second.
				leds_flash_charging_segment(bq7693_get_pack_voltage());
				//Briefest red flicker.
				leds_blink_error_led(50);
				//Check the charger hasn't been unplugged while we're waiting
				//If it has, abandon the charge process and return to main loop
				if (!port_pin_get_input_level(CHARGER_CONNECTED_PIN)) {
					//Charger's been unplugged.
					leds_off();
					bms_state = BMS_CHARGER_UNPLUGGED;
					return;
				}
			}			
			charge_pause_counter++;	
			//Restart charging	
			port_pin_set_output_level(ENABLE_CHARGE_PIN, true);
			bq7693_enable_charge();
		}
		
		if (charge_pause_counter == 10) {
			//After 10 pauses, we are full.
			//Disable the charging
			port_pin_set_output_level(ENABLE_CHARGE_PIN, false);
			bq7693_disable_charge();
			
			leds_off();

			bms_state = BMS_CHARGER_CONNECTED_NOT_CHARGING;
			return;	
		}			
	}
}

void bms_handle_charger_unplugged() {
	//Do a little flash to show how out of sync the pack is, then go to idle.
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
		
	uint8_t highest_cell = 0;
	uint8_t lowest_cell = 0;
		
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] > cell_voltages[highest_cell]) {
			highest_cell = i;
		}
		if (cell_voltages[i] < cell_voltages[lowest_cell]) {
			lowest_cell = i;
		}
	}
	
	uint16_t spread = cell_voltages[highest_cell] - cell_voltages[lowest_cell];
	
	//Flash the error led for 100ms for each 50mV the pack is out of balance
	for (int i=0; i<round(spread/50); ++i) {
		leds_blink_error_led(100);	
	}
	
	bms_state = BMS_IDLE;
}


void bms_mainloop() {
	//Handle the state machinery.
	while (1) {
		switch (bms_state) {
			case BMS_IDLE:
				bms_handle_idle();
				break;
			case BMS_SLEEP:
				bms_handle_sleep();
				break;	
			case BMS_TRIGGER_PULLED:
				bms_handle_trigger_pulled();
				break;
			case BMS_CHARGER_CONNECTED:
				bms_handle_charger_connected();
				break;	
			case BMS_CHARGING:
				bms_handle_charging();
				break;
			case BMS_CHARGER_CONNECTED_NOT_CHARGING:
				bms_handle_charger_connected_not_charging();
				break;
			case BMS_CHARGER_UNPLUGGED:
				bms_handle_charger_unplugged();
				break;
			case BMS_DISCHARGING:
				bms_handle_discharging();
				break;
			case BMS_FAULT:
				bms_handle_fault();
				break;
		}
	}
}

