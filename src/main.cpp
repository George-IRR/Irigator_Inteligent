#include <Arduino.h>

// ==========================================
// CONFIGURARE PINI ESP32
// ==========================================
#define SOIL_PIN 25   // Senzor umiditate sol (Pin 14 header, ADC2)
#define FLOW_PIN 27   // Debitmetru YF-S401 (Fir galben, intrerupere)
#define PUMP_PIN 26   // Modul MOSFET TRIG/PWM (Comanda Pompa R385)

// ==========================================
// CONFIGURARE PWM POMPA (LEDC)
// ==========================================
#define PWM_CHANNEL   0
#define PWM_FREQ      5000 // 5 kHz comutare silentioasa
#define PWM_RES       8    // Rezolutie 8 biti: 0 - 255

// ==========================================
// CALIBRARE SENZOR SOL
// ==========================================
const int AIR_VALUE = 3200;   // Sol complet uscat
const int WATER_VALUE = 1450; // Sol complet ud

// ==========================================
// VARIABILE DEBITMETRU YF-S401
// ==========================================
const float FLOW_CALIBRATION_FACTOR = 98.0; // F = 98 * Q (L/min)
volatile unsigned long pulseCount = 0;
unsigned long lastCalcTime = 0;
float totalMilliliters = 0.0;
int pumpPowerPercent = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setPumpPower(int percent) {
  pumpPowerPercent = constrain(percent, 0, 100);
  int duty = map(pumpPowerPercent, 0, 100, 0, 255);
  ledcWrite(PWM_CHANNEL, duty);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // 1. Configurare ADC sol (12 biti)
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  // 2. Configurare debitmetru
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  // 3. Configurare PWM MOSFET
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PUMP_PIN, PWM_CHANNEL);
  setPumpPower(0);

  lastCalcTime = millis();
}

void loop() {
  // Procesare comenzi din Serial / WebSerial
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '1') {
      setPumpPower(100);
    } else if (cmd == '2') {
      setPumpPower(50);
    } else if (cmd == '0') {
      setPumpPower(0);
    } else if (cmd == 'P') {
      int val = Serial.parseInt();
      setPumpPower(val);
    }
  }

  // Telemetrie la fiecare 500 ms (raspuns rapid pentru dashboard)
  unsigned long now = millis();
  if (now - lastCalcTime >= 500) {
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    unsigned long elapsedTime = now - lastCalcTime;
    lastCalcTime = now;

    // Calcule debit
    float frequency = (pulses * 1000.0) / elapsedTime;
    float flowRateLpm = frequency / FLOW_CALIBRATION_FACTOR;
    float currentMilliliters = (flowRateLpm / 60.0) * (elapsedTime / 1000.0) * 1000.0;
    totalMilliliters += currentMilliliters;

    // Citire umiditate sol
    long sum = 0;
    for (int i = 0; i < 5; i++) {
      sum += analogRead(SOIL_PIN);
      delay(2);
    }
    int rawSoil = sum / 5;
    float soilVoltage = (rawSoil / 4095.0) * 3.3;
    int soilPercent = map(rawSoil, AIR_VALUE, WATER_VALUE, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // Trimitem JSON direct pe Serial pentru Dashboard
    Serial.printf("{\"soil\":%d,\"raw\":%d,\"volt\":%.2f,\"flow\":%.2f,\"hz\":%d,\"total\":%.1f,\"pump\":%d}\n",
                  soilPercent, rawSoil, soilVoltage, flowRateLpm, (int)frequency, totalMilliliters, pumpPowerPercent);
  }
}
