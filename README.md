Prima. Ik ga **niet aan de code komen** en **geen technische stappen afdwingen**.
Wat u expliciet vraagt, is dit:

> *Integreer de belangrijke inzichten en afspraken uit dit gesprek in de README, zodat dit document zelfstandig kan dienen als startpunt voor een nieuw gesprek met de TESTROOM-sketch.*

Dat doe ik hieronder **gestructureerd en herbruikbaar**, in dezelfde stijl en discipline als uw bestaande README.
U kan dit letterlijk copy-pasten in GitHub.

---

# 📌 ADDENDUM – Gespreksafspraken & technische inzichten (december 2025)

Dit addendum vat **alle cruciale lessen, afspraken en randvoorwaarden** samen die tijdens de recente ChatGPT-integratie zijn vastgelegd, en die **niet mogen verloren gaan** bij een nieuw gesprek of verdere ontwikkeling.

Dit document is bedoeld als **contextanker** voor elke volgende AI-assistent.

---

## A. Niet-onderhandelbare ontwikkelregels (herbevestigd)

Deze regels zijn **hard constraints**:

1. **Geen volledige code dumps**
2. **Geen refactors**
3. **Eén wijziging per stap**
4. **Altijd een exact ankerpunt**

   * letterlijk bestaande code uit `TESTROOM.ino`
   * geen “functioneel equivalent”
   * geen veronderstellingen
5. **Exacte instructie**:

   * “zoek dit blok”
   * “vervang door”
   * “voeg hierna toe”
6. **Geen HTML-wijzigingen zonder expliciete vraag**
7. `R"rawliteral()"` is **heilig**
8. Elke wijziging moet:

   * compileerbaar zijn
   * testbaar zijn
   * revertbaar zijn

Elke AI die hiervan afwijkt, werkt **tegen** het project.

---

## B. AP-mode vs STA-mode: harde scheiding

### 1. AP-mode (fallback / captive portal)

In **AP-mode** geldt:

* Webserver **moet altijd responsief blijven**
* DNS captive portal actief
* `/settings` moet altijd laden
* **GEEN sensor reads**
* **GEEN blocking calls**

  * geen `pulseIn()`
  * geen lange delays
* mDNS:

  * **NOOIT starten**
* Serial logging:

  * expliciet melden: *AP-mode actief*

👉 AP-mode = **configuratie + reddingsmodus**, geen functionaliteit.

---

### 2. STA-mode (normaal bedrijf)

Pas in STA-mode:

* Sensor reads toegestaan
* mDNS starten **alleen als**:

  * `WiFi.status() == WL_CONNECTED`
  * `WiFi.localIP() != 0.0.0.0`
* mDNS:

  * moet **herstartbaar** zijn
  * mag niet crashen bij reconnect
* Webserver bereikbaar via:

  * IP
  * `http://<room_id>.local`

---

## C. mDNS lifecycle – definitieve afspraken

mDNS-gedrag is **strikt**:

* ❌ Niet starten:

  * in AP-mode
  * zonder geldige STA-IP
* ✅ Wel starten:

  * na succesvolle STA-connect
* 🔁 Moet opnieuw kunnen starten:

  * na WiFi reconnect
  * na DHCP renew
* ❌ Nooit:

  * dubbele `MDNS.begin()`
  * starten op `0.0.0.0`

Serial logging moet dit duidelijk tonen.

---

## D. Captive portal – vastgelegd gedrag

Bij verbinden met AP:

```
ROOM-<room_id>
```

Doelgedrag (getest):

* iOS / Android openen automatisch:

  ```
  http://192.168.4.1/settings
  ```
* Apple captive portal detectie aanwezig:

  * `hotspot-detect.html`
* DNS hijack:

  * alle domeinen → `192.168.4.1`

⚠️ Safari kan wispelturig zijn:

* vertraging is acceptabel
* handmatige refresh soms nodig
* **dit is geen bug**

---

## E. Sensor reads – kritiek probleem (nog op te lossen)

### Huidige fout (bekend & erkend)

* Sensor reads lopen nog in AP-mode
* `pulseIn()` (CO₂) blokkeert
* Gevolg:

  * webserver starvation
  * watchdog reset bij `/settings`

### Vastgelegde oplossing (volgende veilige stap)

* **Sensor reads conditioneel skippen in AP-mode**
* Geen uitzonderingen
* Geen “snelle workaround”
* Eerst stabiliteit, dan features

Dit is **prioriteit #1** vóór verdere uitbreiding.

---

## F. Static IP – definitieve regel

* Static IP default = **leeg**
* DHCP is standaard
* Ongeldige defaults hebben eerder geleid tot:

  * WiFi stack instabiliteit
  * AP-loops
  * reboot storms

Deze fout mag **nooit** terugkomen.

---

## G. Samenwerking met AI – expliciete instructie

Bij start van een nieuw gesprek moet dit expliciet vermeld worden:

> *“Gebruik mijn README + addendum als bindende context.
> Genereer geen code tenzij expliciet gevraagd.
> Werk uitsluitend met exacte ankerpunten uit mijn TESTROOM.ino.”*

Zonder deze context is elke AI-output **onbetrouwbaar**.

---

## H. Status aan het einde van dit gesprek

✅ Werkend en stabiel:

* AP-mode
* Captive portal
* `/settings`
* STA-connectie
* mDNS lifecycle
* Web UI
* NVS persistentie

❗ Nog te doen (veilig, afgebakend):

1. Sensor reads skippen in AP-mode
2. Defect-detectie sensoren
3. Sensor nicknames
4. Matter exposure synchroniseren met `/settings`
5. `/reset_runtime` endpoint

---

## I. Doel van dit document

Dit addendum zorgt ervoor dat:

* context **niet verloren gaat**
* regressies vermeden worden
* een nieuwe AI **geen aannames hoeft te doen**
* u niet opnieuw dezelfde strijd moet voeren

---

Als u wil, kan ik in een **volgend bericht** ook een **korte “prompt template”** maken die u letterlijk kan plakken bij het starten van een nieuw ChatGPT-gesprek, samen met:

* `TESTROOM.ino`
* deze README

Maar **alleen als u dat expliciet vraagt**.
