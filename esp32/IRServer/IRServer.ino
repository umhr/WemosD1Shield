/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * Version 0.3 May, 2019
 * Version 0.2 June, 2017
 * Copyright 2015 Mark Szabo
 * Copyright 2019 David Conran
 *
 * An IR LED circuit *MUST* be connected to the ESP on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif  // ESP8266
#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif  // ESP32
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>
//
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 14     // Digital pin connected to the DHT sensor 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;
//
const char* kSsid = "TsukuNet";
const char* kPassword = "workingroom01";
MDNSResponder mdns;

#if defined(ESP8266)
ESP8266WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp8266"
#endif  // ESP8266
#if defined(ESP32)
WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp5"
#endif  // ESP32

#define LED1 26
#define LED2 25

const uint16_t kIrLed = 4;  // ESP GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

int heartbeatPin = 27; // The Arduino will send the heartbeat at this output pin.
uint16_t heartbeatCount = 0;
void heartbeat() {
  if(heartbeatCount%5000 == 0){
    Serial.print("HB = ");
    Serial.println(heartbeatCount);
    dhtloop();
    co2loop();
  }
  if(heartbeatCount%1000 == 0 && (WiFi.status() != WL_CONNECTED || int(WiFi.localIP()) == 0)){
    heartbeatCount = 1000;
  }
  if(heartbeatCount == 10){
    pinMode(heartbeatPin, OUTPUT);
    digitalWrite(heartbeatPin,LOW);
    digitalWrite(LED1, HIGH);
  }else if(heartbeatCount == 1000){
    digitalWrite(heartbeatPin, HIGH);
    pinMode(heartbeatPin, INPUT);
    digitalWrite(LED1, LOW);
  //}else if(heartbeatCount > 65500){
    //heartbeatCount = 0;
  }
  heartbeatCount ++;
}

void handleRoot() {
  digitalWrite(LED2, HIGH);
  server.send(200, "text/html",
              "<html>" \
                "<head><title>" HOSTNAME " Index </title>" \
                "<meta http-equiv=\"Content-Type\" " \
                    "content=\"text/html;charset=utf-8\">" \
                "<meta name=\"viewport\" content=\"width=device-width," \
                    "initial-scale=1.0,minimum-scale=1.0," \
                    "maximum-scale=5.0\">" \
                "</head>" \
                "<body>" \
                  "<h1>Hello from " HOSTNAME "</h1>" \
                  "<p><a href=\"ir?code=16769055\">Send 0xFFE01F</a></p>" \
                  "<p><a href=\"ir?code=16429347\">Send 0xFAB123</a></p>" \
                  "<p><a href=\"relay?r1=on\">Relay 1 on</a></p>" \
                  "<p><a href=\"relay?r1=off\">Relay 1 off</a></p>" \
                  "<p><a href=\"relay?r2=on\">Relay 2 on</a></p>" \
                  "<p><a href=\"relay?r2=off\">Relay 2 off</a></p>" \
                  "<p><a href=\"/data.json\">/data.json</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xFFA25D);\">ON 0xFFA25D</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xFFE21D);\">OFF 0xFFE21D</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xFF22DD);\">Flame 0xFF22DD</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xFF02FD);\">Light 0xFF02FD</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xFFC23D);\">Breath 0xFFC23D</a></p>" \
                  "<p><a href=\"javascript:sendIR(0xA90, 'sony');\">Sony 0xA90</a></p>" \
                  "<script>"\
                  "function sendIR(code, protocol){"\
                    "var xhr = new XMLHttpRequest();"\
                    "var url = './ir?code=' + code;"\
                    "url += protocol == undefined?'':'&protocol=' + protocol;"\
                    "console.log(code, protocol, url);"\
                    "xhr.open('GET', url);"\
                    "xhr.onreadystatechange = function (e) {console.dir(e);};"\
                    "xhr.send();"\
                  "}"\
                  "</script>"\
                "</body>" \
              "</html>");
    digitalWrite(LED2, LOW);
}

void handleIr() {
  String protocol = getQuery("protocol");
  if(protocol == "sony"){
    irsend.sendSony(strtoul(getQuery("code").c_str(), NULL, 0), 12);
    Serial.print("sony ");
  }else{
    irsend.sendNEC(strtoul(getQuery("code").c_str(), NULL, 0), 32);
    Serial.print("NEC ");
  }
  Serial.println(getQuery("code"));
  server.send(200, "text/plain", "ok");
}

String getQuery(String key) {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == key) {
      return server.arg(i);
    }
  }
  return "";
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

void handleRelay(){
  String protocol = getQuery("r1");
  if(protocol == "on"){
    digitalWrite(12, LOW);
  }else if(protocol == "off"){
    digitalWrite(12, HIGH);
  }
  protocol = getQuery("r2");
  if(protocol == "on"){
    digitalWrite(13, LOW);
  }else if(protocol == "off"){
    digitalWrite(13, HIGH);
  }
  server.send(200, "text/plain", "ok");
}

uint16_t indexnum =0;
float humidity;
float temperature;
int co2;

void setup(void) {
  pinMode(LED2, OUTPUT);
  pinMode(LED1, OUTPUT);
  irsend.begin();

  Serial.begin(115200);
  WiFi.begin(kSsid, kPassword);
  Serial.println("");
  Serial2.begin(9600);  

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    delay(100);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    delay(100);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());

#if defined(ESP8266)
  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
#else  // ESP8266
  if (mdns.begin(HOSTNAME)) {
#endif  // ESP8266
    Serial.println("MDNS responder started");
    // Announce http tcp service on port 80
    mdns.addService("http", "tcp", 80);
  }

  server.on("/", handleRoot);
  server.on("/ir", handleIr);
  server.on("/relay", handleRelay);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  server.on("/data.json", [](){
    digitalWrite(LED2, HIGH);
    indexnum ++;
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String str = "{";
    str += "\"n\":" + String(indexnum);
    str += ", \"humidity\":" + String(humidity);
    str += ", \"temperature\":" + String(temperature);
    str += ", \"co2\":" + String(co2);
    str += "}";
    server.send(200, "application/json", str);
    digitalWrite(LED2, LOW);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  dhtsetup();
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
}

void dhtsetup() {
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void dhtloop() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    temperature = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    humidity = event.relative_humidity;
  }
}

void co2loop() {
  byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  unsigned char response[9]; // for answer
  Serial2.write(cmd, 9); //request PPM CO2
  Serial2.readBytes(response, 9);
  if (response[0] != 0xFF){
    return;
  }
  if (response[1] != 0x86){
    return;
  }
  unsigned int responseHigh = (unsigned int) response[2];
  unsigned int responseLow = (unsigned int) response[3];
  co2 = (256 * responseHigh) + responseLow;
  Serial.print("CO2: ");
  Serial.print(co2);
  Serial.println(" ppm");
}

void loop(void) {

#if defined(ESP8266)
  mdns.update();
#endif
  server.handleClient();

  heartbeat();
}
