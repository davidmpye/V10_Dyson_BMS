/*
 * leds.c
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 
#include "leds.h"

//speed of LED sequence
#define LED_SEQ_TIME 10

#if DYSON_VERSION == 10
#define NUM_LEDS 6
uint16_t leds[] = { LED_FILTER, LED_BLOCKED, LED_ERR, LED_BAT_LO, LED_BAT_MED, LED_BAT_HI };
#elif DYSON_VERSION == 11
#define NUM_LEDS 2
uint16_t leds[] = { LED_LEFT, LED_RIGHT };
#endif

void leds_init() {
	//Set up the LED pins as IO
	struct port_config led_port_config;
	port_get_config_defaults(&led_port_config);
	led_port_config.direction = PORT_PIN_DIR_OUTPUT;
	for (int i=0; i<NUM_LEDS; ++i) {
		port_pin_set_config(leds[i], &led_port_config);
	}
}

void leds_sequence() {	
	for (int i=0; i<NUM_LEDS; ++i) {
		port_pin_set_output_level(leds[i], true);
		delay_ms(LED_SEQ_TIME);
	}
	for (int i = NUM_LEDS-1; i>=0; --i) {
		port_pin_set_output_level(leds[i], false);
		delay_ms(LED_SEQ_TIME);
	}
}

void leds_off() {
	for (int i=0; i<NUM_LEDS; ++i) {
		port_pin_set_output_level(leds[i], false);
	}
}

void leds_on() {
	for (int i=0; i<NUM_LEDS; ++i) {
		port_pin_set_output_level(leds[i], true);
	}
}

#if DYSON_VERSION == 10
void leds_display_battery_soc(int percent_soc) {
	//LEDs off to start
	port_pin_set_output_level(LED_BAT_LO, false );
	port_pin_set_output_level(LED_BAT_MED, false );
	port_pin_set_output_level(LED_BAT_HI, false );

	//Three LEDs to indicate SoC, so 0-35, 35-70, 70-100.
	//Voltage thresholds:   
	port_pin_set_output_level(LED_BAT_LO, true );
	if (percent_soc > 35) {
		port_pin_set_output_level(LED_BAT_MED, true );
	}
	if (percent_soc>70) {
		port_pin_set_output_level(LED_BAT_HI, true );
	}
}
#endif

#if DYSON_VERSION == 10
void leds_flash_charging_segment(int percent_soc) {
	if (percent_soc <35) {	
		//Flash lo
		port_pin_set_output_level(LED_BAT_LO, true );
		delay_ms(500);
		port_pin_set_output_level(LED_BAT_LO, false );
		delay_ms(500);
	}
	else if (percent_soc<70) {
		//Low on, flash med
		port_pin_set_output_level(LED_BAT_LO, true );

		port_pin_set_output_level(LED_BAT_MED, true );
		delay_ms(500);
		port_pin_set_output_level(LED_BAT_MED, false );
		delay_ms(500);
	}
	else {
		//Low + med on, flash hi
		port_pin_set_output_level(LED_BAT_LO, true );
		port_pin_set_output_level(LED_BAT_MED, true );
		port_pin_set_output_level(LED_BAT_HI, true );
		delay_ms(500);
		port_pin_set_output_level(LED_BAT_HI, false );
		delay_ms(500);
	}
}
#endif

void leds_show_error(int ms) {
#if DYSON_VERSION == 10
		//Flash the error LED
		port_pin_set_output_level(LED_ERR, true );
		delay_ms(ms/2);
		port_pin_set_output_level(LED_ERR, false );
		delay_ms(ms/2);
#else
		//Alt flash L and R
		port_pin_set_output_level(LED_LEFT, true );
		delay_ms(ms/2);
		port_pin_set_output_level(LED_LEFT, false );
		port_pin_set_output_level(LED_RIGHT, true );
		delay_ms(ms/2);
		port_pin_set_output_level(LED_RIGHT, false );
#endif
}

void leds_show_pack_flat() {
#if DYSON_VERSION == 10
	//Blink the lowest battery segment LED
	for (int i=0; i<5; ++i) {
		port_pin_set_output_level(LED_BAT_LO, true );
		delay_ms(100);
		port_pin_set_output_level(LED_BAT_LO, false );
		delay_ms(100);
	}
#elif DYSON_VERSION == 11 
	//Three short flashes of both left and right LEDs
	for (int i=0; i<3; ++i) {
		port_pin_set_output_level(LED_LEFT, true );
		port_pin_set_output_level(LED_RIGHT, true );
		delay_ms(200);
		port_pin_set_output_level(LED_LEFT, false );
		port_pin_set_output_level(LED_RIGHT, false );
		delay_ms(200);
	}

#endif
}

#if DYSON_VERSION == 10
void leds_show_filter_err_status(bool status) {
	port_pin_set_output_level(LED_FILTER, status );
}

void leds_show_blocked_err_status(bool status) {
	port_pin_set_output_level(LED_BLOCKED, status);	
}
#endif