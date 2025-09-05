## 2025 NURA Preparation – ARES 전자팀  
🚀 성균관대학교 유일 항공우주·로켓 동아리 **ARES 전자팀**의 2025 여름 NURA 준비 기록  

---

### 🛰️ 소개  
이 레포지토리는 **ARES 전자팀**의 2025년 NURA 준비 과정을 정리한 공간입니다.    
전자 시스템 설계, 센서 연동, 데이터 로깅 등 비행체에 탑재될 **Avionics 모듈** 개발을 목표로 합니다.  

- 전자팀의 에비오닉스 코드 내용은 다음과 같습니다:  
  - Arduino Nano + BNO055/BMP280/microSD/Relay
  - Sensor: BNO055 (IMU 9 axis) / BMP280 (Barometer)
  - Data Logger: Micro SD card module
  - Actuator: Relay
    
해당 레포지토리는 Redundancy에 대해 깊게 다루지 않습니다.

센서 데이터 읽기 및 제어, 스케줄링 위주의 코드 업로드가 이루어질 예정입니다. 

---
### Cyclic Executive 
- 비선점형, 정적 스케줄링 방식
- loop에서 호출하는 메인동작은 태스크를 관리합니다. 태스크는 각각 재호출 대기시간과 우선순위가 정해져있습니다.

  - Examples
      - 

### Bitmasking & FSM (SystemState)
- FSM 기반의 조건 분기 및 제어를 합니다.
- state를 비트마스킹으로 toggle하여 제어합니다. 상태를 나타내는 변수는 1바이트입니다.

### Bit Slicing
- float로 선언된 센서 데이터들(자세 IMU, 고도 등)의 mantissa(유효숫자, 가수)를 원하는 자릿수만큼만 살립니다.
- SD 기록 시 저장 데이터를 버퍼로 동기화하여 flush 합니다. 원하는 데이터만 slice했기에 버퍼의 크기가 32byte를 넘기지 않습니다.
- LSB (Least Significant Bit)로 다뤄지는 센서 데이터를 SD 기록용 버퍼에 넣기 위해 MSB (Most Significant Bit) 형태로 변환합니다.
- 전체 코드 앞부분의 #define으로 선언된 비트 크기는 slice된 센서 데이터들이 할당될 크기이며, 하드코딩된 값입니다.





