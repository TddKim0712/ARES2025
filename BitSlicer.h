#pragma once
#include <Arduino.h>
#include "SystemState.h"   // sensorData, FSM 등 사용


#define LOG_RECORD_SIZE 32

#define START_BUF_TIME     0
#define START_BUF_PRESSURE 32
#define START_BUF_DELTA_ALT 64
#define START_BUF_ACCX     77
#define START_BUF_ACCY     90
#define START_BUF_ACCZ     103
#define START_BUF_GYRX     116
#define START_BUF_GYRY     130
#define START_BUF_GYRZ     144
#define START_BUF_MAGX     158
#define START_BUF_MAGY     171
#define START_BUF_MAGZ     184
#define START_BUF_FSM      197
#define START_BUF_HEAD     205
#define START_BUF_ROLL     216
#define START_BUF_PITCH    227

#define LEN_BUF_TIME     32
#define LEN_BUF_PRESSURE 32
#define LEN_BUF_DELTA_ALT 13
#define LEN_BUF_ACCX     13
#define LEN_BUF_ACCY     13
#define LEN_BUF_ACCZ     13
#define LEN_BUF_GYRX     14
#define LEN_BUF_GYRY     14
#define LEN_BUF_GYRZ     14
#define LEN_BUF_MAGX     13
#define LEN_BUF_MAGY     13
#define LEN_BUF_MAGZ     13
#define LEN_BUF_FSM      8
#define LEN_BUF_HEAD     11
#define LEN_BUF_ROLL     11
#define LEN_BUF_PITCH    11

// 전역 버퍼는 그대로 ino에서 선언
extern uint8_t bitstream[LOG_RECORD_SIZE];
extern bool dataReady;

// 함수 원형
void insertBits(uint32_t value, uint8_t bitLength, uint16_t startIndex);
uint32_t compressFloatBits(float val, uint8_t fracBits);
void packSensorData();
