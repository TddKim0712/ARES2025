//Cyclic Executive (Non-Preemptive Priority Scheduler)
//SD 관련 수정한 보드 B, 거의 실전 직전 이므로 수정 불필요
//쿼터니언 기반 자세 판단 추가

// 테스트 용 파라미터: 고도 카운트, 우선순위, 쿨타임 검토 필요
#include <Wire.h>
#include <SD.h>
#include "DFRobot_BNO055.h"
#include "DFRobot_BMP280.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "SystemState.h"
#include "Scheduler.h"

// ───── 센서 객체 선언 ─────
static DFRobot_BNO055_IIC bno(&Wire, 0x28);
static DFRobot_BMP280_IIC bmp(&Wire, 0x00);
SoftwareSerial loraSerial(2, -1); // RX, TX   // 로라 핀 확인 필요

// ───── 핀/상수 정의 ─────
#define RELAY_PIN 9
#define SD_CS_PIN 10
#define LORA_BAUD 9600

#define FSM_SETUP_DONE        0x01
#define FSM_ALT_REACHED       0x02
#define FSM_LORA_RECEIVED     0x04
#define FSM_ALT_APOGEE        0x08
#define FSM_HEAD_DIVERTED     0x10
#define FSM_EMERGENCY_DETECTED 0x20
#define FSM_DEPLOY_TRIGGERED  0x40

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

using ms_t = uint32_t;
using byte = uint8_t;
using TaskFn = void(*)();

File logFile;
uint8_t bitstream[LOG_RECORD_SIZE];
bool dataReady = false;  // 데이터가 준비되었는지 확인하는 플래그

// 비트 삽입 함수 (MSB 우선) - FIXED
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

DFRobot_BNO055_IIC::sAxisAnalog_t magData;
DFRobot_BNO055_IIC::sAxisAnalog_t gyrData;
DFRobot_BNO055_IIC::sAxisAnalog_t liaData;
//DFRobot_BNO055_IIC::sEulAnalog_t e;

// ────── 데이터 패킹 함수 (새로 추가) ──────
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

// ────── 태스크 정의 ──────
void task_readBMP() {
    sensorData.pressure = bmp.getPressure();
    sensorData.altitude = 44330.0 * (1 - pow((float)sensorData.pressure / 101325.0, 0.1903));
      
    if (sensorData.altitude > maxAlt) maxAlt = sensorData.altitude;
}

void task_readBNO() {
    magData = bno.getAxis(DFRobot_BNO055_IIC::eAxisMag);
    gyrData = bno.getAxis(DFRobot_BNO055_IIC::eAxisGyr);
    liaData = bno.getAxis(DFRobot_BNO055_IIC::eAxisLia);
    
    // 쿼터니언 기반 오일러 각도 계산 (로깅용)
    DFRobot_BNO055::sQuaAnalog_t quat = bno.getQua();
    quatToEuler(quat.w, quat.x, -quat.y, -quat.z, sensorData.roll, sensorData.pitch, sensorData.head);

    // 나머지 센서 데이터
    sensorData.acc_x = liaData.x; 
    sensorData.acc_y = liaData.y;  
    sensorData.acc_z = liaData.z; 
    sensorData.gyr_x = gyrData.x; 
    sensorData.gyr_y = gyrData.y; 
    sensorData.gyr_z = gyrData.z;
    sensorData.mag_x = magData.x; 
    sensorData.mag_y = magData.y;
    sensorData.mag_z = magData.z;
    
    // 센서 데이터를 읽은 후 패킹
    packSensorData();
}

void task_checkApogee() {
    static uint8_t apogeeCounter = 0;

    if (!isFSMSet(FSM_ALT_REACHED)) {
        if (sensorData.altitude - initAlt >= 12.0f) {   // 실험용, 20이상으로 안전고도 바꿀 것
            setFSMFlag(FSM_ALT_REACHED);
        }
        return;
    }

    if (sensorData.altitude > maxAlt) {
        maxAlt = sensorData.altitude;
        apogeeCounter = 0;
    } else {
        if (++apogeeCounter >= 30) {
            setFSMFlag(FSM_ALT_APOGEE);
        }
        Serial.println("chkapogee");
    }

    // // 하강 감지 기록
    // if (isFSMSet(FSM_ALT_REACHED) && (sensorData.altitude < (initAlt - 5.0f))) {
    //     // EEPROMLogger.logDescent(sensorData.altitude);
    // }
}

// ────── 쿼터니언 유틸리티 함수들 ──────
void quatToEuler(float w, float x, float y, float z, float& roll, float& pitch, float& yaw) {
    roll = atan2(2.0f*(w*x + y*z), 1.0f - 2.0f*(x*x + y*y));
    float sinp = 2.0f*(w*y - z*x);
    if (abs(sinp) >= 1) 
        pitch = copysign(M_PI/2, sinp);
    else 
        pitch = asin(sinp);
    yaw = atan2(2.0f*(w*z + x*y), 1.0f - 2.0f*(y*y + z*z));
    
    // 라디안 → 도
    roll *= 57.29578f;
    pitch *= 57.29578f;
    yaw *= 57.29578f;
}

float getRocketPitch(float qw, float qx, float qy, float qz) {
    // 로켓 Y축의 Z성분: 2*(qw*qy - qz*qx)
    float wz = 2.0f * (qw * qy - qz * qx);
    return asin(wz) * 57.29578f;  // 라디안 → 도
}

// ────── 쿼터니언 기반 자세 판단 (수정됨) ──────
void task_checkHead() {
    static uint8_t tiltCounter = 0;
    
    if (!isFSMSet(FSM_HEAD_DIVERTED)) {
        DFRobot_BNO055::sQuaAnalog_t quat = bno.getQua();
        
        // 수정된 좌표계 매핑: quatToEuler(qw, qx, -qy, -qz, ...)와 일치
        float pitch = getRocketPitch(quat.w, quat.x, -quat.y, -quat.z);
        
        if (pitch <= -5.0f) {
            if (++tiltCounter > 10) {
                setFSMFlag(FSM_HEAD_DIVERTED);
                Serial.println(F(">>> ROCKET TILTED! <<<"));
            }
        } else {
            tiltCounter = 0;
        }
    }
}

void task_readLORA() {
    static uint8_t loraCount = 0;
    while (loraSerial.available()) {
        char c = loraSerial.read();
        if (c == 'A') {
            loraCount++;
            if (loraCount >= 2) {
                setFSMFlag(FSM_LORA_RECEIVED);
            }
        }
    }
}

void task_deploy() {
    if (!isFSMSet(FSM_DEPLOY_TRIGGERED) && 
       ((isFSMSet(FSM_ALT_APOGEE) && ((isFSMSet(FSM_ALT_REACHED)) || isFSMSet(FSM_HEAD_DIVERTED)) || isFSMSet(FSM_LORA_RECEIVED) ))) {
        digitalWrite(RELAY_PIN, HIGH);        // 사출사출사출사출
        setFSMFlag(FSM_DEPLOY_TRIGGERED);
        Serial.println(F(">>> DEPLOYED <<<"));
        // EEPROMLogger.logDeploy(millis());
    }
}

static unsigned long lastCloseTime = 0;

void task_flushSD() {
    if (!dataReady){

       return;  // 데이터가 준비되지 않았으면 리턴
    }
    // SD카드에 바이너리 데이터 기록
    for (int bytePos = 0; bytePos < LOG_RECORD_SIZE; bytePos++) {
        logFile.write(bitstream[bytePos]);
    }

    logFile.flush();
    if (millis() - lastCloseTime > 3000) {  // 3초마다
        logFile.close();
       // Serial.println("SD closed.");
               lastCloseTime = millis() + 30;
        delay(30); 
        logFile = SD.open("log.bin", FILE_WRITE);

    }
    dataReady = false;  // 데이터 사용 완료 표시
}

void task_printSerial() {
    // 쿼터니언 기반 정확한 피치각 계산 (사출 판단용)
    DFRobot_BNO055::sQuaAnalog_t quat = bno.getQua();
    float quatPitch = getRocketPitch(quat.w, quat.x, -quat.y, -quat.z);
    
    Serial.print(F(" A=")); Serial.print(sensorData.altitude, 1);
   // Serial.print(F(" Pitch=")); Serial.print(quatPitch, 1);      // 쿼터니언 피치 (사출 판단용)
    Serial.print(F(" Head=")); Serial.print(sensorData.head, 1);  // 쿼터니언 기반 헤딩 (로깅용)
    Serial.print(F(" Roll=")); Serial.print(sensorData.roll, 1);  // 쿼터니언 기반 롤 (로깅용)
    Serial.print(F(" Pitch=")); Serial.print(sensorData.pitch, 1); // 쿼터니언 기반 피치 (로깅용)
  //  Serial.print(F(" Gx=")); Serial.print(sensorData.gyr_x, 1);
   // Serial.print(F" Gy="); Serial.print(sensorData.gyr_y, 1);
   // Serial.print(F" Gz="); Serial.print(sensorData.gyr_z, 1);
    Serial.print(F(" FSM=")); Serial.println(FSM, BIN);
    Serial.print(F("initalt = ")); Serial.println(initAlt);
}

// ────── 초기화 ──────
void setup() {
    Serial.begin(115200);
    while (!Serial);
     //  부저 - 짧고 조용하게
    tone(3, 900, 1000);     // D3, 900Hz, 1000ms (자동 종료)
    delay(100);            // 여유시간
    pinMode(3, OUTPUT);    // 부저 핀 완전히 LOW로 고정
    digitalWrite(3, LOW);
    delay(200);
    Wire.begin();
    Serial.println(F("buzzer off"));
   

    pinMode(SD_CS_PIN, OUTPUT);
    if (SD.begin(SD_CS_PIN)) {
        if (SD.exists("log.bin")) {
            logFile = SD.open("log.bin", FILE_WRITE);
            logFile.seek(logFile.size());
        } else {
            logFile = SD.open("log.bin", FILE_WRITE);
        }
    } else {
        Serial.println(F("SD err")); 
        
    }

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    if (bno.begin() != DFRobot_BNO055_IIC::eStatusOK) {
        Serial.println(F("BNO err")); 
        return;
    }
    bno.setOprMode(DFRobot_BNO055_IIC::eOprModeNdof);

    if (bmp.begin() != DFRobot_BMP280_IIC::eStatusOK) {
        Serial.println(F("BMP err")); 
        return;
    }
    bmp.setCtrlMeasSamplingPress(DFRobot_BMP280_IIC::eSampling_X4);
    bmp.setCtrlMeasSamplingTemp(DFRobot_BMP280_IIC::eSampling_X2);
    bmp.setConfigFilter(DFRobot_BMP280_IIC::eConfigFilter_X4);

    // 초기화 대기
    ms_t initTime = millis();
    Serial.println(F("initializing.."));

    while (millis() - initTime < 1000) {
        delay(400);
    }

    // 초기값 캘리브레이션 (고도만)
    float sumAlt = 0.0f;
    for (int i = 0; i < 6; i++) {
        sumAlt += 44330.0 * (1 - pow((float)bmp.getPressure() / 101325.0, 0.1903));
        delay(1);
    }
    initAlt = sumAlt / 6.0f;
    maxAlt = initAlt;
    setFSMFlag(FSM_SETUP_DONE);

     loraSerial.begin(LORA_BAUD);

    // 태스크 등록
    registerTask(task_readBMP,        15, 3, true);
    registerTask(task_readBNO,        15, 3, true);
    registerTask(task_checkApogee,    100, 1, true);
    registerTask(task_readLORA,       100, 5, true);
    registerTask(task_deploy,        200, 0, true);
    registerTask(task_flushSD,        45, 3, true);
    registerTask(task_checkHead,      60, 2, true);   // 쿼터니언 기반 자세 판단
    registerTask(task_printSerial,   300, 4, true);
}

// ────── 루프 ──────
void loop() {
    scheduleLoop();
    delay(1);
}