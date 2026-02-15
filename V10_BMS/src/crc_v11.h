/*
 * crc_v11.h
 *
 *  Author:  David Pye + credits to follow!
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#ifndef CRC_V11_
#define CRC_V11_

#include <asf.h>
#include "config.h"

#if DYSON_VERSION == 11

uint16_t init_from_seq(uint8_t seq, uint8_t *matrix_table, uint16_t constant);
uint16_t crc16(uint8_t *data, size_t len, uint16_t init);
uint32_t calculate_message_crc(uint8_t* msg, size_t len);


#endif
#endif
