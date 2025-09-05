// SystemState.cpp
// 시스템의 상태 관련 변수 저장
#include "SystemState.h"

// 전역 변수 정의
SensorData sensorData;
float maxAlt = 0;
float initAlt = 0;
// float initHead = 0;
// float initRoll = 0;
// float initPitch = 0;

// 비트플래그 FSM
volatile uint8_t FSM = 0;

void setFSMFlag(uint8_t flag)   { FSM |= flag; }
void clearFSMFlag(uint8_t flag) { FSM &= ~flag; }
bool isFSMSet(uint8_t flag)     { return FSM & flag; }
