#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";

String ssidWifi;
String passwordWifi;

int ssidIndex = 0;
int passwordIndex = 33;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    .button { 
      background-color: #4CAF50; 
      border: none; 
      color: white; 
      padding: 16px 40px;
      text-decoration: none; 
      font-size: 30px; 
      margin: 2px; 
      cursor: pointer;
    }
    .button2 {
      background-color: #555555;
    } 
    .input {
      font-size: 20px; 
      margin-bottom: 10px;
    }
    .wifi-form { 
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      justify-content: space-around; 
    }
  </style>
</head>
<body>
  <div class="wifi-form">
    <h1>WiFi</h1>
    <input id="ssid" class="input" type="text" maxlength="32">
    <input id="password" class="input" type="password">
    <button onclick="connectToWifi()">Connect</button>
  </div>
</body>
<script>
  function connectToWifi() { 
    var ssid = document.getElementById("ssid").value; 
    var password = document.getElementById("password").value; 
    var xhr = new XMLHttpRequest(); xhr.open("POST", "/", true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(ssid + ":" + password);
  }
</script>
</html>)rawliteral";


AsyncWebServer server(80);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  EEPROM.begin(128);

  ssidWifi = readEEPROM(ssidIndex);
  passwordWifi = readEEPROM(passwordIndex);

  if (ssidWifi.length() > 0 && passwordWifi.length() > 0 && WiFi.status() != WL_CONNECTED) {
    WiFi.softAPdisconnect(true);
    server.end();
    WiFi.begin(ssidWifi, passwordWifi);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
  }
  else 
  {
    createAccessPoint();
  }
}

void loop() {
  if (ssidWifi.length() > 0 && passwordWifi.length() > 0 && WiFi.status() != WL_CONNECTED) {
    WiFi.softAPdisconnect(true);
    server.end();
    WiFi.begin(ssidWifi, passwordWifi);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
  }
}

void createAccessPoint() {
  WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Print ESP8266 Local IP Address
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
          ssidWifi = "";
          passwordWifi = "";        
          String res((char *)data);
          int sepIndex = -1;
          for (int i = 0; i < len; ++i) {
            if (res[i] != ':') {
              if (sepIndex == -1) {
                ssidWifi.concat(res[i]);
              }
              if (i > sepIndex && sepIndex != -1) {
                passwordWifi.concat(res[i]);
              }
            }
            else {
              sepIndex = i;
            }
          }
          writeEEPROM(ssidWifi, ssidIndex);
          writeEEPROM(passwordWifi, passwordIndex);
          request->send(200, "text/plain", "SUCCESS");
    });
    server.begin();
}

String readEEPROM(int address) {
  uint8_t len = EEPROM.read(address);
  if (len == 255) return "";
  String res;  
  for (int i = address + 1; i < len + address + 1; ++i) {
    char c = (char)EEPROM.read(i);
    res.concat(c);
  }
  return res;
}

void writeEEPROM(String value, int address) {
  int len = value.length();
  Serial.println("writeEEPROM:" + value);
  EEPROM.put(address, len);
  for (int i = address + 1; i < len + address + 1; ++i) {
    EEPROM.put(i, value[i - address - 1]);
  }
  EEPROM.commit();
}
