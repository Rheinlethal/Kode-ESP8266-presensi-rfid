JANGAN LUPA DIGANTI DI BAGIAN CODE ESP

// WiFi credentials
#define WIFI_SSID " "//isi SSID WIFI
#define WIFI_PASSWORD " "//isi password Wifi

// Firebase configuration - menggunakan REST API langsung
#define FIREBASE_PROJECT_ID " "//isi project id
#define FIREBASE_DATABASE_URL " "//isi URL Firebase
#define FIREBASE_API_KEY " "//isi API key firebase

Library yang Digunakan

ESP8266WiFi.h
Digunakan untuk menghubungkan NodeMCU ESP8266 ke jaringan WiFi. Library ini merupakan bagian dari core ESP8266 yang dikembangkan oleh Espressif Systems.

ESP8266HTTPClient.h
Berfungsi untuk melakukan permintaan HTTP (seperti GET, POST, PUT) yang dibutuhkan untuk mengirim dan menerima data dari Firebase. Termasuk dalam core ESP8266.

WiFiClientSecure.h
Digunakan untuk membuat koneksi HTTPS (SSL/TLS), memungkinkan komunikasi aman dengan Firebase Realtime Database.

ArduinoJson.h
Library ini digunakan untuk membuat dan memproses data dalam format JSON. Dikembangkan oleh Benoît Blanchon dan banyak digunakan dalam proyek IoT yang berinteraksi dengan REST API.

SPI.h
Digunakan untuk mengatur komunikasi SPI antara NodeMCU dan modul RFID RC522. Library ini adalah bagian dari pustaka standar Arduino.

MFRC522.h
Library untuk mengontrol modul RFID RC522. Memungkinkan pembacaan UID kartu RFID dan pengelolaan komunikasi data RFID. Dikembangkan oleh Miguel Balboa.

Wire.h
Merupakan library komunikasi I2C standar Arduino. Digunakan untuk berkomunikasi dengan perangkat I2C seperti LCD.

LiquidCrystal_I2C.h
Digunakan untuk mengendalikan LCD 16x2 dengan antarmuka I2C. Versi yang umum digunakan berasal dari fork johnrickman atau komunitas open-source lainnya.

NTPClient.h
Digunakan untuk mendapatkan waktu secara real-time dari server NTP. Sangat berguna untuk mencatat waktu absensi. Library ini dikembangkan oleh Fabrice Weinberg.

WiFiUdp.h
Digunakan sebagai pendukung NTPClient untuk melakukan komunikasi dengan protokol UDP, bagian dari core ESP8266.
