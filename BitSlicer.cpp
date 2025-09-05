#include "BitSlicer.h"

// 비트 삽입 함수 (MSB 우선)
void insertBits(uint32_t value, uint8_t bitLength, uint16_t startIndex) {
  for (int i = 0; i < bitLength; i++) {
    uint8_t bit = (value >> (bitLength - 1 - i)) & 0x01;
    uint16_t bitPos = startIndex + i;
    uint16_t byteIndex = bitPos / 8;
    uint8_t bitOffset = 7 - (bitPos % 8);

    if (byteIndex >= LOG_RECORD_SIZE) return; // 버퍼 오버플로우 방지

    if (bit)
      bitstream[byteIndex] |= (1 << bitOffset);
    else
      bitstream[byteIndex] &= ~(1 << bitOffset);
  }
}

// 애비오닉스용 - float 포인터로 직접 비트 추출
uint32_t compressFloatBits(float val, uint8_t fracBits) {
  const uint8_t* raw = (const uint8_t*)&val;

  // IEEE 754: raw[3]=MSB, raw[2], raw[1], raw[0]=LSB (Little Endian)
  uint32_t bits = ((uint32_t)raw[3] << 24) | 
                  ((uint32_t)raw[2] << 16) | 
                  ((uint32_t)raw[1] << 8)  | 
                  ((uint32_t)raw[0]);

  // sign(1) + exp(8) + frac 상위 N비트 추출
  uint32_t totalBits = 9 + fracBits;  // 1 + 8 + N
  return bits >> (32 - totalBits);
}

// 데이터 패킹 함수
void packSensorData() {
  // 비트스트림 초기화
  for (int i = 0; i < LOG_RECORD_SIZE; i++) bitstream[i] = 0;

  // 모든 데이터를 한 번에 패킹
  insertBits(millis(), LEN_BUF_TIME, START_BUF_TIME);
  insertBits(*(uint32_t*)&sensorData.pressure, LEN_BUF_PRESSURE, START_BUF_PRESSURE);
  insertBits(compressFloatBits(sensorData.altitude - initAlt, 4), LEN_BUF_DELTA_ALT, START_BUF_DELTA_ALT);

  insertBits(compressFloatBits(sensorData.acc_x, 4), LEN_BUF_ACCX, START_BUF_ACCX);
  insertBits(compressFloatBits(sensorData.acc_y, 4), LEN_BUF_ACCY, START_BUF_ACCY);
  insertBits(compressFloatBits(sensorData.acc_z, 4), LEN_BUF_ACCZ, START_BUF_ACCZ);

  insertBits(compressFloatBits(sensorData.gyr_x, 5), LEN_BUF_GYRX, START_BUF_GYRX);
  insertBits(compressFloatBits(sensorData.gyr_y, 5), LEN_BUF_GYRY, START_BUF_GYRY);
  insertBits(compressFloatBits(sensorData.gyr_z, 5), LEN_BUF_GYRZ, START_BUF_GYRZ);

  insertBits(compressFloatBits(sensorData.mag_x, 4), LEN_BUF_MAGX, START_BUF_MAGX);
  insertBits(compressFloatBits(sensorData.mag_y, 4), LEN_BUF_MAGY, START_BUF_MAGY);
  insertBits(compressFloatBits(sensorData.mag_z, 4), LEN_BUF_MAGZ, START_BUF_MAGZ);

  insertBits(FSM, LEN_BUF_FSM, START_BUF_FSM);
  insertBits(compressFloatBits(sensorData.head, 2), LEN_BUF_HEAD, START_BUF_HEAD);
  insertBits(compressFloatBits(sensorData.roll, 2), LEN_BUF_ROLL, START_BUF_ROLL);
  insertBits(compressFloatBits(sensorData.pitch, 2), LEN_BUF_PITCH, START_BUF_PITCH);

  dataReady = true;
}
