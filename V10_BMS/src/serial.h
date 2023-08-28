/*
 * serial.h
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

void serial_init(void);

size_t serial_get_next_block(uint8_t **);
void serial_send_next_message(void);
void serial_reset_message_counter(void);
