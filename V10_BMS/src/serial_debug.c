/*
 * serial_debug.c
 *
 *  Author:  David Pye
 *  Contact: davidmpye@gmail.com
 *  Licence: GNU GPL v3 or later
 */ 

#include "serial_debug.h"
#ifdef SERIAL_DEBUG
struct usart_module debug_usart;
#include <string.h>
#endif

char debug_buffer[80];
char *debug_msg_buffer = debug_buffer;

static inline void pin_set_peripheral_function(uint32_t pinmux) {
	uint8_t port = (uint8_t)((pinmux >> 16)/32);
	PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
	16) & 0x01u)));
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
	0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

void serial_debug_init() {

#ifdef SERIAL_DEBUG
	//Set up the pinmux settings for SERCOM0
	pin_set_peripheral_function(PINMUX_PA11C_SERCOM0_PAD3);
	pin_set_peripheral_function(PINMUX_PA10C_SERCOM0_PAD2);
		
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
		
	//Load the necessary settings into the config struct.
	config_usart.baudrate    = 115200;
	config_usart.mux_setting =  USART_RX_3_TX_2_XCK_3 ;
	config_usart.parity = USART_PARITY_NONE;
	config_usart.pinmux_pad2 = PINMUX_PA10C_SERCOM0_PAD2;
	config_usart.pinmux_pad3 = PINMUX_PA11C_SERCOM0_PAD3;
		
	//Init the UART
	while (usart_init(&debug_usart,
		SERCOM0, &config_usart) != STATUS_OK) {
	}
	//Enable
	usart_enable(&debug_usart);
	
	//Initial debug blurb
	serial_debug_send_message("Dyson V10 BMS Aftermarket firmware init\r\n");
	serial_debug_send_message("(C) David Pye davidmpye@gmail.com\r\n");
	serial_debug_send_message("GNU GPL v3.0 or later\r\n");
	//Need to pause 250mS before cell voltages are available from the BQ7693
	delay_ms(250);
	serial_debug_send_cell_voltages();
#endif

}

void serial_debug_send_message(char *msg) {
	
#ifdef SERIAL_DEBUG
	int result = usart_write_buffer_wait(&debug_usart, msg, strlen(msg));
#else
	return 0;
#endif

}

void serial_debug_send_cell_voltages() {
#ifdef SERIAL_DEBUG
	uint16_t *cell_voltages = bq7693_get_cell_voltages();
	serial_debug_send_message("Pack cell voltages:\r\n");
	for (int i=0; i<7; ++i) {
		sprintf(debug_msg_buffer, "Cell %d: %d mV, min %d mV, max %d mV\r\n", i, cell_voltages[i], CELL_LOWEST_DISCHARGE_VOLTAGE, CELL_FULL_CHARGE_VOLTAGE);
		serial_debug_send_message(debug_msg_buffer);
	}
#endif
}
