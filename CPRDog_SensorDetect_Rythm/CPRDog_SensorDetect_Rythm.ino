#include "HX711.h"

#define DOUT  23
#define CLK   22

HX711 scale(DOUT, CLK);

// === Your calibrated values ===
float calibration_factor = 52484.5273;
long  zero_factor        = 8349208;

float get_units_kg();

// ---- Peak-based detection settings ----
const float ALPHA          = 0.4f;   // EMA smoothing factor
const float MIN_PEAK_KG    = 3.0f;   // minimum peak to be a valid compression
const unsigned long SAMPLE_INTERVAL_MS = 20;   // 50 Hz sampling
const unsigned long MIN_INTERVAL_MS    = 250;  // min time between compressions (~240 CPM max)

// State
float smoothedForce       = 0.0f;
float lastSmoothedForce   = 0.0f;
float lastSlope           = 0.0f;

unsigned long lastSampleTime      = 0;
unsigned long lastCompressionTime = 0;
unsigned long sessionStartTime    = 0;

int   compressionCount    = 0;
float avgPeakForce        = 0.0f;

void setup() {
  Serial.begin(115200);
  Serial.println("\nCPR Force Monitor (peak-based)");

  scale.set_scale(calibration_factor);
  scale.set_offset(zero_factor);

  sessionStartTime = millis();
}

void loop() {
  unsigned long now = millis();

  // 1) fixed-rate sampling
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleTime += SAMPLE_INTERVAL_MS;

  // 2) read + smooth
  float rawKg = get_units_kg();
  smoothedForce = ALPHA * rawKg + (1.0f - ALPHA) * smoothedForce;

  // 3) compute slope
  float slope = smoothedForce - lastSmoothedForce;

  // 4) detect local peak (slope turns from + to -)
  bool wasRising   = (lastSlope > 0.0f);
  bool nowFalling  = (slope <= 0.0f);

  if (wasRising && nowFalling) {
    // local maximum near current smoothedForce
    float peak = smoothedForce;
    unsigned long dt = now - lastCompressionTime;

    if (peak >= MIN_PEAK_KG && (lastCompressionTime == 0 || dt > MIN_INTERVAL_MS)) {
      // count this as a compression
      compressionCount++;
      lastCompressionTime = now;

      // update average peak
      avgPeakForce += (peak - avgPeakForce) / compressionCount;

      // evaluate quality for this compression
      evaluateCompression(now, peak, dt);
    }
  }

  lastSlope         = slope;
  lastSmoothedForce = smoothedForce;

  // 5) periodic status print
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 250) {
    lastPrint = now;
    float cpm = computeCPM(now);

    Serial.print("Force: ");
    Serial.print(smoothedForce, 2);
    Serial.print(" kg  | PeakAvg: ");
    Serial.print(avgPeakForce, 2);
    Serial.print(" kg  | CPM: ");
    Serial.print(cpm, 1);
    Serial.print("  | Count: ");
    Serial.println(compressionCount);
  }
}

// --- HX711 to kg ---
float get_units_kg() {
  return scale.get_units() * 0.453592f;
}

// --- overall CPM from session ---
float computeCPM(unsigned long nowMs) {
  if (compressionCount < 2) return 0.0f;
  float elapsedSec = (nowMs - sessionStartTime) / 1000.0f;
  if (elapsedSec <= 0.0f) return 0.0f;
  return (compressionCount * 60.0f) / elapsedSec;
}

// --- evaluate a single compression ---
void evaluateCompression(unsigned long nowMs, float peakKg, unsigned long dtMs) {
  // force zone
  const float MIN_GOOD_KG = 4.0f;
  const float MAX_GOOD_KG = 8.0f;

  const char* forceResult;
  if (peakKg < MIN_GOOD_KG)      forceResult = "Too weak";
  else if (peakKg > MAX_GOOD_KG) forceResult = "Too strong";
  else                           forceResult = "Good force";

  // instant CPM from interval
  float instCPM = 0.0f;
  if (dtMs > 0) {
    instCPM = 60000.0f / (float)dtMs;
  }

  const char* rhythmResult;
  if (instCPM < 80.0f)           rhythmResult = "Too slow";
  else if (instCPM > 140.0f)     rhythmResult = "Too fast";
  else                           rhythmResult = "Good rhythm";

  Serial.print("Compression #");
  Serial.print(compressionCount);
  Serial.print(" -> Peak: ");
  Serial.print(peakKg, 2);
  Serial.print(" kg (");
  Serial.print(forceResult);
  Serial.print("), InstCPM: ");
  Serial.print(instCPM, 1);
  Serial.print(" (");
  Serial.print(rhythmResult);
  Serial.println(")");
}
