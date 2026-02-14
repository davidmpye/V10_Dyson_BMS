/*
 * crc_v11.1
 *
 *  Author:  David Pye + credits to follow!
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 
#include "crc_v11.h"


#if DYSON_VER == 11

//0x63 message crc table
const uint8_t matrix_0x63_hi[] = {
    0x0e, 0x36, 0x12, 0x72, 0x30, 0x6c, 0x0c, 0x68, 0x5e, 0x40, 0x54, 0x24, 0x48, 0x62, 0x6e, 0x06
};
const uint16_t const_0x63_hi = 0xf993;

const uint8_t matrix_0x63_lo[] = {
    0x5c, 0x40, 0x7a, 0x52, 0x24, 0x30, 0x62, 0x44, 0x54, 0x52, 0x24, 0x32, 0x42, 0x20, 0x38, 0x2e
};
const uint16_t const_0x63_lo = 0x98ef;

//0x82 message crc table
const uint8_t matrix_0x82_hi[] = {
    0x10, 0x1a, 0x3a, 0x1e, 0x3e, 0x30, 0x3e, 0x16, 0x14, 0x38, 0x06, 0x28, 0x24, 0x0a, 0x04, 0x06
};
const uint16_t const_0x82_hi = 0xcc64;
const uint8_t matrix_0x82_lo[] = {
    0x34, 0x28, 0x26, 0x0e, 0x1c, 0x0e, 0x1c, 0x38, 0x04, 0x0a, 0x20, 0x02, 0x32, 0x12, 0x26, 0x3a
};
const uint16_t const_0x82_lo = 0xcdbd;

//0x53 message crc table
const uint8_t matrix_0x53_hi[] = {
    0x1c, 0x26, 0x52, 0x24, 0x48, 0x70, 0x62, 0x44, 0x16, 0x4c, 0x64, 0x28, 0x52, 0x5a, 0x56, 0x30
};
const uint16_t const_0x53_hi  = 0x1c6d;
const uint16_t lo_0x53_init = 0xF69C; //NB no matrix table needed here, this is just the init.

//Calculate the double CRC for the message header+payload (Unescaped, no 0x12 delims)
uint32_t calculate_message_crc(uint8_t* msg, size_t len) {
    //Parse the message header
    uint16_t init_lo, init_hi;
    uint16_t msg_len = (msg[0] | msg[1]<<8) -1;
    uint8_t seq = msg[7];
    uint8_t cmd = msg[2];

    //Generate the correct CRC16 init values based on message type and sequence
    switch (cmd) {
        case 0x53:
            init_hi = init_from_seq(seq, matrix_0x53_hi, const_0x53_hi);
            init_lo = lo_0x53_init; //This one doesn't use a matrix table, just a constant. Why?
            break;
        case 0x63:
            init_hi = init_from_seq(seq, matrix_0x63_hi, const_0x63_hi);
            init_lo = init_from_seq(seq, matrix_0x63_lo, const_0x63_lo);
            break;
        case 0x82:
            init_hi = init_from_seq(seq, matrix_0x82_hi, const_0x82_hi);
            init_lo = init_from_seq(seq, matrix_0x82_lo, const_0x82_lo);
            break;
        default:
            //Need to flag this as an error
            break;
    };
    //Generate the higher 2 crc bytes.
    uint16_t crc_hi = crc16(msg, msg_len, init_hi);
    //Generate the first part of low crc based on message
    uint16_t crc_lo = crc16(msg, msg_len, init_lo);
    //Now recalculate the low crc to include the high crc bytes
    crc_lo = crc16((uint8_t*)&crc_hi, 2, crc_lo);
    return crc_hi<<16 | crc_lo;
}

uint16_t crc16(uint8_t *data, size_t len, uint16_t init) {
    uint16_t crc = init;
    //CRC-16 Dyson — poly 0xC9A7 (reflected 0xE593)
    for (size_t i=0; i<len; ++i) {
        crc ^= data[i];
        for (int j=0; j<8; ++j) {
            if (crc & 0x01) 
                crc = (crc >> 1) ^ 0xE593;
            else 
                crc = crc >> 1;
        }
    }
    return crc & 0xFFFF;    
}

uint16_t init_from_seq(uint8_t seq, uint8_t *matrix_table, uint16_t constant) {
    //Compute CRC16 init from seq byte via GF(2) matrix multiply.
    //Each row of the 16x8 matrix produces one bit of the 16-bit init
    uint16_t result = 0x00000000;
    for (int i=0; i<16; ++i) {
        bool bit = 0;
        for (int j=0; j<8; ++j) {
            bit ^= (matrix_table[i] >> j) & 0x01 & ((seq >> j) & 1);
        }
        if (bit) {
            result |= 0x01 << i;
        }
    }
    return result ^ constant;
}

#endif