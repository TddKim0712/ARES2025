//SystemState.h
#pragma once
#include <Arduino.h>

using ms_t = uint32_t;
using TaskFn = void(*)();
using byte = uint8_t; 

struct SensorData {
    uint32_t pressure;
    float altitude;
    float head, roll, pitch;
    float gyr_x, gyr_y, gyr_z;
    float acc_x, acc_y, acc_z;
    float mag_x, mag_y, mag_z;
    uint8_t FSM;
};

extern SensorData sensorData;
extern float initAlt;
// extern float initHead;
// extern float initPitch, initRoll;
extern float maxAlt;

// FSM 플래그 정의 및 유틸
#define FSM_SETUP_DONE        0x01
#define FSM_ALT_REACHED       0x02
#define FSM_LORA_RECEIVED     0x04
#define FSM_ALT_APOGEE        0x08
#define FSM_HEAD_DIVERTED     0x10
#define FSM_EMERGENCY_DETECTED 0x20
#define FSM_DEPLOY_TRIGGERED  0x40

extern volatile uint8_t FSM;
void setFSMFlag(uint8_t flag);
void clearFSMFlag(uint8_t flag);
bool isFSMSet(uint8_t flag);

