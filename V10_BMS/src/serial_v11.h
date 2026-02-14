/*
 * serial_v11.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#include "serial_debug.h"

#if DYSON_VER == 11

#define SERIAL_MSG_DELIM_CHAR 0x12
#define MSG_NUM_OFFSET 0x08
#define MSG_ERR_CODE_OFFSET 0x0F

void serial_init(void);

#endif