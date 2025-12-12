// ESP32_TESTROOM_GPT.ino = Transition from Photon based to ESP32 based Home automation system
// Developed together with ChatGPT in december '25.
// Bereikbaar op http://testroom.local of http://http://192.168.1.36 => Andere controller: Naam (sectie DNS/MDNS) + static IP aanpassen!
// 12dec25: VOLLEDIG AFGEWERKT & BETROUWBAAR. Alles intact + complete webpagina's + OTA met serial feedback

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>  // Voor OTA
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_NeoPixel.h>

#define DHT_PIN       18
#define ONE_WIRE_PIN   4
#define PIR_MOV1      17
#define PIR_MOV2      26
#define SHARP_LED     19
#define SHARP_ANALOG  32
#define LDR_ANALOG    33
#define CO2_PWM       25
#define TSTAT_PIN     27
#define OPTION_LDR    14
#define NEOPIXEL_PIN  16
#define NEOPIXEL_NUM   8

DHT dht(DHT_PIN, DHT22);
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);

const char* WIFI_SSID = "Delannoy";
const char* WIFI_PASS = "kampendaal,34";

// Sensor variabelen
float temp_dht = 0, temp_ds = 0, humi = 0, dew = 0;
int light_ldr = 0, sun_light = 0, dust = 0, co2 = 0;
int tstat_on = 0, mov1_triggers = 0, mov2_triggers = 0;
int mov1_light = 0, mov2_light = 0, beam_alert = 0, beam_light = 0;
unsigned long uptime_sec = 0;

// PIR op 3.3V: beweging = LOW
unsigned long mov1_off_time = 0;
unsigned long mov2_off_time = 0;
const unsigned long LIGHT_ON_DURATION = 30000;
const int LDR_DARK_THRESHOLD = 40;

// Indicator
bool recent_mov1 = false;
bool recent_mov2 = false;
unsigned long last_indicator_reset = 0;

#define MOV_WINDOW 60000
#define MOV_BUF_SIZE 50
unsigned long mov1Times[MOV_BUF_SIZE] = {0};
unsigned long mov2Times[MOV_BUF_SIZE] = {0};

// Fade engine (blijft voor toekomst)
uint8_t currR[NEOPIXEL_NUM], currG[NEOPIXEL_NUM], currB[NEOPIXEL_NUM];
uint8_t targetR[NEOPIXEL_NUM], targetG[NEOPIXEL_NUM], targetB[NEOPIXEL_NUM];
unsigned long lastFadeStep = 0;
const unsigned long FADE_INTERVAL_MS = 15;
const uint8_t FADE_STEP = 4;

void initFadeEngine() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    uint32_t c = pixels.getPixelColor(i);
    currR[i] = (c >> 16) & 0xFF;
    currG[i] = (c >> 8)  & 0xFF;
    currB[i] = c & 0xFF;
    targetR[i] = currR[i]; targetG[i] = currG[i]; targetB[i] = currB[i];
  }
}

void setTargetColor(int idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < 0 || idx >= NEOPIXEL_NUM) return;
  targetR[idx] = r; targetG[idx] = g; targetB[idx] = b;
}

void updateFades() {
  unsigned long now = millis();
  if (now - lastFadeStep < FADE_INTERVAL_MS) return;
  lastFadeStep = now;
  bool changed = false;
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    // R, G, B fade logica (identiek aan jouw versie)
    if (currR[i] < targetR[i]) { uint8_t delta = targetR[i] - currR[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currR[i] += step; changed = true; }
    else if (currR[i] > targetR[i]) { uint8_t delta = currR[i] - targetR[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currR[i] -= step; changed = true; }
    if (currG[i] < targetG[i]) { uint8_t delta = targetG[i] - currG[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currG[i] += step; changed = true; }
    else if (currG[i] > targetG[i]) { uint8_t delta = currG[i] - targetG[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currG[i] -= step; changed = true; }
    if (currB[i] < targetB[i]) { uint8_t delta = targetB[i] - currB[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currB[i] += step; changed = true; }
    else if (currB[i] > targetB[i]) { uint8_t delta = currB[i] - targetB[i]; uint8_t step = (delta < FADE_STEP) ? delta : FADE_STEP; currB[i] -= step; changed = true; }
    pixels.setPixelColor(i, currR[i], currG[i], currB[i]);
  }
  if (changed) pixels.show();
}

// Helpers (identiek)
void pushEvent(unsigned long *buf, int size) {
  unsigned long now = millis();
  for (int i = 0; i < size; i++) {
    if (buf[i] == 0 || (now - buf[i] > MOV_WINDOW)) { buf[i] = now; return; }
  }
  int oldest = 0; unsigned long old = buf[0];
  for (int i = 1; i < size; i++) if (buf[i] < old) { old = buf[i]; oldest = i; }
  buf[oldest] = now;
}

int countRecent(unsigned long *buf, int size) {
  unsigned long now = millis(); int c = 0;
  for (int i = 0; i < size; i++) if (buf[i] && (now - buf[i] <= MOV_WINDOW)) c++;
  return c;
}

float calculateDewPoint(float t, float h) { return isnan(t) || isnan(h) ? 0 : t - ((100 - h) / 5.0); }
int scaleLDR(int r) { return map(constrain(r, 100, 3800), 100, 3800, 100, 0); }
int readDust() { digitalWrite(SHARP_LED, LOW); delayMicroseconds(280); int v = analogRead(SHARP_ANALOG); delayMicroseconds(40); digitalWrite(SHARP_LED, HIGH); delayMicroseconds(9680); return v; }
int readCO2() { unsigned long h = pulseIn(CO2_PWM, HIGH, 2000000); unsigned long l = pulseIn(CO2_PWM, LOW, 2000000); return (h < 100 || l < 100) ? 0 : (int)(5000.0 * (h - 2.0) / (h + l - 4.0)); }

String getJSON() {
  String det = recent_mov1 ? (recent_mov2 ? "MOV1 MOV2" : "MOV1") : (recent_mov2 ? "MOV2" : "-");
  return "{\"a\":" + String(co2) +
         ",\"b\":" + String(dust) +
         ",\"c\":" + String(dew,1) +
         ",\"d\":" + String((int)round(humi)) +
         ",\"e\":" + String(light_ldr) +
         ",\"f\":" + String(sun_light) +
         ",\"g\":" + String(temp_dht,1) +
         ",\"h\":" + String(temp_ds,1) +
         ",\"i\":" + String(mov1_triggers) +
         ",\"j\":" + String(mov2_triggers) +
         ",\"k\":" + String(tstat_on) +
         ",\"l\":" + String(mov1_light) +
         ",\"m\":" + String(mov2_light) +
         ",\"n\":" + String(beam_alert) +
         ",\"o\":" + String(beam_light) +
         ",\"det\":\"" + det + "\"" +
         ",\"up\":" + String(uptime_sec) +
         ",\"v\":" + String(WiFi.RSSI()) +
         ",\"w\":" + String(constrain(2*(WiFi.RSSI()+100),0,100)) +
         ",\"x\":" + String((ESP.getFreeHeap()*100)/ESP.getHeapSize()) + "}";
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_MOV1, INPUT_PULLUP);  // Voor 3.3V HC-SR501: beweging = LOW
  pinMode(PIR_MOV2, INPUT_PULLUP);
  pinMode(SHARP_LED, OUTPUT); digitalWrite(SHARP_LED, HIGH);
  pinMode(TSTAT_PIN, INPUT_PULLUP);
  pinMode(OPTION_LDR, INPUT);

  dht.begin();
  ds18b20.begin();
  if (!tsl.begin()) Serial.println("TSL2561 niet gevonden");
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  pixels.begin(); pixels.clear(); pixels.show();
  initFadeEngine();

  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(192,168,1,36), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  if (MDNS.begin("testroom")) Serial.println("mDNS: http://testroom.local");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // === HOME PAGE ===
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<meta charset='utf-8'>"
                  "<body style='font-family:Arial;background:#222;color:#0f0;text-align:center;padding:50px'>"
                  "<h1>ESP32 Testroom</h1>"
                  "<p style='font-size:150%'>"
                  "<a href='/status' style='color:#0f0;margin:20px'>→ Status Rapport</a><br>"
                  "<a href='/status.json' style='color:#0f0;margin:20px'>→ JSON Data</a><br>"
                  "<a href='/update' style='color:#0f0;margin:20px'>→ OTA Update</a><br>"
                  "<a href='/reboot' style='color:#f00;margin:20px'>→ Reboot</a>"
                  "</p></body>";
    request->send(200, "text/html; charset=utf-8", html);
  });

  // === STATUS PAGE ===
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    uptime_sec = millis() / 1000;
    String det = recent_mov1 ? (recent_mov2 ? "MOV1 MOV2" : "MOV1") : (recent_mov2 ? "MOV2" : "-");

    String html = "<meta charset='utf-8'><body style='font-family:Arial;background:#111;color:#0f0;padding:20px'>"
                  "<h1>Testroom – " + String(uptime_sec) + " s</h1><pre style='font-size:120%'>";
    html += "DHT22 Temperatuur     : " + String(temp_dht, 2) + " °C\n";
    html += "DHT22 Luchtvochtigheid : " + String(humi, 1) + " %\n";
    html += "Dauwpunt             : " + String(dew, 1) + " °C\n";
    html += "DS18B20              : " + String(temp_ds, 2) + " °C\n";
    html += "Licht (TSL2561)      : " + String(sun_light) + " lux\n";
    html += "LDR (donker=100)     : " + String(light_ldr) + "\n";
    html += "Stof (Sharp)         : " + String(dust) + "\n";
    html += "CO₂                  : " + String(co2) + " ppm\n";
    html += "Thermostaat aan      : " + String(tstat_on ? "JA" : "NEE") + "\n";
    html += "MOV1 triggers/min    : " + String(mov1_triggers) + "\n";
    html += "MOV2 triggers/min    : " + String(mov2_triggers) + "\n";
    html += "MOV1 licht aan       : " + String(mov1_light ? "JA" : "NEE") + "\n";
    html += "MOV2 licht aan       : " + String(mov2_light ? "JA" : "NEE") + "\n";
    html += "Beam lichtstraal     : " + String(beam_alert ? "ONDERBROKEN" : "OK") + "\n";
    html += "WiFi RSSI            : " + String(WiFi.RSSI()) + " dBm\n";
    html += "WiFi kwaliteit       : " + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + " %\n";
    html += "Free heap            : " + String((ESP.getFreeHeap() * 100) / ESP.getHeapSize()) + " %\n";
    html += "Laatste beweging     : " + det + "\n";
    html += "</pre><br><a href='/'>Home</a> | <a href='/update'>OTA</a> | <a href='/reboot'>Reboot</a>";
    html += "<script>setTimeout(()=>location.reload(),3000);</script></body>";
    request->send(200, "text/html; charset=utf-8", html);
  });

  // === JSON ENDPOINT ===
  server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "application/json", getJSON());
  });

  // === OTA UPDATE PAGE ===
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<meta charset='utf-8'>"
                  "<body style='font-family:Arial;background:#222;color:#0f0;padding:40px;text-align:center'>"
                  "<h1>OTA Firmware Update</h1>"
                  "<p>Selecteer een .bin bestand (Export compiled Binary in Arduino IDE)</p>"
                  "<form method='POST' action='/update' enctype='multipart/form-data'>"
                  "<input type='file' name='update' accept='.bin'><br><br>"
                  "<input type='submit' value='Upload Firmware' style='font-size:120%;padding:10px'>"
                  "</form><br>"
                  "<a href='/'>← Home</a></body>";
    request->send(200, "text/html; charset=utf-8", html);
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool success = !Update.hasError();
    request->send(200, "text/html", success 
      ? "<h2 style='color:#0f0'>Update succesvol!</h2><p>Rebooting...</p>" 
      : "<h2 style='color:#f00'>Update mislukt!</h2><p>Probeer opnieuw.</p><a href='/update'>Terug</a>");
    if (success) { delay(1000); ESP.restart(); }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.println("\n=== OTA UPDATE GESTART ===");
      Serial.printf("Bestand: %s (%u bytes)\n", filename.c_str(), request->contentLength());
      Update.begin(UPDATE_SIZE_UNKNOWN);
    }
    size_t written = Update.write(data, len);
    if (written != len) Serial.printf("Fout bij schrijven: %u/%u\n", written, len);
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      lastPrint = millis();
      Serial.printf("Geüpload: %u/%u bytes\n", index + len, request->contentLength());
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("OTA succesvol: %u bytes\n", index + len);
        Serial.println("Rebooting...\n");
      } else {
        Serial.println("OTA fout: " + String(Update.errorString()));
      }
    }
  });

  // === REBOOT ===
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h2>Rebooting ESP32...</h2>");
    delay(500);
    ESP.restart();
  });

  server.begin();
  Serial.println("HTTP server gestart – http://testroom.local");
}

unsigned long lastSerial = 0;
unsigned long last_slow = 0;

void loop() {
  updateFades();

  // PIR op 3.3V: beweging = LOW → rising edge van HIGH naar LOW
  static bool last1 = HIGH, last2 = HIGH;
  bool p1 = digitalRead(PIR_MOV1);
  bool p2 = digitalRead(PIR_MOV2);

  if (!p1 && last1) { 
    mov1_off_time = millis() + LIGHT_ON_DURATION; 
    pushEvent(mov1Times, MOV_BUF_SIZE); 
    recent_mov1 = true; 
    last_indicator_reset = millis(); 
  }
  if (!p2 && last2) { 
    mov2_off_time = millis() + LIGHT_ON_DURATION; 
    pushEvent(mov2Times, MOV_BUF_SIZE); 
    recent_mov2 = true; 
    last_indicator_reset = millis(); 
  }

  last1 = p1; last2 = p2;
  if (millis() - last_indicator_reset > 2000) recent_mov1 = recent_mov2 = false;

  bool dark = (light_ldr > LDR_DARK_THRESHOLD);
  bool l1 = dark && (millis() < mov1_off_time);
  bool l2 = dark && (millis() < mov2_off_time);

  // Directe pixel aan/uit voor PIR
  pixels.setPixelColor(0, l1 ? pixels.Color(0, 220, 0) : 0);
  pixels.setPixelColor(1, l2 ? pixels.Color(0, 220, 0) : 0);
  pixels.show();

  mov1_light = l1;
  mov2_light = l2;

  if (millis() - last_slow < 2000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;

  // Sensoren (identiek)
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  ds18b20.requestTemperatures(); temp_ds = ds18b20.getTempCByIndex(0);
  sensors_event_t e; tsl.getEvent(&e); sun_light = (int)e.light;
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));
  dust = readDust();
  co2 = readCO2();
  tstat_on = !digitalRead(TSTAT_PIN);
  beam_alert = (analogRead(OPTION_LDR) < 1500) ? 1 : 0;
  beam_light = beam_alert;

  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
  mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);

  // Serial rapport (identiek)
  if (millis() - lastSerial > 3000) {
    lastSerial = millis();
    String det = recent_mov1 ? (recent_mov2 ? "MOV1 MOV2" : "MOV1") : (recent_mov2 ? "MOV2" : "-");

    Serial.println("\nTESTROOM – " + String(uptime_sec) + " s");
    Serial.println("─────────────────────────────────────");
    Serial.printf("DHT22 Temperatuur     : %.2f °C\n", temp_dht);
    Serial.printf("DHT22 Luchtvochtigheid : %.1f %%\n", humi);
    Serial.printf("Dauwpunt             : %.1f °C\n", dew);
    Serial.printf("DS18B20              : %.2f °C\n", temp_ds);
    Serial.printf("Licht (TSL2561)      : %d lux\n", sun_light);
    Serial.printf("LDR (donker=100)     : %d\n", light_ldr);
    Serial.printf("Stof (Sharp)         : %d\n", dust);
    Serial.printf("CO₂                  : %d ppm\n", co2);
    Serial.printf("Thermostaat aan      : %s\n", tstat_on ? "JA" : "NEE");
    Serial.printf("MOV1 triggers/min    : %d\n", mov1_triggers);
    Serial.printf("MOV2 triggers/min    : %d\n", mov2_triggers);
    Serial.printf("MOV1 licht aan       : %s\n", mov1_light ? "JA" : "NEE");
    Serial.printf("MOV2 licht aan       : %s\n", mov2_light ? "JA" : "NEE");
    Serial.printf("Beam lichtstraal     : %s\n", beam_alert ? "ONDERBROKEN" : "OK");
    Serial.printf("WiFi RSSI            : %d dBm\n", WiFi.RSSI());
    Serial.printf("WiFi kwaliteit       : %d %%\n", constrain(2 * (WiFi.RSSI() + 100), 0, 100));
    Serial.printf("Free heap            : %d %%\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    Serial.printf("Laatste beweging     : %s\n", det.c_str());
    Serial.println("─────────────────────────────────────\n");
  }
}
