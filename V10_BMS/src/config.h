/*
 * config.h
 *
 * Created: 06/07/2023 15:43:48
 *  Author: David Pye
 */ 

#ifndef CONFIG_H_
#define CONFIG_H_

// Pin definitions
#define LED_FILTER PIN_PA01
#define LED_BLOCKED PIN_PA00
#define LED_ERR PIN_PA19

//Battery charge/discharge indicators
#define LED_BAT_LO PIN_PA25
#define LED_BAT_MED PIN_PA24
#define LED_BAT_HI PIN_PA18

//- seems to go to Q3 on the charger inlet side of things, push it high to accept a charge
#define ENABLE_CHARGE_PIN PIN_PA02 

//PA28 appears to be ALERT pin from the BQ7693
#define BQ7693_ALERT_PIN PIN_PA28

 //NB Goes high when trigger pulled, but don't have it pulled up or down..
#define TRIGGER_PRESSED_PIN PIN_PA04
//Goes high when charger plugged in.
#define CHARGER_CONNECTED_PIN PIN_PA06

//PA07 is attached to thermistor RT1

//These are connected to the capacitors C13 & C43 next to the red/black wire serial connector
//BLACK_DIFF_SIG_PIN PIN_PA14 
//RED_DIFF_SIG_PIN PIN_PA15
//No need to #define them as they are just used as part of SERCOM2 as the USART.

//PA07 appears to go to the comparator 2nd pin down (IN-?) 
//Thermistor RT2 is connected as TS2 via the BQ7693

#define CELL_LOWEST_DISCHARGE_VOLTAGE 3200 //mV - wont allow pack to be used if any cells lower than this
#define CELL_LOWEST_CHARGE_VOLTAGE 2000 //mV - won't try to charge the pack if any cells lower than this
#define CELL_FULL_CHARGE_VOLTAGE 4150 //mV - .
 
#endif /* CONFIG_H_ */