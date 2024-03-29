/*
 * leds.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 


#ifndef LEDS_H_
#define LEDS_H_

#include "asf.h"
#include "config.h"

void leds_init(void);
void leds_sequence(void);
void leds_display_battery_soc(int);
void leds_flash_charging_segment(int);
void leds_blink_error_led(int);
void leds_show_pack_flat(void);


void leds_show_filter_err_status(bool);
void leds_show_blocked_err_status(bool);

void leds_off(void);
void leds_on(void);

#endif /* LEDS_H_ */