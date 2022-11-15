/* Solar powered christmas lights
 * Upon boot (either reset or wake up from sleep), do the following:
 * 1. Check Vbat
 * 2. Connect to wifi and send Vbat to server
 * 3a. If connection problem or no response, sleep 30 min
 * 3b. If received update command, start ESP8266HTTPUpdateServer and wait forever 
 * 3c. If not should_turn_on(time, Vbat), sleep 30 min
 * 3d. If should_turn_on(time, Vbat), turn on light show until Vbat < 3.5 or 1am, whichever is earlier. Send Vbat to server every 10min
 * 
 * should_turn_on(time, Vbat): 7pm < time < 11pm and Vbat > 3.7V
 * should_turn_off(time, Vbat): Vbat < (time - 10pm) * 0.5V / 2h + 3.5V
 * should_turn_off: turn off after 10pm if Vbat=3.5V; turn off after midnight if Vbat=4V; 
 * 
 * ESP -> server packet: MAGIC1, nounce, vbat, status(1:boot, 2:led on, 3:ota)
 * server -> ESP packet: MAGIC2, nounce ack, unix timestamp, do_ota
 */

//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Adafruit_NeoPixel.h>
#include "int8lib.h"

#define CONFIG_STASSID "my-wifi-ssid"
#define CONFIG_STAPSK  "my-wifi-password"
#define CONFIG_SERVER_IP "192.168.1.100"
#define CONFIG_SERVER_PORT 12345
#define CONFIG_CLIENT_PORT 54321
#define CONFIG_DATA1_PIN   13
#define CONFIG_DATA2_PIN   12
#define CONFIG_POWER_PIN   4
#define CONFIG_NUM_LEDS    50
#define VBAT_3V5    667
#define VBAT_3V7    705
#define VBAT_4V0    763

static const uint8_t PROGMEM sine_gamma_table[256] = {
  54,57,60,63,67,70,73,77,80,84,88,91,95,99,103,107,
  111,115,119,123,128,132,136,140,145,149,153,157,162,166,170,174,
  178,182,186,190,194,198,202,206,209,213,216,219,222,226,228,231,
  234,236,239,241,243,245,247,248,250,251,252,253,254,255,255,255,
  255,255,255,255,254,253,252,251,250,248,247,245,243,241,239,236,
  234,231,228,226,222,219,216,213,209,206,202,198,194,190,186,182,
  178,174,170,166,162,157,153,149,145,140,136,132,128,123,119,115,
  111,107,103,99,95,91,88,84,80,77,73,70,67,63,60,57,
  54,51,49,46,43,41,38,36,34,32,30,28,26,24,23,21,
  19,18,17,15,14,13,12,11,10,9,8,7,7,6,5,5,
  4,4,3,3,3,2,2,2,1,1,1,1,1,1,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,1,1,1,1,1,2,2,2,3,3,3,4,
  4,5,5,6,7,7,8,9,10,11,12,13,14,15,17,18,
  19,21,23,24,26,28,30,32,34,36,38,41,43,46,49,51};
static inline uint8_t sin_gamma(uint8_t n) {return pgm_read_byte(&sine_gamma_table[n]);}

static uint32_t boot_tod; // time of day at boot. only valid if is_ota=false
static unsigned long last_server_check = 0;
static bool is_ota; // either light show or ota
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
Adafruit_NeoPixel *pixels1 = NULL;
Adafruit_NeoPixel *pixels2 = NULL;
unsigned long last_update_ms1 = 0;
unsigned long last_update_ms2 = 0;


static inline uint8_t get_phase(unsigned long ts, unsigned int pos, unsigned int period_ms, uint8_t period_pos) {
  return ts / (period_ms / 256) + pos * (256 / period_pos);
}
static void show_rgb(unsigned long ts)
{
  if (min(last_update_ms1, last_update_ms2) + 10 > ts) // 100 fps should be good enough
    return;
  bool is1 = last_update_ms1 < last_update_ms2;
  (is1 ? last_update_ms1 : last_update_ms2) = ts;
  Adafruit_NeoPixel *pixels = is1 ? pixels1 : pixels2;

  for (int i = 0; i < CONFIG_NUM_LEDS; i ++) {
    unsigned int xts = ts + (is1 ? 0 : 10000);
    uint8_t r = sin_gamma(get_phase(xts, i, 3000, 40)) / 4;  // period is 3 sec
    uint8_t g = sin_gamma(get_phase(xts, i, 4854, 47)) / 4;  // period is 4.854 sec
    uint8_t b = sin_gamma(get_phase(xts, i, 7853, 57)) / 4;  // period is 7.853 sec
    pixels->setPixelColor(i, r, g, b);
  }
  /*uint32_t status_color = 0;
  switch(WiFi.status()) {
    case WL_CONNECTED: status_color = pixels1->Color(50, 0, 0); break;
    case WL_CONNECT_FAILED: status_color = pixels1->Color(0, 50, 0); break;
    case WL_CONNECTION_LOST: status_color = pixels1->Color(0, 0, 50); break;
    case WL_DISCONNECTED: status_color = pixels1->Color(0, 0, 0); break;
    default: status_color = pixels1->Color(50, 50, 50);
  }
  pixels1->setPixelColor(0, status_color);*/
  pixels->show();
}

static void show_sparkle_smooth(unsigned long ts)
{
  const uint8_t FPS = 50;
  bool is1 = last_update_ms1 < last_update_ms2;
  if ((is1 ? last_update_ms1 : last_update_ms2) + 1000/FPS > ts)
    return;
  (is1 ? last_update_ms1 : last_update_ms2) = ts;
  Adafruit_NeoPixel *pixels = is1 ? pixels1 : pixels2;

  for (int i = 0; i < CONFIG_NUM_LEDS; i ++) {
    const unsigned int id = i * 2 + (is1 ? 1 : 0);
    const unsigned int period = (id * 12373u) % 20000 + 10000; // a random time period in [10, 30) seconds
    const unsigned int phase = (ts + id * 7639u) % period;
    const uint8_t intensity = phase * 4 < period ? sin_gamma((phase * 1024u / period + 192) % 256) / 2 : 0;
    pixels->setPixelColor(i, intensity, intensity * 3 / 4, intensity / 2);
  }
  pixels->show();
}

static void show_sparkle(unsigned long ts)
{
  const uint8_t FPS = 50;
  const uint8_t MAX_SPARKLES = 20;
  const uint8_t SPARKING_PROB = 10; // 1/SPARKING_PROB chance to generate a sparkle
  static uint8_t sparkle1[MAX_SPARKLES*4]; // brightness, hue, fading speed, position
  static uint8_t sparkle2[MAX_SPARKLES*4];
  bool is1 = last_update_ms1 < last_update_ms2;
  if ((is1 ? last_update_ms1 : last_update_ms2) + 1000/FPS > ts)
    return;
  (is1 ? last_update_ms1 : last_update_ms2) = ts;
  Adafruit_NeoPixel *pixels = is1 ? pixels1 : pixels2;
  uint8_t *sparkle = is1 ? sparkle1 : sparkle2;

  // Step 1. fade sparkle
  for (int i = 0; i < MAX_SPARKLES; i ++) {
    if (sparkle[i*4] == 0)
      continue;
    // We substract brightness by to_fade/256. Need to use probability to workout the fraction part.
    const unsigned int to_fade = sparkle[i*4] * (unsigned int)sparkle[i*4+2];
    const uint8_t to_fade_high = to_fade >> 8;
    const uint8_t to_fade_low = to_fade & 0xff;
    if (random8() < to_fade_low) {
      sparkle[i*4] = qsub8(sparkle[i*4], to_fade_high + 1);
    } else {
      sparkle[i*4] = qsub8(sparkle[i*4], to_fade_high);
    }
  }
  // Step 2. generate a random sparkle
  if (random8(SPARKING_PROB) == 0) {
    for (int i = 0; i < MAX_SPARKLES; i ++) {
      if (sparkle[i*4])
        continue;
      sparkle[i*4] = random8(128, 255);
      sparkle[i*4+1] = random8();
      sparkle[i*4+2] = random8(8,32);
      sparkle[i*4+3] = random8(CONFIG_NUM_LEDS);
      break;
    }
  }
  // Step 3. show
  pixels->clear();
  for (int i = 0; i < MAX_SPARKLES; i ++)
    pixels->setPixelColor(sparkle[i*4+3], pixels->ColorHSV(sparkle[i*4+1] << 8, 255, sparkle[i*4]));
  pixels->show();
}

#define MAGIC1 0x18ceb591
#define MAGIC2 0xe1697166
// status: 1=boot, 2=light show, 3=ota, 4=web reboot, 5=low vbat/time sleep
static uint32_t check_server(int status, bool block) {
  WiFiUDP udp;
  if (!udp.begin(CONFIG_CLIENT_PORT)) {
    Serial.println("udp.begin() failed");
    return 0;
  }
  uint32_t msg[4];
  uint32_t reply[4];
  msg[0] = MAGIC1;
  msg[1] = (micros() << 16) ^ system_get_rtc_time();
  msg[2] = analogRead(A0);
  msg[3] = status;
  for (int nsent = 0; nsent < 3; nsent ++) {
    if (!udp.beginPacket(CONFIG_SERVER_IP, CONFIG_SERVER_PORT)) {
      Serial.println("udp.beginPacket() failed");
      goto out0;
    }
    if (!udp.write((const char *)msg, sizeof(msg))) {
      Serial.println("udp.write() failed");
      goto out0;
    }
    if (!udp.endPacket()) {
      Serial.println("udp.endPacket() failed");
      goto out0;
    }
    if (!block)
      goto out0;

    uint32_t ts = millis();
    do {
      int reply_size = udp.parsePacket();
      if (reply_size != sizeof(reply)) {
        yield();
        continue;
      }
      reply[0] = 0;
      udp.read((char *)reply, sizeof(reply));
      if (reply[0] != MAGIC2 || reply[1] != msg[1]) {
        yield();
        continue;
      }
      uint32_t retval = reply[3] ? 1 : reply[2];
      udp.stop();
      return retval;
    } while (millis() - ts < 5000);
  }
  Serial.println("check_server timed out");
out0:
  udp.stop();
  return 0;
}

void stop_led() {
  if (pixels1) {
    delete pixels1;
    pixels1 = NULL;
  }
  if (pixels2) {
    delete pixels2;
    pixels2 = NULL;
  }
  pinMode(CONFIG_DATA1_PIN, INPUT);
  pinMode(CONFIG_DATA2_PIN, INPUT);
  delay(1);
  digitalWrite(CONFIG_POWER_PIN, LOW);
}

void start_led() {
  digitalWrite(CONFIG_POWER_PIN, HIGH);
  delay(1);
  // constructor will set pin to OUTPUT
  pixels1 = new Adafruit_NeoPixel(CONFIG_NUM_LEDS, CONFIG_DATA1_PIN);
  pixels1->begin();
  pixels2 = new Adafruit_NeoPixel(CONFIG_NUM_LEDS, CONFIG_DATA2_PIN);
  pixels2->begin();
}

void handle_status () {
  String content = "<html><head><title>ESP8266 LED</title></head><body>";
  int reading = analogRead(A0);
  content += "Voltage : ";
  content += reading * (3.530 / 673);
  content += " (";
  content += reading;
  content += ")\n</body></html>\r\n";
  server.send(200, "text/html", content);
}

void handle_reboot () {
  stop_led();
  check_server(4, false);
  ESP.restart();
}

/* sync_tod: sets the internal variable boot_tod.
 * unixts: the current UNIX timestamp. */
static void sync_tod (uint32_t unixts) {
  unixts -= millis()/1000;
  boot_tod = (unixts + 8 * 3600) % (24 * 3600);
}

/* Returns local time of day in seconds. sync_tod must be called before.
 * e.g. return 0 if it's midnight SGT
 * e.g. return 19 * 3600 if it's 7pm SGT */
static uint32_t get_tod () {
  return (boot_tod + millis()/1000) % (24 * 3600);
}

void setup(void) {
  Serial.begin(74880);
  Serial.setDebugOutput(true);

  pinMode(CONFIG_POWER_PIN, OUTPUT);
  digitalWrite(CONFIG_POWER_PIN, LOW);
  pinMode(CONFIG_DATA1_PIN, INPUT);
  pinMode(CONFIG_DATA2_PIN, INPUT);

  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG_STASSID, CONFIG_STAPSK);

  while (!WiFi.isConnected()) {
    if (millis() > 20000) { // 20 seconds connection timeout
      Serial.println("wifi connect timed out");
      ESP.deepSleep(60 * 60 * 1000000UL); // sleep 60 min
    }
    yield();
  }

  uint32_t server_time = check_server(1, true);
  Serial.print("check_server return ");
  Serial.println(server_time);
  last_server_check = millis();
  if (server_time == 0) { // no response
    ESP.deepSleep(30 * 60 * 1000000UL); // sleep 30 min
  } else if (server_time == 1) { // OTA update
    is_ota = true;
    httpUpdater.setup(&server, "/66fbfce8-flasher");
  } else {
    int vbat = analogRead(A0);
    if (vbat < VBAT_3V7)
      ESP.deepSleep(30 * 60 * 1000000UL);
    sync_tod(server_time);
    uint32_t tod = get_tod();
    uint32_t time_to_7pm = ((19 + 24) * 3600 - tod) % (24 * 3600); // if it's 8pm in Singapore , time_after_7pm = 3600.
    if (time_to_7pm < 30 * 60) { // less than 30 minutes left
      ESP.deepSleep((time_to_7pm + 1) * 1000000UL);
    } else if (time_to_7pm < 20 * 3600) { // after 11pm
      ESP.deepSleep(30 * 60 * 1000000UL); // sleep 30 min
    }
    is_ota = false;
    start_led();
  }
  server.on("/66fbfce8-status", handle_status);
  server.on("/66fbfce8-reboot", handle_reboot);
  server.begin();
}

/* pre condition: is_ota = false */
bool should_turn_off () {
  uint32_t tod = get_tod();
  if (tod <= 18 * 3600)
    return true;
  unsigned int vbat = analogRead(A0);
  if (vbat <= VBAT_3V5)
    return true;
  if (tod <= 22 * 3600)
    return false;
  return (tod - 22 * 3600) * (VBAT_4V0 - VBAT_3V5) > (vbat - VBAT_3V5) * 2 * 3600;
}

void loop(void) {
  const unsigned long ts = millis();
  static uint16_t loop_count = 0; // The checks dont need to be performed every time.
  loop_count ++;
  if (loop_count == 30000 && ts - last_server_check > 10 * 60 * 1000UL) {
    check_server(is_ota ? 3 : 2, false);
    last_server_check = ts;
  }
  if ((loop_count & 0xf) == 3)
    server.handleClient();

  if (!is_ota) {
    if (loop_count == 60000) {
      static uint8_t low_vbat_count = 0; // There is noise in ADC, so we count 3 consecutive low vbat.
      low_vbat_count = should_turn_off() ? low_vbat_count + 1 : 0;
      if (low_vbat_count >= 3) {
        stop_led();
        check_server(5, false);
        ESP.deepSleep(30 * 60 * 1000000UL); // sleep 30 min
      }
    }

    uint8_t show_case = ts / (15 * 60 * 1000UL) % 3;
    switch (show_case) {
      case 0: show_sparkle(ts); break;
      case 1: show_sparkle_smooth(ts); break;
      case 2: show_rgb(ts); break;
    }
  }
}
