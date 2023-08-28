/**
 * main.c - entry point
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */

#include "bms.h"

int main(void) {
	bms_init();
	bms_mainloop();
	//never returns.
}