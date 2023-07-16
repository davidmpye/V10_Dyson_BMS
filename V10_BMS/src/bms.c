/*
 * bms.c
 *
 * Created: 16/07/2023 19:59:42
 *  Author: David Pye
 */ 

#include "bms.h"

//We start off idle.
enum BMS_STATE bms_state = BMS_IDLE;

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

bool bms_is_safe_to_discharge() {
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] < CELL_LOWEST_DISCHARGE_VOLTAGE) {
			return false;
		}
	}
	return true;
}

bool bms_is_safe_to_charge() {
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	for (int i=0; i<7;++i) {
		if ( cell_voltages[i] < CELL_LOWEST_CHARGE_VOLTAGE ) {
			return false;
		}
	}
	//Need to check temperatures too....

	//Need to check the SYS_STAT status to check it's happy too..
	
	//All seems OK
	return true;
}

bool bms_is_pack_full() {
	
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	//If any cells are at their full charge voltage, we are full.
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] >= CELL_FULL_CHARGE_VOLTAGE ) {
			return true;
		}
	}
	return false;
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

void bms_handle_idle() {
	//Three potential ways out of this state.
	for (int i=0; i< 30000/50; ++i) {
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
	//Display volts
	leds_display_battery_voltage(bq7693_get_pack_voltage());
	//Check if it's safe to discharge or not.
	if (bms_is_safe_to_discharge()) {
		//All go - unleash the power!
		bms_state = BMS_DISCHARGING;
	}
	else {
		bms_state = BMS_DISCHARGE_FAULT;
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
	if (bms_is_safe_to_discharge()) {
		//Sanity check, hopefully already checked prior to here!
		bq7693_enable_discharge();
		//Reset the UART message counter;
		serial_reset_message_counter();
		//Brief pause to allow vac to wake up before we start sending serial data at it.
		delay_ms(260);
	}
	
	while (1) {
		if (!port_pin_get_input_level(TRIGGER_PRESSED_PIN)   || !bms_is_safe_to_discharge) {
			//Either trigger released, or a fault has occurred.
			//Shut off the power.
			bq7693_disable_discharge();
			//Clear the battery status etc.
			leds_off();
			
			if (!port_pin_get_input_level(TRIGGER_PRESSED_PIN)) {
				//Trigger let go, will return to IDLE state.
				bms_state = BMS_IDLE;				
			}
			else if (!bms_is_safe_to_discharge()) {
				//No longer safe to discharge - transit to fault state.
				//Could be flat pack, overheat, overcurrent
				bms_state = BMS_DISCHARGE_FAULT;
			}
			return;
		}
		//No errors, and trigger pressed, so we continue to discharge.
		
		//Show the battery voltage on the LEDs.
		bq7693_update_voltages();
		leds_display_battery_voltage(bq7693_get_pack_voltage());
		
		//Send the USART traffic we need to supply to keep the cleaner running
		serial_send_next_message();
		delay_ms(60);
	}
}

void bms_handle_discharge_fault() {
	for (int i=0; i<5; ++i) {
		leds_blink_error_led(250);
	}
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
		bms_state = BMS_CHARGING_FAULT;
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
		bms_state = BMS_CHARGING_FAULT;
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
			bms_state = BMS_CHARGING_FAULT;
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
	bq7693_update_voltages();
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
			case BMS_DISCHARGE_FAULT:
				bms_handle_discharge_fault();
				break;
		}
	}
}

