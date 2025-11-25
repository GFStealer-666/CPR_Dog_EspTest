#include <WiFi.h>
#include <WiFiUdp.h>
#include "HX711.h"

// ======================= HX711 SETTINGS =======================
HX711 scale;

const int HX_DOUT_PIN = 23;   // DT
const int HX_SCK_PIN  = 22;   // SCK

// === Your Calibration Values ===
long OFFSET = 8499924;              // zero_factor
float SCALE = 101606.7266f;         // calibration_factor

// Optional smoothing
const int HX_AVG_SAMPLES = 10;


// ======================= WiFi AP SETTINGS =======================
const char* AP_SSID = "CPR_Dog_Test_AP";
const char* AP_PASS = "cprtest123";

WiFiUDP udp;
const uint16_t UDP_PORT = 4210;
IPAddress broadcastIp;

const unsigned long SEND_INTERVAL_MS = 100;   // 10 Hz recommended for CPR
unsigned long lastSendTime = 0;


// ======================= READ FORCE FROM HX711 =======================
float readForceKg()
{
  long raw = scale.read_average(HX_AVG_SAMPLES);
  long value = raw - OFFSET;
  float weightKg = (float)value / SCALE;

  return weightKg;
}


// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nCPR Dog - HX711 + UDP Sender");

  // -------- HX711 init --------
  scale.begin(HX_DOUT_PIN, HX_SCK_PIN);
  if (scale.is_ready()) {
    Serial.println("[HX711] Ready");
  } else {
    Serial.println("[HX711] ERROR: Not ready!");
  }

  // -------- WiFi AP init --------
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(AP_SSID, AP_PASS);
  if (!apOk) {
    Serial.println("[WiFi] Failed to start AP!");
  } else {
    Serial.println("[WiFi] AP started");
    Serial.print("[WiFi] SSID: "); Serial.println(AP_SSID);
    Serial.print("[WiFi] PASS: "); Serial.println(AP_PASS);
    Serial.print("[WiFi] AP IP: "); Serial.println(WiFi.softAPIP());
  }

  IPAddress apIP = WiFi.softAPIP();
  broadcastIp = IPAddress(apIP[0], apIP[1], apIP[2], 255);

  // -------- UDP init --------
  if (udp.begin(UDP_PORT)) {
    Serial.print("[UDP] Listening on port ");
    Serial.println(UDP_PORT);
  } else {
    Serial.println("[UDP] Failed to start UDP!");
  }

  lastSendTime = millis();
}


// ======================= LOOP =======================
void loop() {
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    float forceKg = readForceKg();
    String payload = "ForceKg=" + String(forceKg, 2);

    udp.beginPacket(broadcastIp, UDP_PORT);
    udp.print(payload);
    udp.endPacket();

    Serial.print("[HX711] "); Serial.print(forceKg, 2); Serial.println(" kg");
  }
}
