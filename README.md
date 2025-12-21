# ESP32_ZARLAR
Migratie van Photon naar ESP32: Directe integratie met Apple Homekit.

### SAMENVATTING ESP32 Testroom Controller Project (december 2025)
**Huidige status (wat werkt perfect):**

Volledige webinterface met mooie layout (header, sidebar, warning box, hints, table-stijl)

/settings pagina met alle velden:
  - Room naam, WiFi SSID/pass, static IP
  - HVAC defaults (heating setpoint default, vent request default, dew margin, home mode default, LDR dark threshold, beam alert threshold)
  - Optionele sensoren (checkboxes: CO₂, Dust, Sun, MOV2, TSTAT, Beam) – **persistent na reboot**
  - NeoPixel defaults (aantal pixels 1-30, RGB)
  - Factory Reset knop
  - JavaScript validatie (IP-formaat, verplichte velden, confirm)
  - Checkboxes werken betrouwbaar via method="get" + request->hasArg("naam") (geen hidden fields, geen POST nodig)
  - Persistent gemaakt (na power-off/on reboot):
  -   bed (AAN/UIT)
  -   heating_setpoint (huidige gekozen temperatuur)
  -   fade_duration (dim-snelheid 1-10s)
  -   home_mode (Thuis/Uit)
  - Keuze van Optional sensors (6) in /settings bepaalt welke related data getoond worden in UI, Serial en Matter. (JSON toont alles, altijd)
  - Andere states vallen bewust terug naar defaults bij reboot (vent_percent AUTO, pixels uit/AUTO, heating_mode AUTO, vent_mode AUTO)
  - Alle andere functionaliteit intact: OTA, JSON endpoint, NeoPixel kleurkiezer, sliders/toggles met live update, serial output, etc.


**Belangrijke technische keuzes (ter info voor continuïteit):**
- NVS wordt gebruikt voor alle permanente instellingen
- Checkboxes werken via method="get" + hasArg("naam") → simpel en betrouwbaar
- Runtime persistent states: bed, heating_setpoint, fade_duration, home_mode
- Veiligheid: kritieke automatische systemen (vent, pixels, heating/vent mode) vallen terug naar defaults bij reboot
- Web UI: mooie stijl met R"rawliteral()", sidebar, responsive table

**Hoe verder:**
- OP sensors ook uitgeschakeld in Matter!

**Eventuele nice-to-haves:**
   - Bed-switch en andere persistent states ook beschikbaar maken in Matter (als je dat verder uitbreidt)
   - Mogelijk een /reset_runtime endpoint om persistente states te wissen zonder factory reset
   - Verbeterde foutafhandeling bij sensoren (bijv. "sensor defect" melding in UI)
