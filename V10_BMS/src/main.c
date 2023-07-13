/**
 * \file
 *
 * \brief Empty user application template
 *
 */
//ASF libraries picked up here

#include "asf.h"

#include "leds.h"
#include "board.h"
#include "bq7693.h"
#include "serial.h"

void charge(void);
void discharge(void);

bool is_safe_to_charge() {
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	volatile int highest_cell = 0;
	volatile int lowest_cell = 0;
	
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] > cell_voltages[highest_cell]) {
			highest_cell = i;
		}
		if (cell_voltages[i] < cell_voltages[lowest_cell]) {
			lowest_cell = i;
		}
	}
	//Need to check temperatures too....

	//Need to check the SYS_STAT status to check it's happy too..
	
	//Pack full.
	if (cell_voltages[highest_cell] >= CELL_FULL_CHARGE_VOLTAGE)
		return false; 
		
	//A cell has fallen below our 'safe to try to charge' cutoff.
	if (cell_voltages[lowest_cell] < CELL_LOWEST_CHARGE_VOLTAGE) 
		return false;
		
	//Seems ok!
	return true;		
}

bool is_safe_to_discharge() {
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	volatile int highest_cell = 0;
	volatile int lowest_cell = 0;
	
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] > cell_voltages[highest_cell]) {
			highest_cell = i;
		}
		if (cell_voltages[i] < cell_voltages[lowest_cell]) {
			lowest_cell = i;
		}
	}
	
	if (cell_voltages[lowest_cell] < CELL_LOWEST_DISCHARGE_VOLTAGE) {
		return false;
	}
	
	return true;
	
}
int pack_spread_volts() {
	bq7693_update_voltages();
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	
	volatile int highest_cell = 0;
	volatile int lowest_cell = 0;
		
	for (int i=0; i<7;++i) {
		if (cell_voltages[i] > cell_voltages[highest_cell]) {
			highest_cell = i;
		}
		if (cell_voltages[i] < cell_voltages[lowest_cell]) {
			lowest_cell = i;
		}
	}
	return cell_voltages[highest_cell] - cell_voltages[lowest_cell];	
}

void charge() {
	bq7693_update_voltages();
	leds_display_battery_voltage(bq7693_get_pack_voltage());
	
	while (1) {
		 if (port_pin_get_input_level(CHARGER_CONNECTED_PIN) == false || !is_safe_to_charge()) {
			 //Charger unplugged or not safe to charge.
			 //Disable charging.
			 port_pin_set_output_level(PIN_PA02, false);
			 bq7693_disable_charge();
	 
	 		 //Show balancing state.
			 int pack_spread = pack_spread_volts();
			 for (int i=0; i< (int)pack_spread/50; ++i) {
				//One red flash for every 50mV 'out of balance' the pack is
				//as per tinfever's FW for the earlier BMSs.
				leds_blink_error_led(250); //ms
			 }
			 return;
		 }
		 
		 //Safe to charge, and charger connected.
		 //Enable charging pin and FET.
		 port_pin_set_output_level(PIN_PA02, true);
		 //Enable the charge FET in the BQ7693.
		 bq7693_enable_charge();
					  
		 while (port_pin_get_input_level(CHARGER_CONNECTED_PIN) == true) { 
			//MAIN CHARGE LOOP HERE
						
			//Show flashing LED segment to indicate we are charging.
			leds_flash_charging_segment(bq7693_get_pack_voltage());
			delay_ms(50);
			//Check still safe to charge.
			if (!is_safe_to_charge()) {		
				//This often happens because a cell has hit its' fully charged voltage threshold briefly.
				//Stop charging, wait 60 seconds, then check again.	
				port_pin_set_output_level(PIN_PA02, false);
				bq7693_disable_charge();	
				for (int i=0; i<60; ++i) {
					//This function takes a second.
					leds_flash_charging_segment(bq7693_get_pack_voltage());
					//Briefest red flicker.
					leds_blink_error_led(20);
					//Check the charger hasn't been unplugged while we're waiting
					//If it has, abandon the charge process and return to main loop
					if (port_pin_get_input_level(CHARGER_CONNECTED_PIN) == false)
						break;
				}
				//this will return us to the initial safe to charge test at start of loop.
				break;
			
			}
		 }
	}		
}

void discharge() {
	if (port_pin_get_input_level(TRIGGER_PRESSED_PIN) == true && is_safe_to_discharge()) {
		//POWER ON!
		bq7693_enable_discharge();
		//Reset the USART message counter to start of message sequence
		serial_reset_message_counter();
		//Brief pause to allow vac to wake up before we start sending serial data at it.
		delay_ms(200);
	}
	while (port_pin_get_input_level(TRIGGER_PRESSED_PIN) == true && is_safe_to_discharge()) {
		//Show the battery 'SoC' (yeah, I know, just voltage for now)
		leds_off();
		leds_display_battery_voltage(bq7693_get_pack_voltage());
		//Send the USART traffic we need to supply to keep the cleaner running
		serial_send_next_message();
		delay_ms(60);
	}
	bq7693_disable_discharge();	
	leds_off();
}


int main (void){
	//We've been either RESET or woken up by the BQ7693
	//It will wake us up either because the microswitch has been pressed, or because the charger
	//is connected. 
	
	//sets up clocks/IRQ handlers etc.
	system_init();
	//Initialise the delay system
	delay_init(); 
	bq7693_init();
	
	//This needs farmed out into an init routine
	struct port_config charge_pin_config;
	port_get_config_defaults(&charge_pin_config);
	charge_pin_config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA02, &charge_pin_config);
	port_pin_set_output_level(PIN_PA02, false);
	
	struct port_config sense_pin_config;
	port_get_config_defaults(&sense_pin_config);
	sense_pin_config.direction = PORT_PIN_DIR_INPUT;
	sense_pin_config.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(CHARGER_CONNECTED_PIN, &sense_pin_config);
	port_pin_set_config(TRIGGER_PRESSED_PIN, &sense_pin_config);
				
	//Init the LEDs.
	leds_init();
	//Do pretty welcome sequence
	leds_sequence();
	//Initialise the USART we need to talk to the vacuum cleaner
	serial_init();

	//Main loop waits for 5 seconds since last event then we go to sleep until woken.
	for (int i=0; i< 5000/50; ++i) {
		if (port_pin_get_input_level(CHARGER_CONNECTED_PIN) == true) {
			//start charge.
			charge();
			i=0; //reset counter
		}
		else if (port_pin_get_input_level(TRIGGER_PRESSED_PIN) == true) {
			discharge();
			i=0; //reset counter
		}
		delay_ms(50);
	}

	//Nothing happened, bored now.
	leds_sequence();
	bq7693_enter_sleep_mode();
	while(1);
	//Goodbye cruel world.
}

