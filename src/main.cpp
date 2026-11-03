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
#define PWM_RES       8    // Rezolutie 8 biti: 0 (oprit) - 255 (100% forta)

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

// Stare pompa curenta (duty: 0 - 255)
int pumpSpeed = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Functie pentru setarea puterii pompei (0 - 100%)
void setPumpPower(int percent) {
  percent = constrain(percent, 0, 100);
  pumpSpeed = map(percent, 0, 100, 0, 255);
  ledcWrite(PWM_CHANNEL, pumpSpeed);
  Serial.printf("[POMPA] Putere setata: %d%% (PWM Duty: %d/255)\n", percent, pumpSpeed);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==================================================");
  Serial.println("  AquaSense - Test Senzor Sol, Debit & MOSFET    ");
  Serial.println("  Sol: GPIO 25 | Debit: GPIO 27 | MOSFET: GPIO 26 ");
  Serial.println("==================================================");
  Serial.println("Comenzi in terminal:");
  Serial.println("  '1' -> Porneste pompa la 100%");
  Serial.println("  '2' -> Porneste pompa la 50% (PWM)");
  Serial.println("  '0' -> Opreste pompa");
  Serial.println("==================================================");

  // 1. Configurare ADC sol
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  // 2. Configurare debitmetru
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  // 3. Configurare PWM pentru comanda MOSFET
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PUMP_PIN, PWM_CHANNEL);
  setPumpPower(0); // Pornim cu pompa oprita

  lastCalcTime = millis();
}

void loop() {
  // Verificare comenzi primite prin terminalul USART
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '1') {
      setPumpPower(100);
    } else if (cmd == '2') {
      setPumpPower(50);
    } else if (cmd == '0') {
      setPumpPower(0);
    }
  }

  // La fiecare secunda afisam telemetria completa
  unsigned long now = millis();
  if (now - lastCalcTime >= 1000) {
    // Calcule debit
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    unsigned long elapsedTime = now - lastCalcTime;
    lastCalcTime = now;

    float frequency = (pulses * 1000.0) / elapsedTime;
    float flowRateLpm = frequency / FLOW_CALIBRATION_FACTOR;
    float currentMilliliters = (flowRateLpm / 60.0) * (elapsedTime / 1000.0) * 1000.0;
    totalMilliliters += currentMilliliters;

    // Citire umiditate sol
    long sum = 0;
    for (int i = 0; i < 10; i++) {
      sum += analogRead(SOIL_PIN);
      delay(5);
    }
    int rawSoil = sum / 10;
    float soilVoltage = (rawSoil / 4095.0) * 3.3;
    int soilPercent = map(rawSoil, AIR_VALUE, WATER_VALUE, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // Afisare formatata
    Serial.println("--------------------------------------------------");
    Serial.printf("[SOL]        Raw: %-4d | %4.2fV | Umiditate: %3d%%\n", rawSoil, soilVoltage, soilPercent);
    Serial.printf("[DEBITMETRU] Impulsuri/s: %-3lu | Debit: %5.2f L/min | Total: %6.1f mL\n", 
                  pulses, flowRateLpm, totalMilliliters);
    Serial.printf("[POMPA]      Stare: %s (%d%%)\n", (pumpSpeed > 0 ? "PORNITA" : "OPRITA"), map(pumpSpeed, 0, 255, 0, 100));
  }
}
