//not containing RTOS, normal setup-loop based code
//Arduino Nano 33 BLE sense, used for ARES 2025 NURA Redundancy #1 MCU
//embedded sensors: barometer, IMU (6 axis acc, gyr + 3 axis mag)

#include <Arduino_LPS22HB.h> // barometer library
#include "Arduino_BMI270_BMM150.h" // gyro, accelaration
#include <math.h>

#define DATABUFFERSIZE 255
#define RELAYSIGNAL1 7
#define RELAYSIGNAL2 8 //임의로 정한 릴레이 핀

//비트필드: 구조체로 선언한 2바이트 이진수덩어리,
/*
01010101 01010101 -> 이걸 덩어리로 본다
*/
typedef struct FLAGBIT {
  unsigned char ALT_INITIALIZED : 1; //0, 1
  unsigned char ROCKET_LAUNCHED : 1; //0, 1
  unsigned char SATETY_ALT_PASSED : 1; // 0, 1
  unsigned char GYRO_INVERTED : 1; // 0, 1
  unsigned char RF_SIGNAL_GOT : 1;  // 0,1
  unsigned char ALT_CDN1_PASSED : 1; // 0, 1
  unsigned char ALT_CDN2_PASSED : 1; // 0, 1
  unsigned char DEPLOYED : 1; // 0, 1

} FLAGBIT;

typedef struct CDNBIT{

} CDNBIT;

bool isAltInit = false;
float InitAlt = 0.0;



void setInitAlt(uint8_t Time, bool& refAltInit, float& returnInitAlt){ // 초기 고도값 측정하는 함수, 처음 3 번 건너뛰고 횟수만큼 진행, (횟수, 초기고도측정플래그, 초기고도반환)
  if(refAltInit){ return;}
  float AltSum = 0.0;
    
  for (uint8_t i = 0; i< Time+3; i++){
    AltSum += BARO.readPressure();
    if (i < 3){AltSum = 0;}
  }
  returnInitAlt = AltSum / Time;
  refAltInit = true;
}

void SafetyAltCHK()

void setup() {
     Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");
  if (!BARO.begin()){
    Serial.println("Failed to initialize Baro!");
    
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
  
  }
  
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
  Serial.println("X\tY\tZ");

   Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in G's");
  Serial.println("X\tY\tZ");
  
}

void loop() {
    float gyr_x, gyr_y, gyr_z;
    float acc_x, acc_y, acc_z;
    float mag_x, mag_y, mag_z;
  ///
   
    float curAlt = 0.0 ; //currentAltitude 
  

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gyr_x, gyr_y, gyr_z);
    Serial.print("Gyr: ")
    Serial.print(gyr_x);
    Serial.print('\t');
    Serial.print(gyr_y);
    Serial.print('\t');
    Serial.println(gyr_z);
  }
 
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(acc_x, acc_y, acc_z);

    Serial.print(acc_x);
    Serial.print('\t');
    Serial.print(acc_y);
    Serial.print('\t');
    Serial.println(acc_z);
  }
 

  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mag_x, mag_y, mag_z);

    Serial.print(mag_x);
    Serial.print('\t');
    Serial.print(mag_y);
    Serial.print('\t');
    Serial.println(mag_z);
  }

  
  curAlt = 44330 * ( 1 - pow(pressure/101.325, 1/5.255) );
  // print the sensor value
  Serial.print("Altitude according to kPa is = ");
  Serial.print(curAlt);
  Serial.println(" m");

  delay(500);
}
