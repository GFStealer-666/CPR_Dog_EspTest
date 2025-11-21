#include "HX711.h"

// --- Your saved calibration values ---
float calibration_factor = 8349208.00;   // will be updated after calibration
long  zero_factor        = 52484.5273;    // will be updated after calibration

#define DOUT 23
#define CLK  22
#define DEC_POINT 2

HX711 scale(DOUT, CLK);

float get_units_kg();

// ----------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Load Cell Ready ===");
  Serial.println("Press 'c' to CALIBRATE");
  Serial.println("Press 'r' for readings\n");

  scale.set_scale(calibration_factor);
  scale.set_offset(zero_factor);
}

// ----------------------------------------------------

void loop() 
{
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'c') {
      calibrate();
    }
  }

  Serial.print("Reading: ");
  Serial.print(get_units_kg(), DEC_POINT);
  Serial.println(" kg");

  delay(250);
}

// ----------------------------------------------------
// Convert lbs → kg (HX711 default returns lbs when using get_units())
// ----------------------------------------------------
float get_units_kg() {
  return scale.get_units() * 0.453592;
}

// ----------------------------------------------------
// CALIBRATION FUNCTION
// ----------------------------------------------------
void calibrate() {
  Serial.println("\n=== CALIBRATION MODE ===");

  // ---- Step 1: TARE / ZERO ----
  Serial.println("Remove all weight from load cell...");
  delay(3000);

  scale.set_scale();  // no scale factor
  scale.tare();       // reset offset

  zero_factor = scale.read_average(20);
  Serial.print("Zero factor (offset) = ");
  Serial.println(zero_factor);

  // ---- Step 2: PLACE A KNOWN WEIGHT ----
  Serial.println("\nPlace known weight on load cell.");
  Serial.println("Enter the weight in kg (ex: 5.0):");

  while (!Serial.available()) {}
  float known_kg = Serial.parseFloat();
  Serial.print("You entered: ");
  Serial.print(known_kg);
  Serial.println(" kg");

  delay(3000);

  long reading = scale.read_average(20);
  long diff = reading - zero_factor;

  // compute raw counts per kg
  calibration_factor = diff / (known_kg / 0.453592);  // convert kg→lbs because get_units uses lbs

  Serial.println("\n=== NEW CALIBRATION COMPLETE ===");
  Serial.print("New zero_factor = ");
  Serial.println(zero_factor);

  Serial.print("New calibration_factor = ");
  Serial.println(calibration_factor, 4);

  Serial.println("\nCopy these into your main code!");

  // apply new calibration immediately
  scale.set_offset(zero_factor);
  scale.set_scale(calibration_factor);

  Serial.println("\nReturning to normal mode...\n");
}

