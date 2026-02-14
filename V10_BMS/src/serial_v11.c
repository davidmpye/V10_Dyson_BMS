/*
 * serial_v11.c
 *
 *  Author:  David Pye (and others from issue thread on github!)
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#if DYSON_VERSION == 11


#include "asf.h"
#include "serial_v11.h"

#include "leds.h" //fixme

struct usart_module usart_instance;

static inline void pin_set_peripheral_function(uint32_t pinmux) {
	uint8_t port = (uint8_t)((pinmux >> 16)/32);
	PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
	16) & 0x01u)));
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
	0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

//Data is read into here via a callback triggered by usart_read_buffer_job
uint8_t serial_rx_buf[128];
size_t serial_rx_buf_index = 0;

typedef enum SERIAL_RX_STATE {
	AWAITING_START,
	IN_MSG,
	MSG_COMPLETE,
	ESCAPE_CHAR_FOUND,
} SERIAL_RX_STATE;

//NB the state machine could also be used to handle escaped delimiters?

SERIAL_RX_STATE rx_state = AWAITING_START;
uint8_t rx_msg_buf[256];
size_t msg_len = 0;

uint8_t scratch_buf;

//This function is called by the interrupt handler when a byte has been received.
void usart_read_callback(struct usart_module *const usart_module) {
	//Handle special bytes first- delims and escape character should not end up in message itself
	switch (scratch_buf) {
		case SERIAL_MSG_DELIM_CHAR: //delim
			if (rx_state == AWAITING_START) 
				rx_state = IN_MSG; //this is start of message
			else if (rx_state = IN_MSG) 
				rx_state = MSG_COMPLETE;
			break;
		case 0xDB: //escape char
			rx_state = ESCAPE_CHAR_FOUND;
			break;
		default:
			if (rx_state == ESCAPE_CHAR_FOUND) {
				//We've now hit the 2nd half of the escape sequence, so turn it back into the 'correct' char
				switch (scratch_buf) {
					case 0xDD:
						scratch_buf = 0xDB;
						break;
					case 0xDE:
						scratch_buf = 0x12;
						break;
					default:
						//Unknown... we don't think anything else is ever escaped
						char msg_buf[28];
						sprintf(msg_buf, "Unexpected esc char 0x%02X", scratch_buf);
						serial_debug_send_message(msg_buf);
				}
				//Reset the state now escape char found
				rx_state = IN_MSG;
			}
			//Copy the scratch buffer character into the message
			rx_msg_buf[msg_len] = scratch_buf;
			msg_len++;
	}
	
	if (rx_state == MSG_COMPLETE) {
		//Print the message as hex to the debug output
		char msg_buf[msg_len * 2  + 1];
		for (size_t i=0; i<msg_len; ++i) {
			sprintf(msg_buf, "02X", rx_msg_buf[i]);
		}
		//For test purposes, dump all the messages received to console for now
		serial_debug_send_message("Message received from cleaner:");
		serial_debug_send_message(msg_buf);
		serial_debug_send_message("\n");	
		//FIXME - Need to generate the message reply here.....

		//Reset the state machine/buffer index to receive new message
		msg_len = 0;
		rx_state = AWAITING_START;
	} 
	else {
		//Prepare for next byte
		if (msg_len == 255) {
			serial_debug_send_message("Serial rx buffer overflow occurred\n");
			msg_len = 0;
		}
	}
	//Prepare to receive next byte
	usart_read_buffer_job(&usart_instance, &scratch_buf, 1);
}

void serial_init() {	
	//Set up the pinmux settings for SERCOM2
	pin_set_peripheral_function(PINMUX_PA14C_SERCOM2_PAD2);
	pin_set_peripheral_function(PINMUX_PA15C_SERCOM2_PAD3);
	
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	
	//Load the necessary settings into the config struct.
	config_usart.baudrate    = 115200;
	config_usart.mux_setting =  USART_RX_3_TX_2_XCK_3 ;
	config_usart.parity = USART_PARITY_NONE;
	config_usart.pinmux_pad2 = PINMUX_PA14C_SERCOM2_PAD2;
	config_usart.pinmux_pad3 = PINMUX_PA15C_SERCOM2_PAD3;
	
	//Init the UART
	while (usart_init(&usart_instance,
		SERCOM2, &config_usart) != STATUS_OK) {
	}
	
	//Enable a callback for bytes received.
	usart_register_callback(&usart_instance, usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
	
	usart_enable(&usart_instance);
	//Start read job - first byte to start of buffer
	usart_read_buffer_job(&usart_instance, &scratch_buf, 1);
}


#endif