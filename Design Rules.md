📄 DESIGN_RULES.md

Strikte ontwerp- en samenwerkingsregels (bindend)

ESP32 Testroom Controller
1. Fundamenteel principe

Eerst stabiliteit, dan features. Altijd.

Dit project is geen experimentele playground.

2. Code-discipline

❌ Geen volledige code dumps

❌ Geen refactors

❌ Geen “opkuis”

❌ Geen herstructurering

✅ Exact één wijziging per stap

✅ Elke wijziging moet:

compileerbaar

testbaar

revertbaar zijn

3. Ankerregel (absoluut verplicht)

Elke wijziging gebeurt via een letterlijke bestaande regel.

Formaat:

Zoek exact deze regel:
<letterlijke code>

Voeg DIRECT NA deze regel toe:
<nieuwe code>


❌ “Functioneel equivalent”
❌ “In setup() ergens”

➡️ Niet toegestaan

4. HTML & Web UI – extreem gevoelig
4.1 R"rawliteral()"

Volledige UI is rawliteral-gebaseerd

❌ Niet herschrijven

❌ Niet herformatteren

❌ Niet “even aanpassen”

5. AP-mode vs STA-mode (harde scheiding)
5.1 AP-mode (configuratie / reddingsmodus)

In AP-mode:

Webserver moet altijd responsief zijn

DNS captive portal actief

/settings moet altijd laden

❌ Geen sensor reads

❌ Geen blocking calls (pulseIn, lange delay)

❌ Geen mDNS

5.2 STA-mode (normale werking)

Sensor reads toegestaan

JSON updates actief

mDNS toegestaan (zie §6)

6. mDNS lifecycle (vastgelegd)

mDNS mag alleen starten als:

WiFi.status() == WL_CONNECTED

WiFi.localIP() != 0.0.0.0

Nooit:

in AP-mode

dubbel starten

starten op 0.0.0.0

7. Static IP – vaste regel

Static IP default = leeg

DHCP is standaard

Reden: eerdere reboot-storms en AP-loops.

8. Sensor reads – kritieke regel

Sensor reads moeten volledig uit in AP-mode

Geen uitzonderingen

Geen workarounds

➡️ Prioriteit #1
