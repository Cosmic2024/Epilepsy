/*
 * ============================================
 *   EPILEPSY SEIZURE DETECTOR - COMPLETE
 *   With All Sensors + SMS Emergency Alerts
 * ============================================
 *   
 *   Components:
 *   - OLED SSD1306 Display (I2C)
 *   - MPU6050 Gyroscope + Accelerometer (I2C)
 *   - ADXL345 Accelerometer (I2C)
 *   - GSR Sensor (Analog)
 *   - GPS NEO-6M (Serial - 9600 baud)
 *   - SIM800L GSM Module (Serial - 9600 baud)
 *   
 *   Detection Logic:
 *   - GSR elevated (sweating) +
 *   - Accelerometer OR Gyroscope over threshold
 *   = SEIZURE ALERT → Send SMS
 * ============================================
 */

#include <Wire.h>
#include <U8g2lib.h>
#include <MPU6050_light.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <TinyGPSPlus.h>

// ========= Pin Definitions =========
// I2C Pins (Shared by OLED, MPU6050, ADXL345)
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;

// GSR Sensor
constexpr int GSR_PIN = 34;

// GPS NEO-6M
constexpr int GPS_RX_PIN = 16;
constexpr int GPS_TX_PIN = 17;

// SIM800L GSM
constexpr int SIM800_RX_PIN = 27;  // ESP32 RX ← SIM800L TX
constexpr int SIM800_TX_PIN = 14;  // ESP32 TX → SIM800L RX

// ========= OLED Display =========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// ========= MPU6050 =========
MPU6050 mpu(Wire);
bool mpuOK = false;

// ========= ADXL345 =========
Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(12345);
bool adxlOK = false;

// ========= GPS NEO-6M =========
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);

// ========= SIM800L GSM =========
HardwareSerial SerialSIM(1);

// ========= Detection Thresholds =========
constexpr int   GSR_THRESHOLD      = 700;     // Raw ADC (higher = more sweat)
constexpr float ACC_THRESHOLD_G    = 1.20f;   // Acceleration in g
constexpr float GYRO_THRESHOLD_DPS = 120.0f;  // Rotation in °/s

// ========= Emergency Contact =========
String emergencyNumber = "+918675265597";  // ⚠️ CHANGE THIS!

// ========= Timing =========
uint32_t lastDisplay = 0;
constexpr uint32_t DISPLAY_INTERVAL = 150;
uint32_t lastSMSTime = 0;
constexpr uint32_t SMS_COOLDOWN = 60000;  // 1 minute between SMS

// ========= Sensor Values =========
float mpu_ax, mpu_ay, mpu_az;
float mpu_gx, mpu_gy, mpu_gz;
float adxl_ax, adxl_ay, adxl_az;
float combined_ax, combined_ay, combined_az;
int gsrValue;
int satellites;
bool gpsFix;
char timeIST[16];
double latitude, longitude;

// ========= Detection State =========
bool seizureDetected = false;
bool smsSent = false;
uint32_t detectionTime = 0;
constexpr uint32_t ALERT_DURATION_MS = 5000;

// ========= Function Prototypes =========
void readAllSensors();
void updateGPSTime();
bool checkSeizureCondition();
void drawNormalScreen();
void drawAlertScreen();
void printSerial();
void sendSMS(String number, String message);
bool sendATCommand(String cmd, uint32_t timeout, String expectedResponse);

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("═══════════════════════════════════════════════════");
  Serial.println("   EPILEPSY SEIZURE DETECTOR - COMPLETE SYSTEM     ");
  Serial.println("   With SMS Emergency Alerts                       ");
  Serial.println("═══════════════════════════════════════════════════");
  
  // ========= Initialize I2C =========
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  
  // ========= Initialize OLED =========
  Serial.println("Initializing OLED...");
  if (u8g2.begin()) {
    Serial.println("✅ OLED OK");
  } else {
    Serial.println("❌ OLED FAILED");
  }
  
  // Show startup screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub14_tf);
  u8g2.drawStr(10, 25, "SEIZURE");
  u8g2.drawStr(20, 50, "DETECTOR");
  u8g2.sendBuffer();
  delay(1000);
  
  // ========= Initialize MPU6050 =========
  Serial.println("Initializing MPU6050...");
  if (mpu.begin() == 0) {
    mpuOK = true;
    Serial.println("✅ MPU6050 OK");
    delay(500);
    mpu.calcOffsets(true, true);
    Serial.println("✅ MPU6050 Calibrated");
  } else {
    Serial.println("❌ MPU6050 FAILED");
  }
  
  // ========= Initialize ADXL345 =========
  Serial.println("Initializing ADXL345...");
  adxlOK = adxl.begin();
  if (adxlOK) {
    adxl.setRange(ADXL345_RANGE_2_G);
    Serial.println("✅ ADXL345 OK");
  } else {
    Serial.println("❌ ADXL345 FAILED");
  }
  
  // ========= Initialize GPS =========
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("✅ GPS NEO-6M Started (9600 baud)");
  
  // ========= Initialize GSR =========
  pinMode(GSR_PIN, INPUT);
  Serial.println("✅ GSR Sensor Ready");
  
  // ========= Initialize SIM800L =========
  Serial.println("Initializing SIM800L...");
  SerialSIM.begin(9600, SERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);
  delay(2000);
  
  // Check SIM800L communication
  if (sendATCommand("AT", 2000, "OK")) {
    Serial.println("✅ SIM800L OK");
    
    // Set SMS to text mode
    if (sendATCommand("AT+CMGF=1", 2000, "OK")) {
      Serial.println("✅ SMS Text Mode Set");
    }
    
    // Check SIM card
    if (sendATCommand("AT+CPIN?", 2000, "READY")) {
      Serial.println("✅ SIM Card Ready");
    }
    
    // Check network
    sendATCommand("AT+CREG?", 3000, "");
    sendATCommand("AT+CSQ", 2000, "");
  } else {
    Serial.println("❌ SIM800L Not responding");
  }
  
  // ========= Show Configuration =========
  Serial.println("────────────────────────────────────");
  Serial.println("Detection Thresholds:");
  Serial.print("  GSR:       > ");
  Serial.println(GSR_THRESHOLD);
  Serial.print("  Accel:     > ");
  Serial.print(ACC_THRESHOLD_G);
  Serial.println(" g");
  Serial.print("  Gyro:      > ");
  Serial.print(GYRO_THRESHOLD_DPS);
  Serial.println(" °/s");
  Serial.print("  Emergency: ");
  Serial.println(emergencyNumber);
  Serial.println("────────────────────────────────────");
  
  Serial.println("═══════════════════════════════════════════════════");
  Serial.println("System Ready! Monitoring for seizures...");
  Serial.println("═══════════════════════════════════════════════════");
  
  // Show ready screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub11_tf);
  u8g2.drawStr(25, 20, "SYSTEM");
  u8g2.drawStr(25, 38, "READY!");
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(10, 58, "Monitoring...");
  u8g2.sendBuffer();
  delay(1500);
}

void loop() {
  // ========= Read GPS Data =========
  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
  }
  
  // ========= Read SIM800L Data =========
  while (SerialSIM.available()) {
    char c = SerialSIM.read();
    Serial.write(c);
  }
  
  // ========= Read All Sensors =========
  readAllSensors();
  
  // ========= Check for Seizure =========
  if (checkSeizureCondition()) {
    if (!seizureDetected) {
      seizureDetected = true;
      detectionTime = millis();
      smsSent = false;
      Serial.println("\n⚠️ ⚠️ ⚠️  SEIZURE DETECTED!  ⚠️ ⚠️ ⚠️");
    }
  }
  
  // ========= Send SMS Alert =========
  if (seizureDetected && !smsSent && (millis() - lastSMSTime > SMS_COOLDOWN)) {
    sendEmergencySMS();
    smsSent = true;
    lastSMSTime = millis();
  }
  
  // ========= Clear Alert After Duration =========
  if (seizureDetected && (millis() - detectionTime > ALERT_DURATION_MS)) {
    seizureDetected = false;
  }
  
  // ========= Update Display =========
  if (millis() - lastDisplay >= DISPLAY_INTERVAL) {
    lastDisplay = millis();
    
    if (seizureDetected) {
      drawAlertScreen();
    } else {
      drawNormalScreen();
    }
    
    printSerial();
  }
  
  delay(10);
}

// ========= Read All Sensors =========
void readAllSensors() {
  // Read MPU6050
  if (mpuOK) {
    mpu.update();
    mpu_ax = mpu.getAccX();
    mpu_ay = mpu.getAccY();
    mpu_az = mpu.getAccZ();
    mpu_gx = mpu.getGyroX();
    mpu_gy = mpu.getGyroY();
    mpu_gz = mpu.getGyroZ();
  }
  
  // Read ADXL345
  if (adxlOK) {
    sensors_event_t event;
    adxl.getEvent(&event);
    const float g = 9.80665f;
    adxl_ax = event.acceleration.x / g;
    adxl_ay = event.acceleration.y / g;
    adxl_az = event.acceleration.z / g;
  }
  
  // Combine accelerometers (average)
  if (mpuOK && adxlOK) {
    combined_ax = (mpu_ax + adxl_ax) / 2.0f;
    combined_ay = (mpu_ay + adxl_ay) / 2.0f;
    combined_az = (mpu_az + adxl_az) / 2.0f;
  } else if (mpuOK) {
    combined_ax = mpu_ax;
    combined_ay = mpu_ay;
    combined_az = mpu_az;
  } else if (adxlOK) {
    combined_ax = adxl_ax;
    combined_ay = adxl_ay;
    combined_az = adxl_az;
  }
  
  // Read GSR
  gsrValue = analogRead(GSR_PIN);
  
  // Update GPS info
  gpsFix = gps.location.isValid();
  satellites = gps.satellites.value();
  
  if (gpsFix) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }
  
  updateGPSTime();
}

// ========= Update GPS Time =========
void updateGPSTime() {
  if (gps.time.isValid()) {
    int hh = gps.time.hour();
    int mm = gps.time.minute();
    int ss = gps.time.second();
    // Convert UTC to IST (add 5 hours 30 minutes)
    long total = hh * 3600L + mm * 60L + ss + 5 * 3600L + 30 * 60L;
    total = (total % 86400L + 86400L) % 86400L;
    snprintf(timeIST, sizeof(timeIST), "%02d:%02d:%02d", 
             (int)(total / 3600), (int)((total % 3600) / 60), (int)(total % 60));
  } else {
    uint32_t s = millis() / 1000;
    snprintf(timeIST, sizeof(timeIST), "%02lu:%02lu:%02lu", s/3600, (s/60)%60, s%60);
  }
}

// ========= Seizure Detection Logic =========
bool checkSeizureCondition() {
  // Check GSR threshold
  bool gsrHigh = (gsrValue > GSR_THRESHOLD);
  
  // Check Accelerometer threshold (any axis)
  bool accelHigh = (fabs(combined_ax) > ACC_THRESHOLD_G) ||
                   (fabs(combined_ay) > ACC_THRESHOLD_G) ||
                   (fabs(combined_az) > ACC_THRESHOLD_G);
  
  // Check Gyroscope threshold (any axis)
  bool gyroHigh = (fabs(mpu_gx) > GYRO_THRESHOLD_DPS) ||
                  (fabs(mpu_gy) > GYRO_THRESHOLD_DPS) ||
                  (fabs(mpu_gz) > GYRO_THRESHOLD_DPS);
  
  // Seizure detected if: GSR high AND (accel high OR gyro high)
  return gsrHigh && (accelHigh || gyroHigh);
}

// ========= Draw Normal Screen =========
void drawNormalScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  
  char buf[32];
  
  // Line 1: Time + GPS Status
  snprintf(buf, sizeof(buf), "IST %s  GPS:%c", timeIST, gpsFix ? 'Y' : 'N');
  u8g2.drawStr(0, 10, buf);
  
  // Line 2: GSR + Satellites
  snprintf(buf, sizeof(buf), "GSR:%4d  Sats:%d", gsrValue, satellites);
  u8g2.drawStr(0, 24, buf);
  
  // Line 3: Accelerometer
  snprintf(buf, sizeof(buf), "Accel:%.1f %.1f %.1f", combined_ax, combined_ay, combined_az);
  u8g2.drawStr(0, 38, buf);
  
  // Line 4: Gyroscope
  snprintf(buf, sizeof(buf), "Gyro:%.0f %.0f %.0f", mpu_gx, mpu_gy, mpu_gz);
  u8g2.drawStr(0, 52, buf);
  
  // Line 5: Status
  if (gsrValue > GSR_THRESHOLD) {
    u8g2.drawStr(0, 64, "GSR HIGH!");
  } else {
    u8g2.drawStr(0, 64, "Monitoring...");
  }
  
  u8g2.sendBuffer();
}

// ========= Draw Alert Screen =========
void drawAlertScreen() {
  u8g2.clearBuffer();
  
  // Big alert message
  u8g2.setFont(u8g2_font_fub14_tf);
  u8g2.drawStr(5, 20, "SEIZURE");
  u8g2.drawStr(5, 42, "DETECTED!");
  
  // SMS status
  u8g2.setFont(u8g2_font_6x12_tf);
  if (smsSent) {
    u8g2.drawStr(0, 58, "SMS Sent!");
  } else {
    u8g2.drawStr(0, 58, "Sending SMS...");
  }
  
  u8g2.sendBuffer();
}

// ========= Print Serial Output =========
void printSerial() {
  Serial.print("IST: ");
  Serial.print(timeIST);
  Serial.print(" | GPS: ");
  Serial.print(gpsFix ? "FIX" : "NO FIX");
  Serial.print(" | Sats: ");
  Serial.print(satellites);
  Serial.print(" | GSR: ");
  Serial.print(gsrValue);
  Serial.print(gsrValue > GSR_THRESHOLD ? " [HIGH]" : "");
  Serial.print(" | Accel: ");
  Serial.print(combined_ax, 2);
  Serial.print(",");
  Serial.print(combined_ay, 2);
  Serial.print(",");
  Serial.print(combined_az, 2);
  Serial.print(" | Gyro: ");
  Serial.print(mpu_gx, 0);
  Serial.print(",");
  Serial.print(mpu_gy, 0);
  Serial.print(",");
  Serial.println(mpu_gz, 0);
  
  if (seizureDetected) {
    Serial.println(">>> ⚠️ SEIZURE ALERT ACTIVE ⚠️ <<<");
    if (smsSent) {
      Serial.println(">>> SMS Alert Sent to: " + emergencyNumber);
    }
  }
}

// ========= Send Emergency SMS =========
void sendEmergencySMS() {
  Serial.println("\n═══════════════════════════════════════════════════");
  Serial.println("SENDING EMERGENCY SMS...");
  Serial.println("═══════════════════════════════════════════════════");
  
  // Build message
  String message = "SEIZURE DETECTED!\n";
  message += "Time: " + String(timeIST) + " IST\n";
  
  if (gpsFix) {
    message += "Location: ";
    message += String(latitude, 6) + "," + String(longitude, 6);
    message += "\nhttps://maps.google.com/?q=";
    message += String(latitude, 6) + "," + String(longitude, 6);
  } else {
    message += "Location: No GPS Fix";
  }
  
  Serial.println("To: " + emergencyNumber);
  Serial.println("Message:");
  Serial.println(message);
  Serial.println("────────────────────────────────────");
  
  sendSMS(emergencyNumber, message);
  
  Serial.println("═══════════════════════════════════════════════════");
}

// ========= Send SMS Function =========
void sendSMS(String number, String message) {
  // Set text mode
  SerialSIM.println("AT+CMGF=1");
  delay(500);
  
  // Clear buffer
  while (SerialSIM.available()) {
    SerialSIM.read();
  }
  
  // Set recipient
  SerialSIM.print("AT+CMGS=\"");
  SerialSIM.print(number);
  SerialSIM.println("\"");
  delay(500);
  
  // Clear buffer
  while (SerialSIM.available()) {
    SerialSIM.read();
  }
  
  // Send message
  SerialSIM.print(message);
  delay(500);
  
  // Send Ctrl+Z (ASCII 26)
  SerialSIM.write(26);
  
  Serial.println("SMS sending... Please wait...");
  
  // Wait for response
  delay(5000);
  
  String response = "";
  while (SerialSIM.available()) {
    response += (char)SerialSIM.read();
  }
  
  if (response.indexOf("OK") >= 0 || response.indexOf("+CMGS") >= 0) {
    Serial.println("✅ SMS SENT SUCCESSFULLY!");
  } else if (response.indexOf("ERROR") >= 0) {
    Serial.println("❌ SMS FAILED!");
    Serial.println("Response: " + response);
  } else {
    Serial.println("SMS status unknown. Response: " + response);
  }
}

// ========= Send AT Command =========
bool sendATCommand(String cmd, uint32_t timeout, String expectedResponse) {
  Serial.print(">>> ");
  Serial.println(cmd);
  SerialSIM.println(cmd);
  
  String response = "";
  uint32_t startTime = millis();
  
  while (millis() - startTime < timeout) {
    while (SerialSIM.available()) {
      response += (char)SerialSIM.read();
    }
  }
  
  Serial.print("<<< ");
  Serial.println(response);
  
  if (expectedResponse.length() > 0) {
    return (response.indexOf(expectedResponse) >= 0);
  }
  return true;
}