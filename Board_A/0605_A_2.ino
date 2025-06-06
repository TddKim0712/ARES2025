//not containing RTOS, normal setup-loop based code
//Arduino Nano 33 BLE sense, used for ARES 2025 NURA Redundancy #1 MCU
//embedded sensors: barometer, IMU (6 axis acc, gyr + 3 axis mag)
//0605_김유찬 작성, 코드 A 형태, FSM 수정함, 센서호출 및 비상하강 알고리즘 수정 필요
// 시간조건: 출발 이후로부터 watchdog 타이머 (6초? 수정필요) --> 추진팀 문의 필요
// 고도조건: 최고 고도로부터 5미터 이상 강하 (카운트 3번 이상)
// 자세조건: 자이로센서의 헤드 부호가 뒤집힘 (처음 init될때의 부호랑 30도 이상으로 5번 카운트, 자세 각도 관련해서 수정필요) --> 추진, 동체 문의 필요
// 반대편 아두이노: 자세조건 || 고도미갱신1초
/*
전원인가 -> init 단계 (초기고도 설정) -> 발사 감지 -> 안전고도 체크 -> 메인 시퀀스  (현재 작성된 코드)
나중에는 state들을 비트필드로 바꿔서 쉽게 연산하기

!! 발사감지를 초기에 진행하면서 동시에 (가속도 변화 또는 고도 변화로) 비상상황 체크하는 것도 검토 필요 (코드 B로 분리?)
예시) init 단계 (센서 초기화) -> (발사감지 = 비상상황 체크)
*/

//코드 내부 cnt 오류 해결 할것!
//deploycontrol 검토 필요

#include <Arduino_LPS22HB.h>              // 압력 센서
#include <Arduino_BMI270_BMM150.h>        // 가속도+자이로+자기장 센서
#include <math.h>

#define SAFETY_ALT      100.0  
#define LAUNCH_THRESH    2.0
#define RELAY_PIN      7


enum State {
  INIT, LAUNCH_DETECT, SAFETY_ALT_CHECK, MAIN_LOGIC, DEPLOY , EMERGENCY
};
volatile State currentState = INIT;

// 로깅용 변수
float curAltitude = 0.0;
float baseAltitude = 0.0;
float deltaAltitude = 0.0;

float acc_x, acc_y, acc_z;
float gyr_x, gyr_y, gyr_z;

// 상태 플래그
bool altInitialized = false;
bool launched = false;
bool safetyPassed = false;

uint8_t relayControlStartTime = 0;

// 로깅 함수, FSM 기반 알고리즘, State 바뀔때마다 로깅.
void logStateChange(State s) {
  Serial.print("[STATE] → ");
  switch (s) {
    case INIT: Serial.println("INIT"); break;
    case LAUNCH_DETECT: Serial.println("LAUNCH_DETECT"); break;
    case SAFETY_ALT_CHECK: Serial.println("SAFETY_ALT_CHECK"); break;
    case MAIN_LOGIC: Serial.println("MAIN_LOGIC"); break;
   
    case EMERGENCY: Serial.println("EMERGENCY"); break;
  }
}

void logEvent(const char* msg) {
  Serial.print("[EVENT] ");
  Serial.println(msg);
}

void readSensors() { // 센서값읽기, 가속도자이로지자계 + 고도
  if (IMU.accelerationAvailable()) IMU.readAcceleration(acc_x, acc_y, acc_z);
  if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gyr_x, gyr_y, gyr_z);
  curAltitude = 44330 * (1 - pow(BARO.readPressure() / 101.0, 1.0 / 5.0));
  deltaAltitude = curAltitude - baseAltitude;
}

void InitAlt() { // const int samples -> 고정된 샘플 추출 횟수, 이걸로 평균 내서 
  float pressureSum = 0;
  const uint8_t samples = 10;
  for (uint8_t i = 0; i < samples; i++) {
    delay(2);
    pressureSum += BARO.readPressure();
  }
  float avgPressure = pressureSum / samples;
  baseAltitude = 44330 * (1 - pow(avgPressure / 101.0, 1.0 / 5.0));
  altInitialized = true;
  logEvent("Base Altitude Initialized");
}

void LaunchDetect(const uint8_t count_threshold) {
  uint8_t cnt = 0;
  if (deltaAltitude > LAUNCH_THRESH) {
    cnt ++;
    if (cnt >= count_threshold){
    launched = true;
    logEvent("Launch Detected!");
    currentState = SAFETY_ALT_CHECK;
    logStateChange(currentState);
    }
  }
}

void CheckSafetyAlt(const uint8_t count_threshold) {
   uint8_t cnt = 0;
  if (deltaAltitude > SAFETY_ALT) {
    cnt++;
     if (cnt >= count_threshold){
    safetyPassed = true;
    logEvent("Safety Altitude Passed!");
    currentState = MAIN_LOGIC;
    logStateChange(currentState);
     }
  }
}

void MainLogic() {


}

void deployControl(const uint8_t relayClosingSec){ // 릴레이를 몇 초간 닫을 건지 (사출이 완료될 경우 계속 회로 닫을 필요 없음)
  uint8_t deltaTime = millis();
  if ((millis() - relayControlStartTime) <= relayClosingSec){
    digitalWrite(RELAY_PIN, HIGH);
  }
  
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);

  if (!BARO.begin()) {
    Serial.println("Barometer init failed!");
    while (1);
  }
  if (!IMU.begin()) {
    Serial.println("IMU init failed!");
    while (1);
  }

  logStateChange(currentState);
}

void loop() { // SD 기록 관련 없음 아직...
START:
  readSensors();
  // writeSD();
  switch (currentState) {
    case INIT:
      if (!altInitialized) InitAlt();  // 초기고도 읽는 상태인지?  초기고도 세팅 :  
      currentState = LAUNCH_DETECT;
      logStateChange(currentState);
      break;

    case LAUNCH_DETECT:
      if (!launched){ 
        LaunchDetect();
        goto START;
            }
      break;

    case SAFETY_ALT_CHECK:
      if (!safetyPassed){
         CheckSafetyAlt();
        if (!LoraSignalReceived){
          
        }
        // check lora module rx, goto deploy
        goto START;
      }
      break;

    case MAIN_LOGIC:
      MainLogic();
      break;
    case EMERGENCY: // emergency대신 앞 부분에서 launch 체크하면서 따짐, goto deploy

      goto DEPLOY;
      break;
  }
DEPLOY:
  deployControl(3000);

  // 주기적으로 상태 출력
  Serial.print("Alt: "); Serial.print(curAltitude);
  Serial.print(" | ΔAlt: "); Serial.print(deltaAltitude);
  Serial.print(" | AccX: "); Serial.print(acc_x);
  Serial.print(" | GyrZ: "); Serial.println(gyr_z);

  delay(2);  
}
