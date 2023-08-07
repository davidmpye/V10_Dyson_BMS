/*
 * bq7693.c
 *
 * Created: 06/07/2023 15:42:22
 *  Author: David Pye
 *  Licence: GNU GPL v3 or later

 */ 


#include "bq7693.h"

void bq7693_i2c_init(void);

//"internal" function primitives
int bq7693_read_block(uint8_t start_addr, size_t len, uint8_t* buf);
int bq7693_write_block(uint8_t start_addr, size_t len, uint8_t *buf);
uint8_t bq7693_calc_checksum(uint8_t inCrc, uint8_t data);

volatile uint16_t bq7693_cell_voltages[7];

volatile int bq7693_adc_gain = 0;   // in uV/LSB
volatile int bq7693_adc_offset = 0; //in mV

// maps for settings in chip protection registers

const int SCD_delay_setting [4] =
{ 70, 100, 200, 400 };

const int SCD_threshold_setting [8] =
{ 44, 67, 89, 111, 133, 155, 178, 200 }; // mV

const int OCD_delay_setting [8] =
{ 8, 20, 40, 80, 160, 320, 640, 1280 }; // ms
const int OCD_threshold_setting [16] =
{ 17, 22, 28, 33, 39, 44, 50, 56, 61, 67, 72, 78, 83, 89, 94, 100 };  // mV

const uint8_t UV_delay_setting [4] = { 1, 4, 8, 16 }; // s
const uint8_t OV_delay_setting [4] = { 1, 2, 4, 8 }; // s

struct i2c_master_module i2c_master_instance;
//Helper function for setting the pinmux
static inline void pin_set_peripheral_function(uint32_t pinmux) {
	uint8_t port = (uint8_t)((pinmux >> 16)/32);
	PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
	16) & 0x01u)));
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
	0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

void bq7693_i2c_init() {	
	//Enable I2C on SERCOM1 - pads PA16 + PA17
	pin_set_peripheral_function(PINMUX_PA16C_SERCOM1_PAD0);
	pin_set_peripheral_function(PINMUX_PA17C_SERCOM1_PAD1);
	
	//I2C peripheral init (from ASF example)
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	config_i2c_master.buffer_timeout = BQ7693_TIMEOUT;
	i2c_master_init(&i2c_master_instance, SERCOM1, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
}

void bq7693_init() {
	bq7693_i2c_init();
	bq7693_write_register(SYS_CTRL2, 0x00); //Ensure that charge/discharge FETs are off so pack is safe.
		
	//Read the ADC offset and gain values and store	
	uint8_t scratch1, scratch2;
	bq7693_read_register(ADCOFFSET, 1, &scratch1);  // convert from 2's complement
	bq7693_adc_offset = (signed int)scratch1;
	bq7693_read_register(ADCGAIN1, 1, &scratch1);
	bq7693_read_register(ADCGAIN2, 1, &scratch2);
	bq7693_adc_gain = 365 + ((( scratch1 & 0x0C) << 1) | (( scratch2 & 0xE0) >> 5)); // uV/LSB
	
	bq7693_write_register(PROTECT1, 0x82);
	bq7693_write_register(PROTECT2, 0x04);
	bq7693_write_register(PROTECT3, 0x00);
	
	bq7693_write_register(OV_TRIP, 0xAF); //need to reinterpret these to values...
	bq7693_write_register(UV_TRIP, 0xAF);
	
	bq7693_write_register(CELLBAL1, 0x00); //Disable cell balancing 1
	bq7693_write_register(CELLBAL2, 0x00); //Disable cell balancing 2
	bq7693_write_register(CC_CFG, 0x19); //'magic' value as per datasheet.
	bq7693_write_register(SYS_CTRL2, 0x40); //CC_EN - enable continuous operation of coulomb counter

	bq7693_write_register(SYS_CTRL1, 0x18); //ADC_EN, TEMP_SEL	
	
	bq7693_read_register(SYS_STAT, 1, &scratch1);
	bq7693_write_register(SYS_STAT, scratch1); //Explicitly clear any set bits in the SYS_STAT register by writing them back. 
}

bool bq7693_read_register(uint8_t addr, size_t len, uint8_t *buf) {
	uint16_t timeout = 0;
	bool result = true;
	
	//Initial write to set register address.
	struct i2c_master_packet packet = {
		.address = BQ7693_ADDR,
		.data_length = 1,
		.data = &addr
	};
	
	//Tx address
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == BQ7693_TIMEOUT) {
			break;
		}
	}
	//Rx value
	packet.data_length = len;
	packet.data = buf;
	
	while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == BQ7693_TIMEOUT) {
			result = false;
			break;
		}
	}
	return result;
}

bool bq7693_write_register(uint8_t addr, uint8_t value) {
	uint16_t timeout = 0;
	bool result = true;
	
	uint8_t buf[3];
	buf[0] = addr;
	buf[1] = value;
	
	//Calculate CRC (over slave address + rw bit, reg address + data.
	uint8_t crc = bq7693_calc_checksum(0x00, (BQ7693_ADDR <<1) | 0);
	crc = bq7693_calc_checksum(crc, buf[0]);
	crc = bq7693_calc_checksum(crc, buf[1]);
	buf[2] = crc;

	//Initial write to set register address.
	struct i2c_master_packet packet = {
		.address = BQ7693_ADDR,
		.data_length = 3,
		.data = buf
	};
	
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		/* Increment timeout counter and check if timed out. */
		if (timeout++ == BQ7693_TIMEOUT) {
			result = false;
			break;
		}
	}
	return result;
}

uint8_t bq7693_calc_checksum(uint8_t inCrc, uint8_t inData) {
    // CRC is calculated over the slave address (including R/W bit), register address, and data.
	uint8_t i;
	uint8_t data;
	data = inCrc ^ inData;
	for ( i = 0; i < 8; i++ ) {
		if (( data & 0x80 ) != 0 ) {
			data <<= 1;
			data ^= 0x07;
		}
		else data <<= 1;
	}	
	return data;
}

void bq7693_enable_charge() {
	bq7693_write_register(SYS_CTRL2, 0x41); //CC_EN, CHG_ON
}

void bq7693_disable_charge() {
	bq7693_write_register(SYS_CTRL2, 0x40); //CC_EN, CHG_ON = 0
}

void bq7693_enable_discharge() {
	bq7693_write_register(SYS_CTRL2, 0x40);  //CC_EN=1
	bq7693_write_register(SYS_CTRL1, 0x18);  //ADC_EN=1, TEMP_SEL=1
	
	bq7693_write_register(PROTECT1, 0x9F); 
	bq7693_write_register(PROTECT2, 0x04);

	bq7693_write_register(SYS_STAT, 0x80);  
	
	bq7693_write_register(SYS_CTRL2, 0x42);//CC_EN, DSG_ON
	bq7693_write_register(PROTECT2, 0x04);
	bq7693_write_register(PROTECT1, 0x82);
	bq7693_write_register(SYS_STAT, 0x80);
}

void bq7693_disable_discharge() {
	bq7693_write_register(SYS_CTRL2, 0x40);//CC_EN, DSG_OFF
}

int bq7693_read_temperature() {
	//Returns 'C * 10 eg 217 = 21.7'C
	
	volatile uint8_t scratch[3];
 	volatile int adcVal = 0;

	volatile unsigned long  rts;
	volatile int vtsx;
	volatile float tmp;
	bq7693_read_register(TS2_HI_BYTE, 3, scratch);

	adcVal =  ((scratch[0]&0x3F)<<8);
	adcVal |= scratch[2]; //ignore the unwanted CRC byte.
		
	// calculate R_thermistor according to bq769x0 datasheet
	vtsx = adcVal * 0.382; // mV
	rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
	 
	// Temperature calculation using Beta equation
	// - According to bq769x0 datasheet, only 10k thermistors should be used
	// - 25°C reference temperature for Beta equation assumed
    tmp = 1.0/(1.0/(273.15+25) + 1.0/3435 *log(rts/10000.0)); // K
	volatile int result = (tmp - 273.15)* 10;
	return result;
}

volatile uint16_t *bq7693_get_cell_voltages() {
	volatile uint8_t scratch[3];
	volatile uint16_t tempval;
	//Voltages for each cell
	//The cells are connected as below on these packs...
	int cellsToRead[] = { 0,1,2,3,5,6,9};
	for (int i=0; i< 7; ++i) {
		//Because CRC is enabled, we need to read 3 bytes (VCx_HI, the CRC byte (ignore), then VCx_Lo)
		bq7693_read_register((VC1_HI_BYTE + 2*cellsToRead[i]), 3, scratch);
		tempval = ((scratch[0] & 0x3F) <<8) | scratch[2];
		bq7693_cell_voltages[i] = tempval * bq7693_adc_gain/1000 + bq7693_adc_offset;
	}

	return bq7693_cell_voltages;
}

volatile int bq7693_get_pack_voltage() {
	volatile uint8_t scratch[3];
	volatile uint16_t tempval;
	bq7693_read_register(BAT_HI_BYTE, 3, scratch);
	tempval = scratch[0] <<8 | scratch[2];
	int bq7693_pack_voltage = 4 * bq7693_adc_gain * tempval / 1000 + ( 7 * bq7693_adc_offset);
	return bq7693_pack_voltage;
}

void bq7693_enter_sleep_mode() {
	bq7693_write_register(SYS_CTRL1, 0x00);
	bq7693_write_register(SYS_CTRL1, 0x01);
	bq7693_write_register(SYS_CTRL1, 0x02);
}

//charge counter notes 
/*

uint8_t reg;
float batCurrent;
if (reg &  0x80 ) {
	uint16_t adcVal;
	bq7693_read_register(0x32, 1, &reg);
	adcVal  = (reg)<<8;
	bq7693_read_register(0x33, 1, &reg);
	adcVal |= reg;
	batCurrent = adcVal * 8.44;
	
}
else batCurrent = 0;
}
*/