/*
 * leds.h
 *
 * Created: 06/07/2023 16:58:01
 *  Author: David Pye
 */ 


#ifndef LEDS_H_
#define LEDS_H_

#include "asf.h"
#include "config.h"

void leds_init(void);
void leds_sequence(void);
void leds_display_battery_voltage(int);
void leds_flash_charging_segment(int);
void leds_blink_error_led(int);
void leds_show_pack_flat(void);

void leds_off(void);
void leds_on(void);

#endif /* LEDS_H_ */