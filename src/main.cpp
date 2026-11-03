#include <Arduino.h>

// ==========================================
// CONFIGURARE PINI
// ==========================================
// 1. Senzor umiditate sol (Pin 14 header / GPIO 25)
#define SOIL_PIN 25

// 2. Senzor debit apa YF-S401 (Fir galben -> GPIO 27)
#define FLOW_PIN 27

// ==========================================
// CALIBRARE SENZOR SOL
// ==========================================
const int AIR_VALUE = 3200;   // Valoare in aer (uscat)
const int WATER_VALUE = 1450; // Valoare in apa (ud)

// ==========================================
// VARIABILE DEBITMETRU YF-S401
// ==========================================
// YF-S401 (0.3 - 6 L/min): Formula standard este F = 98 * Q (Q in L/min)
// Adica ~5880 impulsuri per litru (~5.88 impulsuri per mL)
const float FLOW_CALIBRATION_FACTOR = 98.0; 

volatile unsigned long pulseCount = 0;
unsigned long lastCalcTime = 0;
float totalMilliliters = 0.0;

// Rutina de intrerupere (se apeleaza la fiecare impuls de la debitmetru)
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==================================================");
  Serial.println("  AquaSense - Test Senzor Sol + Debitmetru YF-S401");
  Serial.println("  Senzor Sol: GPIO 25 | Debitmetru: GPIO 27       ");
  Serial.println("==================================================");

  // Configurare ADC senzor sol (12 biti: 0 - 4095, plaja 0 - 3.3V)
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  // Configurare pin debitmetru cu pullup intern
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  lastCalcTime = millis();
}

void loop() {
  unsigned long now = millis();
  
  // Masuram si afisam la fiecare 1 secunda (1000 ms)
  if (now - lastCalcTime >= 1000) {
    // 1. Calcule debitmetru
    // Dezactivam temporar intreruperile pentru a citi variabila atomic
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    unsigned long elapsedTime = now - lastCalcTime;
    lastCalcTime = now;

    // Frecventa impulsurilor (Hz = impulsuri pe secunda)
    float frequency = (pulses * 1000.0) / elapsedTime;

    // Debit curent in L/min: Q = F / 98
    float flowRateLpm = frequency / FLOW_CALIBRATION_FACTOR;

    // Volum trecut in acest interval (in mL)
    // (flowRateLpm / 60) * (elapsedTime / 1000) * 1000 mL
    float currentMilliliters = (flowRateLpm / 60.0) * (elapsedTime / 1000.0) * 1000.0;
    totalMilliliters += currentMilliliters;

    // 2. Citire senzor umiditate sol (medie din 10 citiri)
    long sum = 0;
    for (int i = 0; i < 10; i++) {
      sum += analogRead(SOIL_PIN);
      delay(5);
    }
    int rawSoil = sum / 10;
    float soilVoltage = (rawSoil / 4095.0) * 3.3;
    int soilPercent = map(rawSoil, AIR_VALUE, WATER_VALUE, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // 3. Afisare pe terminal serial
    Serial.println("--------------------------------------------------");
    Serial.printf("[SOL]        Raw: %-4d | %4.2fV | Umiditate: %3d%%\n", rawSoil, soilVoltage, soilPercent);
    Serial.printf("[DEBITMETRU] Impulsuri/s: %-3lu | Debit: %5.2f L/min | Total: %6.1f mL\n", 
                  pulses, flowRateLpm, totalMilliliters);
  }
}
