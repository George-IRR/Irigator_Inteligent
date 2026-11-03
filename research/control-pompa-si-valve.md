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

## 3. Cum alimentam pompa de `6V` din sursa de `12V` (Step-Down)

Pentru ca electrovalvele merg la `12V` iar pompa la `3-6V`, cel mai simplu este sa avem o singura sursa de `12V` si sa coboram tensiunea la `6V` pentru pompa:

### A. Convertor DC-DC Step-Down (Buck) - Recomandat
- **MP1584EN (Mini Step-Down)** sau **LM2596** (`~4-8 lei`): au eficienta mare (`85-92%`), nu se incalzesc si tin `1.5-2A` fara probleme. Reglam din potentiometru iesirea exact la `6V`.
- **De ce nu regulator liniar (`LM7806`):** ca sa scada de la `12V` la `6V` la un curent de `0.5-1A`, ar transforma `3-6W` direct in caldura si ar intra in protectie termica fara radiator mare.

### B. Alternative mai compacte la modulul MOSFET separat
Modulul dual MOSFET (`D4184`, `15A`, `400W`, `~2.83 lei` pe Sigmanortec sau `6.49 lei` pe Optimus Digital) este foarte ieftin si bun, dar supradimensionat pentru pompa noastra. Daca vrem ceva mai integrat:
1. **Step-Down cu pin de `EN` (Enable):** unele module cu **MP1584EN** sau **TPS5430** au pin `EN`. Putem lega pinul direct la microcontroler (HIGH = porneste convertorul si pompa la `6V`, LOW = opreste tot cu consum aproape zero) si scapam de modulul MOSFET.
2. **Driver mini de motor (`DRV8833` sau `MX1508`):** module mici (`15x15 mm`) care tin `1.5A` si pot comanda direct pompa la `6V`.

### C. Schema de legare (Sursa 12V -> Step-Down 6V -> Modul MOSFET -> Pompa)
```text
[ Sursa 12V ]
     |
     +---> [ Modul Step-Down IN+ / IN- ] (reglat la 6V)
                     |
                     +---> [ OUT+ ] ----------------------> [ Pompa (+) ]
                     +---> [ OUT- / GND ] -> [ Modul MOSFET: VIN- ]
                                                    |
                                             [ Modul MOSFET: VOUT- ] -> [ Pompa (-) ]

[ Microcontroler (3.3V / 5V) ]
     +---> GPIO (PWM / ON-OFF) -> [ Modul MOSFET: TRIG/PWM ]
     +---> GND -----------------> [ Modul MOSFET: GND ]
```

---

## 4. Protectia obligatorie pentru pompa si valve (Dioda Flyback)

Atat pompa, cat si electrovalvele contin bobine. Cand sunt oprite brusc, bobina produce un varf de tensiune inversa care poate arde tranzistorul sau reseta microcontrolerul.
- Pe fiecare pompa sau valva comandata cu MOSFET/tranzistor separat se pune in paralel o **dioda antiparalel (flyback)** (cu linia/catodul spre `+` si anodul spre `-`):
  - **1N5819 (Schottky)** pentru pompa (fiind rapida, permite si control PWM).
  - **1N4007** pentru electrovalve (sau se folosesc direct diodele deja integrate in cipul `ULN2003A`).
- Pe bornele pompei e bine sa punem si un mic condensator ceramic de `100 nF` pentru a filtra zgomotul electric.

---

## 5. Concluzie practica pentru schema noastra

Cea mai simpla si ieftina combinatie:
1. **Pentru coborarea tensiunii (`12V` -> `6V`):** un modul step-down **MP1584EN** sau **LM2596** reglat la `6V`.
2. **Pentru pompa (`3-6V`):** modulul dual MOSFET (**D4184**, `15A`) sau un tranzistor **AO3400 / IRLZ44N** + o dioda **1N5819**, ca sa putem controla si debitul prin PWM fara pierderi de tensiune.
3. **Pentru mai multe electrovalve (`12V`):** un singur integrat **ULN2003A** (care comanda pana la 7 valve direct de la pinii microcontrolerului, fara alte piese pe langa).
