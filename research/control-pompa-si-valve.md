# Metode low-cost pentru controlul pompei si al electrovalvelor

Pentru a comanda pompa submersibila de `3-6V` (`130-220 mA` in mers, pana la `600-800 mA` la pornire) si electrovalvele de `12V` (`120 mA` fiecare) de la o placa cu logica de `3.3V` (ESP32) sau `5V` (Arduino), avem trei variante principale:

---

## 1. Comparatie rapida (Pret si Eficienta)

| Metoda | Cost estimativ (RO) | Consum propriu | Control turatie pompa (PWM) | Cand merita folosita |
| :--- | :--- | :--- | :--- | :--- |
| **Tranzistor MOSFET Logic-Level (`AO3400` / `IRLZ44N` / Modul `D4184`)** | `0.50 - 5 lei` / canal | `0 mA` (nu consuma nimic pe poarta) | **DA** (putem regla debitul din cod) | Cea mai buna varianta pentru pompa sau cand avem 1-2 valve |
| **Integrat cu 7-8 canale (`ULN2003A` / `ULN2803A`)** | `3 - 4.5 lei` tot cipul (7-8 canale) | `~1 mA` pe pin | Nu este ideal la frecvente mari | Cea mai ieftina si simpla solutie cand avem mai multe electrovalve (4-6 randuri) |
| **Modul cu Releu mecanic (`5V`)** | `8 - 16 lei` (1-4 canale) | **`70-80 mA` per releu activ** | **NU** (s-ar strica mecanic) | Doar daca am comanda motoare mari la `220V AC`. Pentru `5V/12V` pe baterie nu este recomandat |

---

## 2. De ce tranzistorii sunt mai buni decat releele in proiectul nostru

### A. Problema releelor mecanice
- **Consum inutil de curent:** doar ca sa tina contactul inchis, bobina unui releu de `5V` consuma `70-80 mA`. Daca tinem 3 relee pornite, consumam mai mult curent pe relee decat pe pompa in sine.
- **Uzura si zgomot:** contactele mecanice fac scantei la oprirea bobinelor si se uzeaza in timp, plus ca nu permit reglarea vitezei pompei (PWM).

### B. Tranzistor MOSFET Logic-Level (Recomandat pentru pompa)
- Modele potrivite: **AO3400** (foarte ieftin, sub `1 leu`), **IRLZ44N** (format clasic cu pini, `~5 lei`) sau module gata facute cu **D4184 / LR7843** (`~5 lei`).
- *Atentie:* trebuie evitat modulul cu **IRF520**, deoarece are nevoie de `10V` pe poarta ca sa se deschida complet si nu merge bine direct de la pinii de `3.3V` ai unui ESP32.
- **Avantaj mare:** nu are cadere de tensiune (toata puterea ajunge la pompa), nu se incalzeste deloc si ne permite sa reglam turatia pompei prin PWM (de exemplu, sa reducem debitul pentru randurile mai apropiate).

### C. Tranzistor BJT clasic (`2N2222` / `BC337`) vs. `ULN2003A`
- Un tranzistor NPN mic (`2N2222`) e foarte ieftin (`0.20 lei`), dar cere `15-20 mA` din pinul microcontrolerului si se poate arde la curentul de pornire al pompei (`800 mA`).
- In schimb, integratul **ULN2003A** (`~3 lei`) contine intr-o singura piesa **7 tranzistoare Darlington** + rezistentele de intrare + diodele de protectie. Un singur cip de `3 lei` poate controla toate electrovalvele de `12V` (cate un canal pentru fiecare valva de `120 mA`).

---

## 3. Protectia obligatorie pentru pompa si valve (Dioda Flyback)

Atat pompa, cat si electrovalvele contin bobine. Cand sunt oprite brusc, bobina produce un varf de tensiune inversa care poate arde tranzistorul sau reseta microcontrolerul.
- Pe fiecare pompa sau valva comandata cu MOSFET/tranzistor separat se pune in paralel o **dioda antiparalel (flyback)**:
  - **1N5819 (Schottky)** pentru pompa (fiind rapida, permite si control PWM).
  - **1N4007** pentru electrovalve (sau se folosesc direct diodele deja integrate in cipul `ULN2003A`).
- Pe bornele pompei e bine sa punem si un mic condensator ceramic de `100 nF` pentru a filtra zgomotul electric.

---

## 4. Concluzie practica pentru schema noastra

Cea mai simpla si ieftina combinatie (sub `10 lei` in total):
1. **Pentru pompa (`3-6V`):** un tranzistor MOSFET **AO3400 / IRLZ44N** (sau modul **D4184**) + o dioda **1N5819**, ca sa putem controla si debitul prin PWM fara pierderi de tensiune.
2. **Pentru mai multe electrovalve (`12V`):** un singur integrat **ULN2003A** (care comanda pana la 7 valve direct de la pinii microcontrolerului, fara alte piese pe langa).
