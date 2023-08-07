/*
 * serial.h
 *
 * Created: 12/07/2023 18:02:35
 *  Author: David Pye
 */ 

void serial_init(void);

size_t serial_get_next_block(uint8_t **);
void serial_send_next_message(void);
void serial_reset_message_counter(void);
