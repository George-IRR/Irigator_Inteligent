# Irigator Inteligent (AquaSense)

Proiect dezvoltat pentru provocarea AquaSense (SAS UnitBV) - sistem inteligent de irigatii bazat pe ESP32, senzori de umiditate si debitmetru.

## Structura Proiect

* `src/main.cpp` - Codul sursa pentru ESP32 (citire senzori, calcul procent umiditate, comunicatie serial / USART).
* `platformio.ini` - Configurarea proiectului PlatformIO pentru ESP32 (`board = esp32dev`, baud rate 115200).
* `spec/` - Specificatii hardware, componente, pinout si cerintele provocarii.
* `research/` - Documentatie tehnica (formule FAO-56, control pompe si valve).
* `image/` - Diagrame bloc si diagrame electrice SVG.

---

## Conexiuni Pini (ESP32)

| Componenta | Pin Componenta | Pin ESP32 | Tensiune Alimentare | Note |
| :--- | :--- | :--- | :--- | :--- |
| **Senzor Umiditate Sol** | VCC / GND / AOUT | 3V3 / GND / **GPIO 25** (Pin 14 header) | **3.3V** | Obligatoriu 3.3V ca sa nu arzi ADC-ul. GPIO 25 e pe ADC2 (fara Wi-Fi). La activare Wi-Fi se muta pe GPIO 34/35 (ADC1). |
| **Debitmetru (YF-S401)** | Rosu / Negru / Galben | VIN (5V) / GND / **GPIO 27** | **5V** | Firul galben transmite impulsuri Hall catre GPIO 27. |
| **Modul MOSFET #1 (Pompa R385)** | TRIG (PWM) / GND | **GPIO 25** (sau dedicat PWM) / GND | 12V extern pe forta | Comanda pompa de apa 12V cu dioda 1N5819 in antiparalel. |
| **Modul MOSFET #2 (Valva 12V)** | TRIG / GND | **GPIO 26** / GND | 12V extern pe forta | Comanda electrovalva normal deschisa cu dioda 1N5819 in antiparalel. |

---

## Comenzi Utile (PlatformIO)

Compilare si upload pe ESP32:
```bash
pio run -t upload
```

Monitorizare terminal serial (USART):
```bash
pio device monitor
```
*(sau la 115200 baud direct)*
