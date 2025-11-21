#include <WiFi.h>
#include <WiFiUdp.h>

// ======================= WiFi AP SETTINGS =======================
const char* AP_SSID = "CPR_Dog_Test_AP";   // Test WiFi name
const char* AP_PASS = "cprtest123";        // Test WiFi password

WiFiUDP udp;
const uint16_t UDP_PORT = 4210;           // Must match Unity listener
IPAddress broadcastIp;

// Send interval (ms)
const unsigned long SEND_INTERVAL_MS = 333;  // 3 Hz
unsigned long lastSendTime = 0;


// ======================= RANDOM HELPER =======================
// return random float in [minVal, maxVal]
float randomFloat(float minVal, float maxVal) {
  // random() gives long in [0, max)
  long r = random(0, 10000); // 0..9999
  float t = (float)r / 10000.0f;   // 0..0.9999
  return minVal + t * (maxVal - minVal);
}


// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("CPR Dog - UDP Random Test Node");

  // Seed the random generator
  randomSeed(analogRead(0)); // or esp_random(), but this is fine for testing

  // --- WiFi AP init ---
  WiFi.mode(WIFI_AP);

  bool apOk = WiFi.softAP(AP_SSID, AP_PASS);
  if (!apOk) {
    Serial.println("[WiFi] Failed to start AP!");
  } else {
    Serial.println("[WiFi] AP started.");
    Serial.print("       SSID: "); Serial.println(AP_SSID);
    Serial.print("       PASS: "); Serial.println(AP_PASS);
    Serial.print("       AP IP: "); Serial.println(WiFi.softAPIP());
  }

  // Compute broadcast IP (x.x.x.255)
  IPAddress apIP = WiFi.softAPIP();
  broadcastIp = IPAddress(apIP[0], apIP[1], apIP[2], 255);

  // --- UDP init ---
  if (udp.begin(UDP_PORT)) {
    Serial.print("[UDP] Listening on port ");
    Serial.println(UDP_PORT);
    Serial.print("[UDP] Broadcast IP: ");
    Serial.println(broadcastIp);
  } else {
    Serial.println("[UDP] Failed to start UDP.");
  }

  lastSendTime = millis();
}


// ======================= MAIN LOOP =======================
void loop() {
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime += SEND_INTERVAL_MS;

    // 1) Generate random test value (simulate force in kg)
    float fakeForceKg = randomFloat(0.0f, 10.0f);   // 0.00 .. 9.99 kg

    // 2) Build payload (string)
    // Option A: like the real CPR Dog
    String payload = "ForceKg=" + String(fakeForceKg, 2);

    // Option B: just a random int example (uncomment if you want to test int)
    // int fakeInt = random(0, 200); // 0..199
    // String payload = "RandomInt=" + String(fakeInt);

    // 3) Send UDP broadcast
    udp.beginPacket(broadcastIp, UDP_PORT);
    udp.print(payload);
    udp.endPacket();

    // 4) Debug to Serial
    Serial.print("[UDP] Sent: ");
    Serial.println(payload);
  }
}
