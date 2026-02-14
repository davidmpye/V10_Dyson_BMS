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

//We can do these regardless of HW version (but in different ways)
void leds_init(void);
void leds_sequence(void);
void leds_show_pack_flat(void);
void leds_off(void);
void leds_on(void);
void leds_show_error(int);

//We can only do these on V10 as later ones lack LEDs
#if DYSON_VERSION == 10
void leds_display_battery_soc(int);
void leds_flash_charging_segment(int);
void leds_show_filter_err_status(bool);
void leds_show_blocked_err_status(bool);
#endif

#endif /* LEDS_H_ */