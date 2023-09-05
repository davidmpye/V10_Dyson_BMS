/*
 * serial.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#include "serial_debug.h"

/* This is a bit of info about the serial protocol, which I don't fully understand.

There are variable length messages, of which the 21 byte long messages seem to contain filter/blocked status.

If the message sequence number (byte 8) is 3 or 6, and byte 0x0F is 0x01, then FILTER ERROR has occurred.
If the message sequence number (byte 8) is 4 or 7, and byte 0x0F is 0x01, then BLOCKED ERROR has occurred.
 
If the byte 0x0F is 0x00, then the error state is OK.
*/

#define SERIAL_MSG_DELIM_CHAR 0x12
#define MSG_NUM_OFFSET 0x08
#define MSG_ERR_CODE_OFFSET 0x0F

void serial_init(void);

size_t serial_get_next_block(uint8_t **);
void serial_send_next_message(void);
void serial_reset_message_counter(void);
