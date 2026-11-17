// Pin Definitions file for Nucleo F401RE

#ifndef pindefs_h
#define pindefs_h

#include "STM32_CAN.h"

//Define your pins like this. Make sure to prefix them with PIN_
// N.B. These are commented out, and here for illustrative purposes only
// delete them when you've defined your own pins.
// #define PIN_BRAKE PC2
// #define PIN_CHANGE_DIRECTION PA4
// #define PIN_BRAKE_RELEASE PC0
// #define PIN_HORN PA1
// #define PIN_AUTOSTOP PB0
// #define PIN_REGEN PC1
// #define PIN_DSD_DETECT PA0
// #define PIN_JS_SIG PC3
 
// Not ever used, noted here only for reference.
#define PIN_CAN_P_TD PA12
#define PIN_CAN_P_RD PA11
#define PIN_CAN_V_TD PB13
#define PIN_CAN_V_RD PB12

#define PIN_CAN_V_PORT CAN1
#define PIN_CAN_V_PINS DEF

#define PIN_CAN_VESC_PORT CAN2
#define PIN_CAN_VESC_PINS DEF

// These pins are to allow for a digital logic analyser to
// time your loop execution times without spamming Serial.Print
#define PIN_PRIMARY_TASK_TRACE PB4
#define PIN_SECONDARY_TASK_TRACE PB5

#define PIN_HEARTBEAT_LED LED_BUILTIN

#endif
