#include <Arduino.h>

// Pin senzor umiditate sol
// Pe placuta e pinul 14 / GPIO 25
// NOTA: GPIO 25 e pe ADC2. Pentru teste fara Wi-Fi merge perfect.
// Daca mai tarziu pornesti Wi-Fi, muta-l pe GPIO 34 sau 35 (ADC1).
#define SOIL_PIN 25

// Valori de calibrare (se ajusteaza dupa primele teste):
// Aer (complet uscat) ~ 3100 - 3500
// Pahar cu apa (100% umed) ~ 1300 - 1600
const int AIR_VALUE = 3200;
const int WATER_VALUE = 1450;

void setup() {
  // Pornire terminal serial / USART
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=========================================");
  Serial.println("  AquaSense - Test Senzor Umiditate Sol  ");
  Serial.println("  Pin: GPIO 25 (Pin 14 header)           ");
  Serial.println("=========================================");

  // Configurare rezolutie ADC (12 biti: 0 - 4095)
  analogReadResolution(12);

  // Atenuare 11dB pentru plaja completa 0 - 3.3V
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
}

void loop() {
  // Facem o medie din 10 citiri ca sa fie semnalul stabil
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(10);
  }
  int rawValue = sum / 10;

  // Calcul tensiune aproximativa (0 - 3.3V)
  float voltage = (rawValue / 4095.0) * 3.3;

  // Calcul procent umiditate
  // La senzorul capacitiv: valoare mare = sol uscat, valoare mica = sol ud
  int moisturePercent = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  // Afisare pe terminal serial
  Serial.print("[SOL] Raw ADC: ");
  Serial.print(rawValue);
  Serial.print(" | Volti: ");
  Serial.print(voltage, 2);
  Serial.print("V | Umiditate: ");
  Serial.print(moisturePercent);
  Serial.print("%");

  // Interpretare rapida
  if (moisturePercent < 20) {
    Serial.println(" -> Sol USCAT (trebuie udat)");
  } else if (moisturePercent < 60) {
    Serial.println(" -> Sol OPTIM");
  } else {
    Serial.println(" -> Sol FOARTE UD");
  }

  delay(1000);
}
