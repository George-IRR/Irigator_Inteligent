# Specificatii tehnice - Irigator Inteligent

Acest document centralizeaza ideile din `spec-manual.md` si le extinde cu detalii tehnice si variante practice de implementare pentru sistemul de irigatie.

---

## 1. Obiectivul proiectului

Scopul sistemului este sa ofere o irigatie automata cat mai precisa, calculata pe baza nevoilor reale ale plantelor, fara costuri mari si fara sa incarcam instalatia cu piese inutile pe fiecare planta sau rand in parte.

Principalele probleme pe care proiectul le rezolva:
- **Consumul de apa:** fiecare rand primeste exact volumul necesar de apa, evitand risipa.
- **Consumul de energie:** arhitectura gandita sa tina cont de consumul pompei, al valvelor si al nodurilor de senzori.
- **Productie mai mare si mai sanatoasa:** evitam atat uscarea solului, cat si udarea in exces (balti sau umiditate prea mare la radacina, care duc la boli si putrezire).

---

## 2. Calculul necesarului de apa (Modelul FAO-56)

In loc sa udam dupa un program fix, cantitatea de apa se calculeaza folosind formulele si tabelele din [baza de date FAO-56](../research/baza-de-date_fao-56.md):

$$ET_c = K_c \times ET_0$$

- **$ET_0$ (evapotranspiratia de referinta):** depinde de vreme (temperatura, umiditate relativa, vant si radiatie solara).
- **$K_c$ (coeficientul culturii):** difera in functie de planta si de stadiul ei de crestere (rasad, crestere, coacere).
- **Proprietatile solului ($FC$, $WP$, $TAW$):** ne spun cata apa poate retine pamantul in zona radacinilor inainte sa fie nevoie de o noua udare.

---

## 3. Masurarea volumului de apa fara costuri mari

Avem doua variante simple ca sa stim cata apa ajunge la plante fara sa punem senzori pe fiecare rand:

### Varianta A: Dozare pe baza de timp si calibrare ($V = Q \times t$)
- Nu necesita nicio piesa in plus (cost zero pentru senzori de debit).
- Masuram o singura data ce debit ($Q$) are pompa pe fiecare rand (de exemplu, cati mililitri curg intr-un minut) si calculam in cod cat timp ($t$) trebuie sa mearga pompa pentru a livra volumul dorit.
- **Dezavantaj:** daca scade nivelul apei din bazin sau scade tensiunea din baterie, debitul pompei se schimba si apare o mica eroare.

### Varianta B: Un singur debitmetru central pe iesirea pompei (Recomandat)
- Se monteaza un singur senzor de debit cu efect Hall (modelul **YF-S401**, potrivit pentru debite mici de `0.3 - 6 L/min`) chiar la iesirea din pompa, inainte de ramificatia catre randuri.
- Cand udam un rand, pompa merge pana cand senzorul numara exact impulsurile pentru volumul de apa calculat, indiferent daca randul este mai sus sau mai jos pe teren.
- In plus, sistemul isi da seama imediat daca s-a terminat apa din bazin sau daca s-a infundat un furtun.

---

## 4. Distributia apei pe randuri si diferenta de altitudine

Pe un teren in panta sau cu randuri puse la inaltimi diferite, presiunea scade cu aproximativ `0.1 bar` la fiecare metru diferenta de nivel ($\Delta P = \rho \cdot g \cdot \Delta h$). Randurile de mai sus primesc un debit mai mic decat cele de la baza.

### Optiuni pentru controlul randurilor:

1. **Cate o electrovalva pentru fiecare rand:**
   - Permite deschiderea individuala a fiecarui rand in functie de nevoile lui si de altitudine (randurile de mai sus pot fi tinute deschise mai mult timp pentru a compensa debitul mai mic).
   - **Atentie la alegerea valvelor:** electrovalva din lista de piese ([valva electromagnetica micro 12V](parts.md)) este de tip **Normal Deschis (NO)**. Asta inseamna ca lasa apa sa treaca atunci cand nu este alimentata si consuma curent (`120 mA` la `12V`) cat timp sta inchisa. Daca avem multe randuri, este mai eficient sa folosim valve **Normal Inchise (NC)** (care consuma curent doar pe randul care se uda) sau o valva doar pe randurile care trebuie oprite.

2. **Numar mic de valve / Redirectionarea fluxului cu un distribuitor rotativ:**
   - In loc sa cumparam cate o electrovalva pentru fiecare rand, putem folosi un **distribuitor rotativ (diverter multi-port)** printat 3D sau mecanic, actionat de un singur servomotor (ex. MG996R) sau motor pas cu pas (ex. 28BYJ-48).
   - Motorul roteste un canal interior catre furtunul randului care trebuie udat, iar apoi nu mai consuma energie.
   - *Nota:* distribuitoarele hidromecanice pasive din comert (care schimba singure canalul cand pornesti/opresti pompa) cer o presiune de minim `1.5 - 2 bar`, deci nu vor functiona cu o micro-pompa de `3-6V`.

---

## 5. Analiza pieselor actuale (`parts.md`)

Am verificat specificatiile pieselor propuse in [parts.md](parts.md) pentru a vedea cum se comporta impreuna:

| Componenta | Tensiune / Curent | Caracteristici principale | Observatii importante pentru proiect |
| :--- | :--- | :--- | :--- |
| **Pompa de apa submersibila** | `3 - 6V DC` (`130 - 220 mA`) | Debit: `80 - 120 L/h` (`1.3 - 2 L/min`), Inaltime maxima de pompare: `0.4 - 1.1 m` (`0.04 - 0.11 bar`), furtun `5 - 5.5 mm` | Are ciclu de lucru intermitent (`5 min PORNIT / 5 min OPRIT`). Nu poate ridica apa la o diferenta de nivel mai mare de `1 metru` si nu are presiune suficienta pentru picuratoare cu autocompensare comerciale (care cer peste `0.5 bar`). |
| **Electrovalva micro 12V** | `12V DC` (`120 mA`, `~1.44W`) | Tip: **Normal Deschis (NO)**, actionare directa (merge de la `0 bar` pana la `0.5 bar`), stuturi de `5.5 mm` | Se potriveste la furtun si la presiunea mica a pompei, dar fiind Normal Deschisa trebuie tinuta sub tensiune ca sa opreasca apa. |

---

## 6. Comunicatie LoRa si masurarea factorilor meteo (umiditate, vant)

Pentru terenuri intinse, unde nu avem semnal Wi-Fi, folosim noduri cu comunicare radio **LoRa** (consum foarte mic de baterie, raza de ordinul kilometrilor):

- **Nodurile din camp (LoRa):**
  - Masoara temperatura si umiditatea relativa a aerului (ex. senzor **SHT31** sau **BME280**) si umiditatea din sol (senzor capacitiv) pentru a verifica starea reala a pamantului.
  - Intra in mod *deep sleep* intre citiri si trimit datele la fiecare 15-30 de minute catre unitatea centrala.
- **Masurarea vantului si a radiatiei solare:**
  - **Varianta fizica:** se poate adauga un anemometru mecanic cu cupe (cu contact reed sau senzor Hall) pe un nod din camp, care numara rotatiile pentru a afla viteza vantului ($u_2$).
  - **Varianta hibrida / backup prin API meteo:** senzorii de vant ieftini se pot bloca in timp de la praf sau ploaie, iar senzorii de radiatie solara sunt scumpi. De aceea, unitatea centrala poate prelua automat viteza vantului, radiatia solara si chiar $ET_0$ calculat direct dintr-un serviciu meteo gratuit (cum este **Open-Meteo API**), combinand datele meteo cu citirile locale de umiditate primite prin LoRa.

---

## 7. Controlul electronic low-cost pentru pompa si electrovalve

Detaliile complete si comparatia de preturi se gasesc in [control-pompa-si-valve.md](../research/control-pompa-si-valve.md):

- **De ce evitam releele mecanice:** un releu obisnuit de `5V` consuma `70-80 mA` doar ca sa tina contactul inchis, se uzeaza mecanic si nu permite reglarea turatiei pompei.
- **Controlul pompei (`3-6V`):** folosim un tranzistor **MOSFET Logic-Level** (**AO3400**, **IRLZ44N** sau modul cu **D4184**, sub `5 lei`) impreuna cu o dioda antiparalel rapida **1N5819 (Schottky)**. Astfel nu avem pierderi de tensiune si putem regla debitul pompei din cod prin semnal PWM.
- **Controlul mai multor electrovalve (`12V`):** folosim un singur circuit integrat **ULN2003A** (`~3 lei`), care are 7 canale si diode de protectie incluse, putand comanda direct pana la 7 electrovalve de la pinii de `3.3V` sau `5V` ai microcontrolerului.

