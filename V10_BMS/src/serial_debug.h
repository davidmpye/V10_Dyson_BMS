/*
 * serial_debug.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 


#ifndef SERIAL_DEBUG_H_
#define SERIAL_DEBUG_H_

#include "asf.h"
#include "config.h"

#include "bq7693.h"

void serial_debug_init(void);
void serial_debug_send_message(char *msg);

void serial_debug_send_cell_voltages();

extern char *debug_msg_buffer;
#endif /* SERIAL_DEBUG_H_ */