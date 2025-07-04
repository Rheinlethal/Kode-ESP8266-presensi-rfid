#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi credentials
#define WIFI_SSID "GKBB1"
#define WIFI_PASSWORD "dutasayangrara"

// Firebase configuration - menggunakan REST API langsung
#define FIREBASE_PROJECT_ID "rfid-attendance-system-acb13"
#define FIREBASE_DATABASE_URL "https://rfid-attendance-system-acb13-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY "AIzaSyCHsie4V1BsTx07tI2uYpFvAfZn2L5wFD4"

// Pin definitions untuk RFID RC522
#define SS_PIN 4    // Pin SDA/SS (D2)
#define RST_PIN 5   // Pin RST (D1)

// Inisialisasi objects
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000); // GMT+7 untuk Indonesia
WiFiClientSecure client;
HTTPClient http;

// Variables
String lastUID = "";
unsigned long lastReadTime = 0;
bool rfidWorking = false;
bool wifiConnected = false;
bool firebaseConnected = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RFID FIREBASE ATTENDANCE SYSTEM ===");
  
  // Inisialisasi I2C untuk LCD
  Wire.begin(2, 0); // SDA = D4, SCL = D3
  
  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RFID Attendance");
  lcd.setCursor(0, 1);
  lcd.print("System Starting");
  
  delay(2000);
  
  // Inisialisasi WiFi
  setupWiFi();
  
  // Inisialisasi Firebase
  setupFirebase();
  
  // Inisialisasi NTP
if (wifiConnected) {
  timeClient.begin();

  Serial.print("Syncing time");
  int retries = 0;
  while (!timeClient.update() && retries < 10) {
    Serial.print(".");
    delay(500);
    retries++;
  }

  if (timeClient.isTimeSet()) {
    Serial.println("\n Time synced: " + getFormattedTime());
  } else {
    Serial.println("\n Time sync failed!");
  }
}

Serial.print("Epoch time: ");
Serial.println(timeClient.getEpochTime());

  
  // Inisialisasi SPI dan RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Test koneksi RFID
  setupRFID();
  
  // Tampilkan status akhir
  displayStatus();
  
  Serial.println("=== SISTEM SIAP DIGUNAKAN ===");
  printConnectionInfo();
}

void setupWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Update LCD dengan progress
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi: Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi: FAILED");
    lcd.setCursor(0, 1);
    lcd.print("Check Network");
    delay(2000);
  }
}

void setupFirebase() {
  if (!wifiConnected) {
    firebaseConnected = false;
    Serial.println("Cannot setup Firebase - WiFi not connected");
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing");
  lcd.setCursor(0, 1);
  lcd.print("Firebase...");
  
  // Set SSL client untuk HTTPS
  client.setInsecure(); // Untuk development, di production sebaiknya gunakan sertifikat
  
  // Test koneksi Firebase dengan HTTP request sederhana
  String testUrl = String(FIREBASE_DATABASE_URL) + "/test.json?auth=" + FIREBASE_API_KEY;
  
  http.begin(client, testUrl);
  http.addHeader("Content-Type", "application/json");
  
  String testData = "\"connected_" + String(millis()) + "\"";
  int httpResponseCode = http.PUT(testData);
  
  Serial.print("Firebase test HTTP Response: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Firebase response: ");
    Serial.println(response);
    
    if (httpResponseCode == 200) {
      firebaseConnected = true;
      Serial.println("Firebase connected successfully!");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Firebase: OK");
      delay(2000);
    } else {
      firebaseConnected = false;
      Serial.println("Firebase test failed - HTTP error");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Firebase: ERROR");
      lcd.setCursor(0, 1);
      lcd.print("HTTP: " + String(httpResponseCode));
      delay(2000);
    }
  } else {
    firebaseConnected = false;
    Serial.print("Firebase connection error: ");
    Serial.println(http.errorToString(httpResponseCode));
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase: ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Connection Fail");
    delay(2000);
  }
  
  http.end();
}

void setupRFID() {
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  
  Serial.print("MFRC522 Version: 0x");
  Serial.println(version, HEX);
  
  if (version == 0x00 || version == 0xFF) {
    rfidWorking = false;
    Serial.println("RFID RC522 tidak terdeteksi!");
  } else {
    rfidWorking = true;
    Serial.println("RFID RC522 siap digunakan!");
  }
}

void displayStatus() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (wifiConnected && firebaseConnected && rfidWorking) {
    lcd.print("System: READY");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card");
  } else {
    lcd.print("System: ERROR");
    lcd.setCursor(0, 1);
    if (!wifiConnected) lcd.print("WiFi Failed");
    else if (!firebaseConnected) lcd.print("Firebase Failed");
    else if (!rfidWorking) lcd.print("RFID Failed");
  }
}

void loop() {
  // Update time jika WiFi connected
  if (wifiConnected) {
    timeClient.update();
  }
  
  // Cek status sistem
  if (!rfidWorking) {
    delay(1000);
    return;
  }
  
  // Cek apakah ada kartu baru
  if (!rfid.PICC_IsNewCardPresent()) {
    // Hapus LCD jika sudah 5 detik sejak pembacaan terakhir
    if (millis() - lastReadTime > 5000 && lastUID != "") {
      displayStatus();
      lastUID = "";
    }
    return;
  }
  
  // Coba baca kartu
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Baca UID kartu
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  // Cek apakah kartu sama dengan yang terakhir dibaca (untuk menghindari duplikasi)
  if (uid == lastUID && (millis() - lastReadTime) < 3000) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  
  // Update waktu dan UID
  lastReadTime = millis();
  lastUID = uid;
  
  // Tampilkan di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card Detected!");
  lcd.setCursor(0, 1);
  lcd.print("Processing...");
  
  Serial.println("=== KARTU TERDETEKSI ===");
  Serial.print("UID: ");
  Serial.println(uid);
  
  // Proses absensi
  if (wifiConnected && firebaseConnected) {
    processAttendance(uid);
  } else {
    // Mode offline - simpan di EEPROM atau tampilkan saja
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("OFFLINE MODE");
    lcd.setCursor(0, 1);
    lcd.print("UID: " + uid.substring(0, 12));
    
    Serial.println("Working in offline mode - no internet connection");
  }
  
  // Hentikan komunikasi dengan kartu
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(1000);
}

void processAttendance(String uid) {
  Serial.println("=== PROCESSING ATTENDANCE ===");
  
  // Cek apakah user terdaftar
  String userName = getUserName(uid);
  
  if (userName != "") {
    Serial.print("User found: ");
    Serial.println(userName);
    
    // Record attendance
    recordAttendance(uid, userName);
    
  } else {
    Serial.println("User not found! Registering as new user...");
    
    // Tampilkan di LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New User!");
    lcd.setCursor(0, 1);
    lcd.print("Registering...");
    
    // Register user baru
    userName = "User_" + uid.substring(0, 4);
    if (registerNewUser(uid, userName)) {
      recordAttendance(uid, userName);
    }
  }
}

String getUserName(String uid) {
  String url = String(FIREBASE_DATABASE_URL) + "/users/" + uid + "/name.json?auth=" + FIREBASE_API_KEY;
  
  http.begin(client, url);
  int httpResponseCode = http.GET();
  
  String userName = "";
  if (httpResponseCode == 200) {
    String response = http.getString();
    // Remove quotes from JSON string response
    response.replace("\"", "");
    if (response != "null") {
      userName = response;
    }
  }
  
  http.end();
  return userName;
}

bool registerNewUser(String uid, String userName) {
  String url = String(FIREBASE_DATABASE_URL) + "/users/" + uid + ".json?auth=" + FIREBASE_API_KEY;
  
  // Create JSON data
  DynamicJsonDocument doc(1024);
  doc["name"] = userName;
  doc["uid"] = uid;
  doc["registered_at"] = getFormattedTime();
  doc["status"] = "active";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.PUT(jsonString);
  
  if (httpResponseCode == 200) {
    Serial.println("New user registered successfully!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("User Registered");
    lcd.setCursor(0, 1);
    lcd.print("Welcome!");
    
    http.end();
    return true;
  } else {
    Serial.print("Failed to register user. HTTP code: ");
    Serial.println(httpResponseCode);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Registration");
    lcd.setCursor(0, 1);
    lcd.print("Failed!");
    
    http.end();
    return false;
  }
}

bool recordAttendance(String uid, String userName) {
  String timestamp = getFormattedTime();
  String date = getDateOnly();
  String time = getTimeOnly();
  
  // Path untuk attendance record
  String url = String(FIREBASE_DATABASE_URL) + "/attendance/" + date + "/" + uid + "/" + String(millis()) + ".json?auth=" + FIREBASE_API_KEY;
  
  // Create JSON data
  DynamicJsonDocument doc(1024);
  doc["uid"] = uid;
  doc["name"] = userName;
  doc["timestamp"] = timestamp;
  doc["date"] = date;
  doc["time"] = time;
  doc["device"] = "ESP8266_RFID";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.PUT(jsonString);
  
  if (httpResponseCode == 200) {
    Serial.println("Attendance recorded successfully!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Welcome!");
    lcd.setCursor(0, 1);
    if (userName.length() > 16) {
      lcd.print(userName.substring(0, 16));
    } else {
      lcd.print(userName);
    }
    
    // Update user's last attendance
    updateLastAttendance(uid, timestamp);
    
    http.end();
    return true;
  } else {
    Serial.print("Failed to record attendance. HTTP code: ");
    Serial.println(httpResponseCode);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Attendance");
    lcd.setCursor(0, 1);
    lcd.print("Failed!");
    
    http.end();
    return false;
  }
}

void updateLastAttendance(String uid, String timestamp) {
  String url = String(FIREBASE_DATABASE_URL) + "/users/" + uid + "/last_attendance.json?auth=" + FIREBASE_API_KEY;
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  String data = "\"" + timestamp + "\"";
  http.PUT(data);
  http.end();
}

String getFormattedTime() {
  if (!wifiConnected) {
    return "No Internet - " + String(millis());
  }

  unsigned long epochTime = timeClient.getEpochTime();

  // Validasi waktu (minimal tahun 2001)
  if (epochTime < 1000000000) {
    return "Invalid Time - " + String(epochTime);
  }

  struct tm *ptm = gmtime((time_t *)&epochTime);

  char timeString[32];
  sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d",
          ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
          ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  return String(timeString);
}


String getDateOnly() {
  if (!wifiConnected) {
    return "offline";
  }
  
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  
  char dateString[16];
  sprintf(dateString, "%04d-%02d-%02d", 
          ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
  
  return String(dateString);
}

String getTimeOnly() {
  if (!wifiConnected) {
    return String(millis());
  }
  
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  
  char timeString[16];
  sprintf(timeString, "%02d:%02d:%02d", 
          ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  
  return String(timeString);
}

void printConnectionInfo() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("WiFi: ");
  Serial.println(wifiConnected ? "Connected" : "Failed");
  if (wifiConnected) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  
  Serial.print("Firebase: ");
  Serial.println(firebaseConnected ? "Connected" : "Failed");
  
  Serial.print("RFID: ");
  Serial.println(rfidWorking ? "Working" : "Failed");
  
  Serial.println("=== PIN CONNECTIONS ===");
  Serial.println("RFID RC522:");
  Serial.println("SDA/SS -> D2 (GPIO4)");
  Serial.println("SCK -> D5 (GPIO14)");
  Serial.println("MOSI -> D7 (GPIO13)");
  Serial.println("MISO -> D6 (GPIO12)");
  Serial.println("RST -> D1 (GPIO5)");
  Serial.println("3.3V -> 3V3");
  Serial.println("GND -> GND");
  Serial.println();
  Serial.println("LCD I2C:");
  Serial.println("SDA -> D4 (GPIO2)");
  Serial.println("SCL -> D3 (GPIO0)");
  Serial.println("VCC -> 5V or 3V3");
  Serial.println("GND -> GND");
  Serial.println("=====================");
}