#ifndef VESC_PROTO_DEFS_H
#define VESC_PROTO_DEFS_H

#include <cstdint>

#define VESC_STATUS_1 0x09
#define VESC_STATUS_2 0x0E
#define VESC_STATUS_3 0x0F
#define VESC_STATUS_4 0x10
#define VESC_STATUS_5 0x1B
#define VESC_STATUS_6 0x3A

struct VescStatus
{
    unsigned long lastContactMillis = 0;
    bool seenBefore = false;
    int32_t rpm = 0;
    float current = 0.0f;
    float dutyCycle = 0.0f;
    float ahConsumed = 0.0f;
    float ahCharged = 0.0f;
    float whConsumed = 0.0f;
    float whCharged = 0.0f;
    float mosfetTemp = 0.0f;
    float motorTemp = 0.0f;
    float inputCurrent = 0.0f;
    float pidPosition = 0.0f;
    int32_t tachoRel = 0;
    float voltageIn = 0.0f;
    int16_t adc1millivolts = 0;
    int16_t adc2millivolts = 0;
    int16_t adc3millivolts = 0;
    int16_t servoPosition = 0;
};


#endif