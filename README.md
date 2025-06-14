## 2025 NURA Preparation – ARES 전자팀  
🚀 성균관대학교 유일 항공우주·로켓 동아리 **ARES 전자팀**의 2025 여름 NURA 준비 기록  

---

### 🛰️ 소개  
이 레포지토리는 **ARES 전자팀**의 2025년 NURA 준비 과정을 정리한 공간입니다.    
전자 시스템 설계, 센서 연동, 데이터 로깅 등 비행체에 탑재될 **Avionics 모듈** 개발을 목표로 합니다.  
이번 로켓 설계는 전체적으로 **Quality Control (QC)** 에 목표를 두고 있습니다.  
전자팀은 다음을 계획하고 있습니다:  
**사출 판단 Redundancy**  
**통신 모듈 QC**  
**배터리 QC**  
**자이로 센서 QC**  
**RTOS**  
해당 레포지토리는 Redundancy와 RTOS 위주의 코드 업로드가 이루어질 예정입니다.  

---

### 📦 Board A  
**보드 A**는 `Arduino Nano 33 BLE Sense Rev2`를 사용합니다.  
자세한 사양은 아래 아두이노 공식 문서를 참고하세요:  
🔗 [Arduino Nano 33 BLE Sense Rev2 공식 문서](https://docs.arduino.cc/hardware/nano-33-ble-sense-rev2/)

---

#### mbed RTOS
보드 A의 Arduino Nano 33 BLE Sense Rev2 는 일반 아두이노와 다르게 mbedOS가 탑재되어 있습니다.  
일반 아두이노처럼 사용할 수 있지만 ARM에서 제공한 기능을 시험삼아 써보려고 합니다.  
다음 사이트에 mbedOS가 제공하는 커널과 RTOS api가 있습니다.  
https://os.mbed.com/docs/mbed-os/v6.16/mbed-os-api-doxy/group__rtos-public-api.html


