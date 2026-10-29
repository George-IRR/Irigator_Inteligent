În agronomie nu se folosește o valoare fixă statică (ex: „o roșie are nevoie de 2 litri de apă pe zi”), deoarece consumul depinde de stadiul fenologic, condițiile atmosferice și tipul de sol. Necesarul de apă al culturii ($ET_c$) se calculează standard prin relația:

$$ET_c = K_c \times ET_0$$

Unde $ET_0$ este evapotranspirația de referință (calculată din date meteo), iar $K_c$ este coeficientul specific culturii.

Există mai multe baze de date și standarde publice care oferă exact acești parametri:

---

### 1. Baza de date FAO-56 (Standardul global de referință)

Documentul **FAO Irrigation and Drainage Paper No. 56** (elaborat de Organizația pentru Alimentație și Agricultură a ONU) conține tabelele standard utilizate în toate sistemele industriale de irigații pentru peste 100 de tipuri de culturi.

Datele furnizate per cultură includ:

* **$K_c$ pe 3 faze de dezvoltare:** $K_{c\_ini}$ (faza inițială / răsărire), $K_{c\_mid}$ (dezvoltare vegetativă completă / înflorire), $K_{c\_end}$ (coacere / recoltare).
* **Durata fiecărui stadiu (în zile):** $L_{ini}$, $L_{dev}$, $L_{mid}$, $L_{late}$.
* **Adâncimea rădăcinii ($Z_r$ în metri):** minimă și maximă (ex: 0.2m la răsad, 0.7–1.2m la maturitate).
* **Factorul de epuizare critică ($p$ sau MAD – Management Allowed Depletion):** fracția din apa ușor accesibilă care poate fi extrasă de plantă înainte de a intra în stres hidric (valori tipice între 0.3 și 0.6).
* **Înălțimea maximă a plantei ($h$ în metri):** necesară pentru corecții micrometeorologice de vânt și umiditate.

Aceste date sunt disponibile public în format structurat în fișierele programului **CROPWAT 8.0 (FAO)** (extensia `.CRO`, fișiere text ușor parsabile).

---

### 2. Seturi de date deschise derivate (CSV / JSON)

Pentru integrare directă într-o bază de date sau script de calcul:

* **FAO-56 Crop Parameters în CSV / GitHub:** Există multiple repository-uri open-source care au extras tabelele FAO-56 în tabele curate (căutare: `fao56 crop coefficients csv` sau biblioteca Python `pyeto`).
* **Kaggle – Crop Water Requirement Datasets:** Conțin tabele corelate între cultură, tip de sol, temperatură și consum estimat de apă în milimetri/zi (căutare: `Crop Water Requirement and Irrigation Dataset`).
* **USDA-NRCS (National Resources Conservation Service) – Irrigation Guide:** Conține matrice de consum zilnic pe specii și tipuri de climă (consumptive use data).

---

### 3. Structura tipică a datelor (Exemplu tabelar)

Pentru un sistem de irigații, tabela de referință are următoarea structură:

| Cultură | $K_{c\_ini}$ | $K_{c\_mid}$ | $K_{c\_end}$ | Adâncime rădăcină ($Z_r$) | Factor epuizare ($p$) | Consum tipic ($ET_c$) |
| --- | --- | --- | --- | --- | --- | --- |
| **Tomate** | 0.60 | 1.15 | 0.80 | 0.7 – 1.5 m | 0.40 | 400 – 800 mm / sezon |
| **Ardei** | 0.60 | 1.05 | 0.90 | 0.5 – 1.0 m | 0.30 | 600 – 900 mm / sezon |
| **Cartof** | 0.50 | 1.15 | 0.75 | 0.4 – 0.6 m | 0.35 | 500 – 700 mm / sezon |
| **Porumb** | 0.30 | 1.20 | 0.35 | 1.0 – 1.7 m | 0.55 | 500 – 800 mm / sezon |
| **Gazon (Cool season)** | 0.90 | 0.95 | 0.85 | 0.15 – 0.3 m | 0.50 | 3 – 8 mm / zi (funcție de $ET_0$) |

---

### 4. Baze de date complementare necesare pentru calcul

Necesarul de apă nu poate fi aplicat corect fără al doilea dataset critic: **proprietățile hidrice ale solului**.

Sursa standard este **SoilGrids (ISRIC)** sau tabelele USDA de textură a solului:

* **Capacitatea de câmp ($FC$ – Field Capacity):** conținutul volumetric de apă la care solul este saturat util (ex: 15% în nisip, 35% în argilă).
* **Punctul de ofilire permanentă ($WP$ – Wilting Point):** pragul sub care planta moare (ex: 6% în nisip, 20% în argilă).
* **Apa disponibilă totală ($TAW$):** $TAW = 1000 \times (FC - WP) \times Z_r$ (exprimată în mm).