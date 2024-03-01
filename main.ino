#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>


#define FAN 27  // Dioda LED podłączona do pinu GPIO2 (D4 na ESP8266)
#define PUMP 22  // Dioda LED podłączona do pinu GPIO2 (D4 na ESP8266)
#define SPARK 25  // Dioda LED podłączona do pinu GPIO2 (D4 na ESP8266)

#define EEPROM_SIZE 40
#define REG_FAN1 0
#define REG_FAN2 4
#define REG_FAN3 8
#define REG_PUMPON_ONE 12
#define REG_PUMPOFF_ONE 16
#define REG_PUMPON_TWO 20
#define REG_PUMPOFF_TWO 24
#define REG_PUMPON_THREE 28
#define REG_PUMPOFF_THREE 32
#define REG_SPARKOFF 36


const char *ap_ssid = "ESP_AP";        // Nazwa sieci AP
const char *ap_password = "password";  // Hasło do sieci AP (opcjonalne)

const int freq = 5000;
const int ledChanel = 0;
const int resolution = 8;

bool fan1Enable = false;
bool fan2Enable = false;
bool fan3Enable = false;
int fanValue = 0;
int fan1Value = 0;
int fan2Value = 20;
int fan3Value = 100;
bool fanStop = false;


int pumpOnTime = 1000;
int pumpOffTime = 2000;
int pumpOnTime1 = 1000;
int pumpOffTime1 = 2000;
int pumpOnTime2 = 1000;
int pumpOffTime2 = 2000;
int pumpOnTime3 = 1000;
int pumpOffTime3 = 2000;
bool pumpStop = false;

bool isSparkButtonPressed = false; // Zmienna do śledzenia stanu przycisku
bool isSparkButtonOn = false;      // Zmienna do śledzenia stanu lampki
int sparkOffTimeout = 0;
bool sparkStop = false;
bool sparkStartFlag = true;
bool sparkStatusChanged = false;

unsigned long previousPumpMillis = 0;
unsigned long previousSparkMillis = 0;

unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
const long interval1 = 10; // Okres wywołania funkcji 1 w milisekundach (1 sekunda)
const long interval2 = 20; // Okres wywołania funkcji 2 w milisekundach (2 sekundy)
const long interval3 = 14; // Okres wywołania funkcji 3 w milisekundach (5 sekund)


AsyncWebServer server(80);

void setup() {
  // Inicjalizacja trybu AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  ledcSetup(ledChanel,freq,resolution);
  ledcAttachPin(FAN,ledChanel);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(PUMP, OUTPUT);  // set the LED pin mode
  pinMode(SPARK, OUTPUT);  // set the LED pin mode
  digitalWrite(PUMP, 0);
  digitalWrite(SPARK, 0);
 
  if(!SPIFFS.begin(true)){
    Serial.println("Błąd inicjalizacji SPIFFS");
    return;
  }
  // Wyświetlenie adresu IP AP
  Serial.begin(115200);
  Serial.println();
  Serial.print("Access Point SSID: ");
  Serial.println(ap_ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  fan1Value = EEPROM.readInt(REG_FAN1);
  fan2Value = EEPROM.readInt(REG_FAN2);
  fan3Value = EEPROM.readInt(REG_FAN3);
  pumpOnTime1 = EEPROM.readInt(REG_PUMPON_ONE );
  pumpOffTime1 = EEPROM.readInt(REG_PUMPOFF_ONE);
  pumpOnTime2 = EEPROM.readInt(REG_PUMPON_TWO );
  pumpOffTime2 = EEPROM.readInt(REG_PUMPOFF_TWO);
  pumpOnTime3 = EEPROM.readInt(REG_PUMPON_THREE );
  pumpOffTime3 = EEPROM.readInt(REG_PUMPOFF_THREE);
  sparkOffTimeout = EEPROM.readInt(REG_SPARKOFF);
  
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "";
    html += "<html>";
    html += "  <head>";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "    <meta name=\"viewport\" content=\"height=device-height, initial-scale=1\">";
    html += "    <style>";
    html += "      body {";
    html += "        background-size: cover;";
    html += "        background: linear-gradient(30deg, rgba(118,144,149,1) 0%, rgba(49,56,57,1) 77%, rgba(115,115,115,1) 93%, rgba(138,136,159,1) 100%);";
    html += "        display: flex;";
    html += "        flex-direction: column;";
    html += "        align-items: center;";
    html += "        justify-content: center;";
    html += "        height: 100vh;";
    html += "        margin: 0;";
    html += "      }";
    html += "      .background-container {";
    html += "        background-color: rgb(213, 234, 238);";
    html += "        border: 2px solid black;";
    html += "        border-radius: 10px;";
    html += "        padding: 10px;";
    html += "        align-items: center;";
    html += "        width: 75%;";
    html += "        margin-top: 20px;";
    html += "      }";
    html += "      .row {";
    html += "        display: flex;";
    html += "        justify-content: center;";
    html += "        align-items: center;";
    html += "        width: 100%;";
    html += "        padding-top : 1%;";
    html += "        padding-bottom : 1%;";
    html += "        flex-grow: 3;";
    html += "      }";
    html += "      .row-stop{";
    html += "        display: flex;";
    html += "        justify-content: center;";
    html += "        align-items: center;";
    html += "        width: 100%;";
    html += "      }";
    html += "      .icon {";
    html += "        width: 50px;";
    html += "        height: 50px;";
    html += "        background-size: cover;";
    html += "      }";
    html += "      .fan1-icon, .fan2-icon, .fan3-icon {";
    html += "        background-size: cover;";
    html += "        background-repeat: no-repeat;";
    html += "        background-position: center center;";
    html += "        width: 50px;";
    html += "        height: 50px;";
    html += "      }";
    html += "      .low-icon {";
    html += "        background-image: url('/low');";
    html += "      }";
    html += "      .high-icon {";
    html += "        background-image: url('/high');";
    html += "      }";
    html += "      .icons-container .low-icon,";
    html += "      .icons-container .high-icon {";
    html += "        background-size: cover;";
    html += "        background-repeat: no-repeat;";
    html += "        background-position: center center;";
    html += "        width: 50px;";
    html += "        height: 50px;";
    html += "        margin-top: 30%;";
    html += "      }";
    html += "      .icons-container  {";
    html += "        text-align: center;";
    html += "        width: 70%;";
    html += "        height: 30px;";
    html += "        border: 2px;";
    html += "        border-style: double;";
    html += "        border-color: black;";
    html += "        border-radius: 10px;";
    html += "        background-color: rgba(198, 198, 198, 0.3);";
    html += "        color: black;";
    html += "        font-size: 16px;";
    html += "        margin: 5px;";
    html += "        margin-left: 3%;";
    html += "        margin-right: 3%;";
    html += "        position: relative;";
    html += "        pointer-events: none;";
    html += "        background: rgb(213, 234, 238);";
    html += "      }";
    html += "      .icons-container2 {";
    html += "        display: flex;";
    html += "        flex-direction: column;";
    html += "        align-items: center;";
    html += "        margin: 0;";
    html += "        padding-left: 5%;";
    html += "        padding-right: 5%;";
    html += "      }";
    html += "      .icons-container2 input[type=\"text\"] {";
    html += "        text-align: center;";
    html += "        width: 100%;";
    html += "        height: 30px;";
    html += "        border: 2px;";
    html += "        border-style: double;";
    html += "        border-color: black;";
    html += "        border-radius: 10px;";
    html += "        background-color: rgba(198, 198, 198, 0.3);";
    html += "        color: black;";
    html += "        font-size: 16px;";
    html += "        margin: 15px 0;";
    html += "        background: rgb(213, 234, 238);";
    html += "      }";
    html += "      .button {";
    html += "        background-size: 60%;";
    html += "        position: relative;";
    html += "        background-repeat: no-repeat;";
    html += "        background-position: center center;";
    html += "        border: 2px solid black;";
    html += "        background-color: rgba(198, 198, 198, 0.3);";
    html += "        color: black;";
    html += "        margin: 0 auto;";
    html += "        font-size: 16px;";
    html += "        cursor: pointer;";
    html += "        width: 120px;";
    html += "        height: 120px;";
    html += "        border-radius: 50px;";
    html += "        text-align: center;";
    html += "      }";
    html += "      .button:hover {";
    html += "        background-size: 70%;";
    html += "      }";
    html += "      .stop-button {";
    html += "        background-size: 60%;";
    html += "        display: flex;";
    html += "        background-repeat: no-repeat;";
    html += "        background-position: center center;";
    html += "        border: 2px solid black;";
    html += "        background-color: rgba(198, 198, 198, 0.3);";
    html += "        color: black;";
    html += "        font-size: 16px;";
    html += "        cursor: pointer;";
    html += "        width: 120px;";
    html += "        height: 120px;";
    html += "        border-radius: 50px;";
    html += "        justify-content: center center;";
    html += "        align-items: center center;";
    html += "      }";
    html += "      .stop-button:hover {";
    html += "        background-size: 70%;";
    html += "      }";
    html += "      .slider-container {";
    html += "        width: 100%;";
    html += "      }";
    html += "      .slider {";
    html += "        width: 100%;";
    html += "        margin: 0;";
    html += "      }";
    html += "      .slider::-webkit-slider-thumb {";
    html += "        width: 50px;";
    html += "        height: 0px;";
    html += "        background-color: #0074d9;";
    html += "        border: none;";
    html += "        border-radius: 50%;";
    html += "        cursor: pointer;";
    html += "      }";
    html += "      aside{";
    html += "        width:80%;";
    html += "        float:left;";
    html += "      }";
    html += "      section{";
    html += "        width:20%;";
    html += "        height: 100%;";
    html += "        display: flex;";
    html += "        float:left;";
    html += "      }";
    html += "      .clear{";
    html += "        clear:both;";
    html += "      }";
    html += "";
    html += "    </style>";
    html += "  </head>";
    html += "";
    html += "  <body>";
    html += "    <div class=\"background-container\">";
    html += "      <aside>";
    html += "        <!-- Rząd 1: Przyciski wiatraka -->";
    html += "        <div class=\"row\">";
    html += "          <button class=\"button\" id=\"fan1-button\" style=\"background-image: url('/fan1');\"></button>";
    html += "          <button class=\"button\" id=\"fan2-button\" style=\"background-image: url('/fan2');\"></button>";
    html += "          <button class=\"button\" id=\"fan3-button\" style=\"background-image: url('/fan3');\"></button>";
    html += "        </div>";
    html += "";
    html += "        <!-- Rząd 2: Pola tekstowe -->";
    html += "        <div class=\"row\">";
    html += "          <input class=\"icons-container\" type=\"text\" id=\"fanValue1\" value=\"0\">";
    html += "          <input class=\"icons-container\" type=\"text\" id=\"fanValue2\" value=\"0\">";
    html += "          <input class=\"icons-container\" type=\"text\" id=\"fanValue3\" value=\"0\">";
    html += "        </div>";
    html += "";
    html += "        <!-- Rząd 3: Suwaki -->";
    html += "        <div class=\"row\">";
    html += "          <div class=\"slider-container\">";
    html += "            <input type=\"range\" min=\"0\" max=\"255\" class=\"slider\" id=\"fanSlider1\">";
    html += "          </div>";
    html += "        </div>";
    html += "      </aside>";
    html += "";
    html += "      <section>";
    html += "        <div class=\"row-stop\">";
    html += "          <button class=\"stop-button\" id=\"fan-stop\" style=\"background-image: url('/stop');\"></button>";
    html += "        </div>";
    html += "      </section>";
    html += "";
    html += "      <div class=\"clear\"></div>";
    html += "    </div>";
    html += "";
    html += "    <div class=\"background-container\">";
    html += "";
    html += "      <aside>";
    html += "        <!-- Rząd 1: Przyciski wiatraka -->";
    html += "        <div class=\"row\">";
    html += "          <div class=\"icons-container2\">";
    html += "            <div class=\"icon low-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpLowStatus1\" value=\"0\">";
    html += "            <div class=\"icon high-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpHighStatus1\"  value=\"0\">";
    html += "          </div>";
    html += "          <div class=\"icons-container2\">";
    html += "            <div class=\"icon low-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpLowStatus2\" value=\"0\">";
    html += "            <div class=\"icon high-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpHighStatus2\"  value=\"0\">";
    html += "          </div>";
    html += "          <div class=\"icons-container2\">";
    html += "            <div class=\"icon low-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpLowStatus3\" value=\"0\">";
    html += "            <div class=\"icon high-icon\"></div>";
    html += "            <input type=\"text\" id=\"pumpHighStatus3\"  value=\"0\">";
    html += "          </div>";
    html += "        </div>";
    html += "";
    html += "";
    html += "      </aside>";
    html += "";
    html += "      <section>";
    html += "        <div class=\"row-stop\">";
    html += "          <button class=\"stop-button\" id=\"pump-stop\" style=\"background-image: url('/stop');\"></button>";
    html += "        </div>";
    html += "      </section>";
    html += "";
    html += "      <div class=\"clear\"></div>";
    html += "    </div>";
    html += "";
    html += "    <div class=\"background-container\">";
    html += "      <aside>";
    html += "        <!-- Trzeci background z zawartością -->";
    html += "        <!-- Świeca -->";
    html += "        <div class=\"row\">";
    html += "          <div class=\"spark-icon\">";
    html += "            <button class=\"button\" id=\"spark-button\"style=\"background-image: url('/spark');\"></button>";
    html += "          </div>";
    html += "          <div class=\"icons-container2\">";
    html += "            <input type=\"text\" id=\"sparkTimeout\" value=\"0\">";
    html += "          </div>";
    html += "        </div>";
    html += "      </aside>";
    html += "";
    html += "      <section>";
    html += "        <div class=\"row-stop\">";
    html += "          <button class=\"stop-button\" id=\"spark-stop\" style=\"background-image: url('/stop');\"></button>";
    html += "        </div>";
    html += "      </section>";
    html += "    </div>";



      // ====================================================================
      // --- SCRIPTS  
      // ====================================================================

    html += "  <script>											";
      // ====================================================================
      // --- FAN SECTION  
      // ====================================================================
      
    html += " var sliderChangeEnabled = true;";
    html += "function fanValue() {";
    html += "  console.log(\"FanValue funcition on...\");";
    html += "    var xhr1 = new XMLHttpRequest();";
    html += "    xhr1.open(\"GET\", \"/fanValue\", true);";
    html += "    xhr1.onreadystatechange = function() {";
    html += "        if (xhr1.readyState === 4 && xhr1.status === 200) {";
    html += "            var fanSlider1 = document.getElementById(\"fanSlider1\");"; 
    html += "            fanSlider1.value = xhr1.responseText;";
    html += "            sliderChangeEnabled = true;";
    html += "        };";
    html += "    };";
    html += "    xhr1.send();";
    html += "};";

    html += "      function fan1Button() {	sliderChangeEnabled = false;							";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/fan1-toggle', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var fan1Button = document.getElementById(\"fan1-button\");		";
    html += "            var fan2Button = document.getElementById(\"fan2-button\");		";
    html += "            var fan3Button = document.getElementById(\"fan3-button\");		";
    html += "             if (xhr.responseText === \"FAN1-ON\") {		";
    html += "                 fan1Button.style.backgroundColor = \"green\";		";
    html += "                 fan2Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "                 fan3Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "                 fanValue();		";
    html += "             } else {	sliderChangeEnabled = true;	";
    html += "                  fan1Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             }		";
    html += "          }											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      }											";
    html += "      document.getElementById(\"fan1-button\").addEventListener(\"click\", fan1Button);";		


    html += "      function fan2Button() {		sliderChangeEnabled = false;						";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/fan2-toggle', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var fan1Button = document.getElementById(\"fan1-button\");		";
    html += "            var fan2Button = document.getElementById(\"fan2-button\");		";
    html += "            var fan3Button = document.getElementById(\"fan3-button\");		";
    html += "             if (xhr.responseText === \"FAN2-ON\") {		";
    html += "            fan2Button.style.backgroundColor = \"green\";		";
    html += "            fan1Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            fan3Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "                 fanValue();		";
    html += "             } else {	sliderChangeEnabled = true;	";
    html += "            fan2Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            }		";
    html += "          }											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      }											";
    html += "      document.getElementById(\"fan2-button\").addEventListener(\"click\", fan2Button);	";

    html += "      function fan3Button() {	sliderChangeEnabled = false;							";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/fan3-toggle', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var fan1Button = document.getElementById(\"fan1-button\");		";
    html += "            var fan2Button = document.getElementById(\"fan2-button\");		";
    html += "            var fan3Button = document.getElementById(\"fan3-button\");		";
    html += "             if (xhr.responseText === \"FAN3-ON\") {		";
    html += "            fan3Button.style.backgroundColor = \"green\";		";
    html += "            fan1Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            fan2Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "                 fanValue();		";
    html += "             } else {	sliderChangeEnabled = true;	";
    html += "            fan3Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            };		";
    html += "          };											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      };											";
    html += "      document.getElementById(\"fan3-button\").addEventListener(\"click\", fan3Button);	";
    
    html += "function fetchDataForTimeOffPumpValue1() {";
    html += "    var xhr1 = new XMLHttpRequest();";
    html += "    xhr1.open(\"GET\", \"/timeOffPumpValue1\", true);";
    html += "    xhr1.onreadystatechange = function() {";
    html += "        if (xhr1.readyState === 4 && xhr1.status === 200) {";
    html += "            var pumpLowStatus1 = document.getElementById(\"pumpLowStatus1\");";
    html += "             console.log(\"Wartosc inputa zmieniła się na: \" + pumpLowStatus1.value);";
    html += "            pumpLowStatus1.value = xhr1.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr1.send();";
    html += "};";
    html += "";
    html += "function fetchDataForTimeOnPumpValue1() {";
    html += "    var xhr2 = new XMLHttpRequest();";
    html += "    xhr2.open(\"GET\", \"/timeOnPumpValue1\", true);";
    html += "    xhr2.onreadystatechange = function() {";
    html += "        if (xhr2.readyState === 4 && xhr2.status === 200) {";
    html += "            var pumpHighStatus1 = document.getElementById(\"pumpHighStatus1\");";
    html += "            pumpHighStatus1.value = xhr2.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr2.send();";
    html += "}";
        html += "function fetchDataForTimeOffPumpValue2() {";
    html += "    var xhr1 = new XMLHttpRequest();";
    html += "    xhr1.open(\"GET\", \"/timeOffPumpValue2\", true);";
    html += "    xhr1.onreadystatechange = function() {";
    html += "        if (xhr1.readyState === 4 && xhr1.status === 200) {";
    html += "            var pumpLowStatus2 = document.getElementById(\"pumpLowStatus2\");";
    html += "            pumpLowStatus2.value = xhr1.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr1.send();";
    html += "};";
    html += "";
    html += "function fetchDataForTimeOnPumpValue2() {";
    html += "    var xhr2 = new XMLHttpRequest();";
    html += "    xhr2.open(\"GET\", \"/timeOnPumpValue2\", true);";
    html += "    xhr2.onreadystatechange = function() {";
    html += "        if (xhr2.readyState === 4 && xhr2.status === 200) {";
    html += "            var pumpHighStatus2 = document.getElementById(\"pumpHighStatus2\");";
    html += "            pumpHighStatus2.value = xhr2.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr2.send();";
    html += "}";
        html += "function fetchDataForTimeOnPumpValue3() {";
    html += "    var xhr1 = new XMLHttpRequest();";
    html += "    xhr1.open(\"GET\", \"/timeOffPumpValue3\", true);";
    html += "    xhr1.onreadystatechange = function() {";
    html += "        if (xhr1.readyState === 4 && xhr1.status === 200) {";
    html += "            var pumpLowStatus3 = document.getElementById(\"pumpLowStatus3\");";
    html += "            pumpLowStatus3.value = xhr1.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr1.send();";
    html += "};";
    html += "";
    html += "function fetchDataForTimeOffPumpValue3() {";
    html += "    var xhr2 = new XMLHttpRequest();";
    html += "    xhr2.open(\"GET\", \"/timeOnPumpValue3\", true);";
    html += "    xhr2.onreadystatechange = function() {";
    html += "        if (xhr2.readyState === 4 && xhr2.status === 200) {";
    html += "            var pumpHighStatus3 = document.getElementById(\"pumpHighStatus3\");";
    html += "            pumpHighStatus3.value = xhr2.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr2.send();";
    html += "}";
    html += "function fetchDataForSparkOffTimeout() {";
    html += "    var xhr2 = new XMLHttpRequest();";
    html += "    xhr2.open(\"GET\", \"/sparkOffTimeout\", true);";
    html += "    xhr2.onreadystatechange = function() {";
    html += "        if (xhr2.readyState === 4 && xhr2.status === 200) {";
    html += "            var sparkTimeouts = document.getElementById(\"sparkTimeout\");";
    html += "            sparkTimeouts.value = xhr2.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr2.send();";
    html += "}";
    html += "function fetchDataForFan1Value() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan1Value\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan1Value = document.getElementById(\"fanValue1\");";
    html += "            fan1Value.value = xhr.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFan2Value() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan2Value\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan2Value = document.getElementById(\"fanValue2\");";
    html += "            fan2Value.value = xhr.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFan3Value() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan3Value\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan3Value = document.getElementById(\"fanValue3\");";
    html += "            fan3Value.value = xhr.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFanValue() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fanValue\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fanSlider = document.getElementById(\"fanSlider1\");";
    html += "            fanSlider.value = xhr.responseText;";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFan1State() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan1State\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan1Button = document.getElementById(\"fan1-button\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   fan1Button.style.backgroundColor = \"green\";		";
    html += "               } else {		";
    html += "                   fan1Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFan2State() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan2State\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan2Button = document.getElementById(\"fan2-button\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   fan2Button.style.backgroundColor = \"green\";		";
    html += "               } else {		";
    html += "                   fan2Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFan3State() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fan3State\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var fan3Button = document.getElementById(\"fan3-button\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   fan3Button.style.backgroundColor = \"green\";		";
    html += "               } else {		";
    html += "                   fan3Button.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };	";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForPumpStopState() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/pumpStopState\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var pumpButton = document.getElementById(\"pump-stop\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   pumpButton.style.backgroundColor = \"red\";		";
    html += "               } else {		";
    html += "                   pumpButton.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForFanStopState() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/fanStopState\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var pumpButton = document.getElementById(\"fan-stop\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   pumpButton.style.backgroundColor = \"red\";		";
    html += "               } else {		";
    html += "                   pumpButton.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForSparkStopState() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/sparkStopState\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var pumpButton = document.getElementById(\"spark-stop\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   pumpButton.style.backgroundColor = \"red\";		";
    html += "               } else {		";
    html += "                   pumpButton.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataForSparkState() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open(\"GET\", \"/sparkState\", true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "        if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "            var sparkButton = document.getElementById(\"spark-button\");";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "                   sparkButton.style.backgroundColor = \"green\";		";
    html += "               } else {		";
    html += "                   sparkButton.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "             };		";
    html += "        };";
    html += "    };";
    html += "    xhr.send();";
    html += "};";
    html += "function fetchDataAndInitialize() {";
    html += "    fetchDataForTimeOnPumpValue1();";
    html += "    fetchDataForTimeOffPumpValue1();";
    html += "    fetchDataForTimeOnPumpValue2();";
    html += "    fetchDataForTimeOffPumpValue2();";
    html += "    fetchDataForTimeOnPumpValue3();";
    html += "    fetchDataForTimeOffPumpValue3();";
    html += "    fetchDataForFan1Value();";
    html += "    fetchDataForFan2Value();";
    html += "    fetchDataForFan3Value();";
    html += "    fetchDataForFan1State();";
    html += "    fetchDataForFan2State();";
    html += "    fetchDataForFan3State();";
    html += "    fetchDataForSparkState();";
    html += "    fetchDataForFanValue();";
    html += "    fetchDataForSparkOffTimeout();";
    html += "    fetchDataForPumpStopState();";
    html += "    fetchDataForFanStopState();";
    html += "    fetchDataForSparkStopState();";
    html += "     }";
    html += "fetchDataAndInitialize();";
    html += "";
    html += "   "; 
    html += "   "; 
    html += "      function pumpStop() {								";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/pump-stop', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var pumpButton = document.getElementById(\"pump-stop\");		";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "            pumpButton.style.backgroundColor = \"red\";		";
    html += "             } else {		";
    html += "            pumpButton.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            }		";
    html += "          }											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      }											";
    html += "      document.getElementById(\"pump-stop\").addEventListener(\"click\", pumpStop);		";
    html += "      function fanStop() {								";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/fan-stop', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var fan = document.getElementById(\"fan-stop\");		";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "            fan.style.backgroundColor = \"red\";		";
    html += "             } else {		";
    html += "            fan.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            }		";
    html += "          }											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      }											";
    html += "      document.getElementById(\"fan-stop\").addEventListener(\"click\", fanStop);		";
    html += "      function sparkStop() {								";	
    html += "        var xhr = new XMLHttpRequest();							";
    html += "        xhr.open('GET', '/spark-stop', true);							";
    html += "        xhr.onreadystatechange = function() {							";
    html += "          if (xhr.readyState === 4 && xhr.status === 200) {					";
    html += "            var spark = document.getElementById(\"spark-stop\");		";
    html += "             if (xhr.responseText === \"ON\") {		";
    html += "            spark.style.backgroundColor = \"red\";		";
    html += "             } else {		";
    html += "            spark.style.backgroundColor = \"rgba(198, 198, 198, 0.3)\";		";
    html += "            }		";
    html += "          }											";
    html += "        };											";
    html += "        xhr.send();										";
    html += "      }											";
    html += "      document.getElementById(\"spark-stop\").addEventListener(\"click\", sparkStop);		";
    html += "var isSparkButtonPressed = false;"; // Dodaj zmienną do śledzenia stanu przycisku
    html += "function toggleSpark() {";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open('GET', '/spark-toggle', true);";
    html += "    xhr.onreadystatechange = function() {";
    html += "      if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "        var sparkButton = document.getElementById('spark-button');";
    html += "        if (xhr.responseText === 'ON') {";
    html += "          sparkButton.style.backgroundColor = 'green';";
    html += "        } else {";
    html += "          sparkButton.style.backgroundColor = 'rgba(198, 198, 198, 0.3)';";
    html += "        }";
    html += "      }";
    html += "    };";
    html += "    xhr.send();";
    html += "}";
    html += "document.getElementById('spark-button').addEventListener('mousedown', function() {";
    html += "  isSparkButtonPressed = true;";
    html += "});";
    html += "document.getElementById('spark-button').addEventListener('mouseup', function() {";
    html += "  isSparkButtonPressed = false;";
    html += "  toggleSpark();"; // Wywołaj funkcję toggleSpark po puszczeniu przycisku
    html += "});";
    html += " function handleChangeON(value) {";
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + value);";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open('GET', '/timeon-change?time='+newValue, true);";
    html += "  }";
    html += " function handleChangeOFF(value) {";
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + value);";
    html += "    var xhr = new XMLHttpRequest();";
    html += "    xhr.open('GET', '/timeoff-change?time='+newValue, true);";
    html += "  }";
    html += " var value = 0;";
    html += " pumpLowStatus1.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeoff1-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";
    html += " pumpHighStatus1.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeon1-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";

    html += " pumpLowStatus2.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeoff2-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";
    html += " pumpHighStatus2.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeon2-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";

    html += " pumpLowStatus3.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeoff3-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";
    html += " pumpHighStatus3.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/timeon3-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";

    html += " sparkTimeout.addEventListener(\"input\", function (e) {";
    html += "  var newValue = e.target.value; ";     
    html += "  console.log(\"Wartosc inputa zmieniła się na: \" + newValue);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/sparkOffTimeout-change?time='+newValue, true);";
    html += "  xhr.send();";
    html += " });";

    html += " ";

    html += "function handleFanSliderChange() {";
    html += "  if (sliderChangeEnabled) {";
    html += "  var fanSlider1 = document.getElementById(\"fanSlider1\");";
    html += "  var fanSliderValue = fanSlider1.value; ";
    html += "  console.log(\"Wartosc fanSliderValue zmieniła się na: \" + fanSliderValue);";
    html += "";
    html += "  var xhr = new XMLHttpRequest();";
    html += "    xhr.onreadystatechange = function() {";
    html += "      if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "                if (xhr.responseText === \"OK\") {";
    html += "                var fan1Button = document.getElementById(\"fan1-button\");";
    html += "                var fan2Button = document.getElementById(\"fan2-button\");";
    html += "                var fan3Button = document.getElementById(\"fan3-button\");";
    html += "                var fan1Value = document.getElementById(\"fanValue1\");";
    html += "                var fan2Value = document.getElementById(\"fanValue2\");";
    html += "                var fan3Value = document.getElementById(\"fanValue3\");";
    html += "                    if(fan1Button.style.backgroundColor === \"green\"){";
    html += "                     fan1Value.value = fanSliderValue;";
    html += "                    };";
    html += "                    if(fan2Button.style.backgroundColor === \"green\"){";
    html += "                     fan2Value.value = fanSliderValue;";
    html += "                    };";
    html += "                    if(fan3Button.style.backgroundColor === \"green\"){";
    html += "                     fan3Value.value = fanSliderValue;";
    html += "                    };";
    html += "                };";
    html += "            };";
    html += "        };";
    html += "  var data = JSON.stringify({fanSliderValue : fanSliderValue});";
    //html += "  data.append('fanSliderValue', fanSliderValue); ";
    html += "  xhr.open('POST', '/fan-slider1-change', true);";
    html += "  xhr.setRequestHeader('Content-Type', 'application/json; charset=utf-8');";
    html += "  xhr.send(data);";
    html += "};";
    html += "}";
    html += "";
    html += "document.getElementById(\"fanSlider1\").addEventListener(\"input\", handleFanSliderChange);";
    html += " ";
    html += "    </script>											";
    html += "</body>";
    html += "</html>";
      
    request->send(200, "text/html", html);
  });
  // ====================================================================
  // --- IMAGES START  
  // ====================================================================

        server.on("/fan1", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/fan1.png", "image/png");
      });
        server.on("/fan2", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/fan2.png", "image/png");
      });
        server.on("/fan3", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/fan3.png", "image/png");
      });
      
        server.on("/low", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/low.png", "image/png");
      });
      
        server.on("/high", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/high.png", "image/png");
      });
      
        server.on("/spark", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/spark.png", "image/png");
      });

      server.on("/pump", HTTP_GET, [](AsyncWebServerRequest *request){
         request->send(SPIFFS, "/pump.png", "image/png");
      });

      server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
         request->send(SPIFFS, "/stop.png", "image/png");
      });

      server.on("/timeout", HTTP_GET, [](AsyncWebServerRequest *request){
         request->send(SPIFFS, "/timeout.png", "image/png");
      });
      
      server.on("/pumpStopState", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = pumpStop ? "ON" : "OFF";
        Serial.print("Nowa wartość pumpStop: (/pumpStop)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fanStopState", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = fanStop ? "ON" : "OFF";
        Serial.print("Nowa wartość fanStop: (/fanStop)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/sparkStopState", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = sparkStop ? "ON" : "OFF";
        Serial.print("Nowa wartość sparkStop: (/sparkStop)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/sparkState", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = isSparkButtonOn ? "ON" : "OFF";
        Serial.print("Nowa wartość isSparkButtonOn: (/isSparkButtonOn)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });
      
      server.on("/timeOffPumpValue1", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOffTime1);//, 0);
        Serial.print("Nowa wartość pumpOffTime1: (/timeOffPumpValue1)");
        Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/timeOnPumpValue1", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOnTime1);//,, 0);
      Serial.print("Nowa wartość pumpOnTime: (/timeOnPumpValue1)");
      Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/timeOffPumpValue2", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOffTime2);//, 0);
        Serial.print("Nowa wartość pumpOffTime2: (/timeOffPumpValue2)");
        Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/timeOnPumpValue2", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOnTime2);//,, 0);
      Serial.print("Nowa wartość pumpOnTime: (/timeOnPumpValue2)");
      Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/timeOffPumpValue3", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOffTime3);//, 0);
        Serial.print("Nowa wartość pumpOffTime3: (/timeOffPumpValue3)");
        Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/timeOnPumpValue3", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(pumpOnTime3);//,, 0);
      Serial.print("Nowa wartość pumpOnTime: (/timeOnPumpValue3)");
      Serial.println(response);
        request->send(200, "text/plain", response);
      });

      server.on("/fanValue", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(fanValue);
        Serial.print("Nowa wartość fanValue: (/fanValue)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fan1Value", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(fan1Value);
        Serial.print("Nowa wartość fan1Value: (/fanValue1)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fan1State", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = fan1Enable ? "ON" : "OFF";
        Serial.print("Nowa wartość fan1Enable: (/fan1Enable)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fan2Value", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(fan2Value);
        Serial.print("Nowa wartość fan2Value: (/fanValue2)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fan2State", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = fan2Enable ? "ON" : "OFF";
        Serial.print("Nowa wartość fan2Enable: (/fan2Enable)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

     server.on("/fan3Value", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(fan3Value);
        Serial.print("Nowa wartość fan3Value: (/fanValue3)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/fan3State", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = fan3Enable ? "ON" : "OFF";
        Serial.print("Nowa wartość fan2Enable: (/fan3Enable)");
        Serial.println(response);
      request->send(200, "text/plain", response);
      });

      server.on("/sparkOffTimeout", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = String(sparkOffTimeout);//,, 0);
        Serial.print("Nowa wartość sparkOffTimeout: (/sparkOffTimeout)");
        Serial.println(response);
        request->send(200, "text/plain", response);
      });

  // ====================================================================
  // --- IMAGES END  
  // ====================================================================




  // ====================================================================
  // --- PERIFERIALS 
  // ====================================================================
  server.on("/fan1-toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    fan1Enable = !fan1Enable;
    fan2Enable = false;
    fan3Enable = false;

    // Przygotowanie odpowiedzi HTTP
    String fanState = fan1Enable ? "FAN1-ON" : "FAN1-OFF";
    request->send(200, "text/plain", fanState);
    Serial.print(fanState);
  });

  server.on("/fan2-toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    fan2Enable = !fan2Enable;
    fan1Enable = false;
    fan3Enable = false;
    
    // Przygotowanie odpowiedzi HTTP
    String fanState = fan2Enable ? "FAN2-ON" : "FAN2-OFF";
    request->send(200, "text/plain", fanState);
    Serial.print(fanState);
  });

  server.on("/fan3-toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    fan3Enable = !fan3Enable;
    fan1Enable = false;
    fan2Enable = false;
    
    // Przygotowanie odpowiedzi HTTP
    String fanState = fan3Enable ? "FAN3-ON" : "FAN3-OFF";
    request->send(200, "text/plain", fanState);
    Serial.print(fanState);
  });

  server.onRequestBody(
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            if ((request->url() == "/fan-slider1-change") &&
                (request->method() == HTTP_POST))
            {
                const size_t        JSON_DOC_SIZE   = 512U;
                DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
                
                if (DeserializationError::Ok == deserializeJson(jsonDoc, (const char*)data))
                {
                    JsonObject obj = jsonDoc.as<JsonObject>();

                    Serial.print(obj["fanSliderValue"].as<String>().c_str());
                    int fanSliderValue = obj["fanSliderValue"].as<String>().toInt();

                    if (fan1Enable & fan1Value != fanSliderValue ){
                      fan1Value = fanSliderValue;
                      EEPROM.writeInt(REG_FAN1, fan1Value);
                      EEPROM.commit();
                    }else if (fan2Enable & fan2Value != fanSliderValue ){
                      fan2Value = fanSliderValue;
                      EEPROM.writeInt(REG_FAN2, fan2Value);
                      EEPROM.commit();
                    }else if (fan3Enable & fan3Value != fanSliderValue){
                      fan3Value = fanSliderValue;
                      EEPROM.writeInt(REG_FAN3, fan3Value);
                      EEPROM.commit();
                    }
                }

                request->send(200, "text/plain", "OK");
            }
        });
  
  server.on("/pump-stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    pumpStop = !pumpStop;
    
    // Przygotowanie odpowiedzi HTTP
    String pinState = pumpStop ? "ON" : "OFF";
    request->send(200, "text/plain", pinState);
    Serial.print(pinState);
  });

  server.on("/fan-stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    fanStop = !fanStop;
    
    // Przygotowanie odpowiedzi HTTP
    String pinState = fanStop ? "ON" : "OFF";
    request->send(200, "text/plain", pinState);
    Serial.print(pinState);
  });

  server.on("/spark-stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // Zmiana stanu pinu
    sparkStop = !sparkStop;
    
    // Przygotowanie odpowiedzi HTTP
    String pinState = sparkStop ? "ON" : "OFF";
    request->send(200, "text/plain", pinState);
    Serial.print(pinState);
  });

  server.on("/spark-toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Zmiana stvanu przycisku
    isSparkButtonPressed = !isSparkButtonPressed;

    if (isSparkButtonPressed) {
      // Przycisk jest wciśnięty, więc możesz tu włączyć lampkę
      isSparkButtonOn = true;
    } else {// F_TRIG detection
      if (!sparkStop) {
          Serial.print("Wywołałem spark-toggle");  
          sparkStatusChanged = true;
      }
      // Przycisk został puścił, więc możesz tu wyłączyć lampkę
      isSparkButtonOn = false;
    }
    String response = isSparkButtonOn ? "ON" : "OFF";
    request->send(200, "text/plain", response);
  });

  server.on("/timeon1-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      int newPumpOnTime = newValue.toInt();//Float();
      
      // Zaktualizuj zmienną pumpOnTime
      pumpOnTime1 = newPumpOnTime;
      EEPROM.writeInt(REG_PUMPON_ONE, pumpOnTime1);
      EEPROM.commit();
      //Serial.print("W eepromie zapisano : " + EEPROM.read(REG_PUMPON));
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOnTime1: " + String(pumpOnTime1);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOnTime1: ");
      Serial.println(pumpOnTime1);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/timeoff1-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      float newPumpOffTime = newValue.toFloat();
      
      // Zaktualizuj zmienną pumpOffTime
      pumpOffTime1 = newPumpOffTime;
      EEPROM.writeInt(REG_PUMPOFF_ONE, pumpOffTime1);
      EEPROM.commit();
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOffTime: " + String(pumpOffTime1);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOffTime: ");
      Serial.println(pumpOffTime1);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/timeon2-change", HTTP_GET, [](AsyncWebServerRequest *request) {
     // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      int newPumpOnTime = newValue.toInt();//Float();
      
      // Zaktualizuj zmienną pumpOnTime
      pumpOnTime2 = newPumpOnTime;
      EEPROM.writeInt(REG_PUMPON_TWO, pumpOnTime2);
      EEPROM.commit();
      //Serial.print("W eepromie zapisano : " + EEPROM.read(REG_PUMPON));
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOnTime2: " + String(pumpOnTime2);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOnTime2: ");
      Serial.println(pumpOnTime2);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/timeoff2-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      float newPumpOffTime = newValue.toFloat();
      
      // Zaktualizuj zmienną pumpOffTime
      pumpOffTime2 = newPumpOffTime;
      EEPROM.writeInt(REG_PUMPOFF_TWO, pumpOffTime2);
      EEPROM.commit();
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOffTime: " + String(pumpOffTime2);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOffTime: ");
      Serial.println(pumpOffTime2);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/timeon3-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      int newPumpOnTime = newValue.toInt();//Float();
      
      // Zaktualizuj zmienną pumpOnTime
      pumpOnTime3 = newPumpOnTime;
      EEPROM.writeInt(REG_PUMPON_THREE, pumpOnTime3);
      EEPROM.commit();
      //Serial.print("W eepromie zapisano : " + EEPROM.read(REG_PUMPON));
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOnTime3: " + String(pumpOnTime3);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOnTime3: ");
      Serial.println(pumpOnTime3);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/timeoff3-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      float newPumpOffTime = newValue.toFloat();
      
      // Zaktualizuj zmienną pumpOffTime
      pumpOffTime3 = newPumpOffTime;
      EEPROM.writeInt(REG_PUMPOFF_THREE, pumpOffTime3);
      EEPROM.commit();
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość pumpOffTime: " + String(pumpOffTime3);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość pumpOffTime: ");
      Serial.println(pumpOffTime3);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  server.on("/sparkOffTimeout-change", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pobierz wartość parametru 'time' z żądania GET
    if (request->hasParam("time")) {
      String newValue = request->getParam("time")->value();
      
      // Przekształć wartość newValue na float (jeśli to konieczne)
      float newSparkOffTime = newValue.toFloat();
      
      // Zaktualizuj zmienną pumpOffTime
      sparkOffTimeout = newSparkOffTime;
      EEPROM.writeInt(REG_SPARKOFF, sparkOffTimeout);
      EEPROM.commit();
      
      // Przygotuj odpowiedź HTTP
      String response = "Nowa wartość sparkOffTimeout: " + String(sparkOffTimeout);
      
      request->send(200, "text/plain", response);
      
      // Możesz także wyświetlić nową wartość na monitorze szeregowym
      Serial.print("Nowa wartość sparkOffTimeout: ");
      Serial.println(sparkOffTimeout);
    } else {
      // Jeśli parametr 'time' nie został przesłany w żądaniu
      request->send(400, "text/plain", "Brak parametru 'time' w żądaniu");
    }
  });

  // Rozpocznij serwer
  server.begin();
  unsigned long currentMillis = millis();
  digitalWrite(SPARK, 0);
  previousSparkMillis = currentMillis - sparkOffTimeout*1000;
}

void loop() {
  /*unsigned long currentMillis = millis();

  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    fan();
  }

  // Wywołanie funkcji 2 co interval2
  if (currentMillis - previousMillis2 >= interval2) {
    previousMillis2 = currentMillis;
    pump();
  }

  // Wywołanie funkcji 3 co interval3
  if (currentMillis - previousMillis3 >= interval3) {
    previousMillis3 = currentMillis;
    spark();
  }*/
  fan();
  pump();
  spark();
}

void fan(){
  
  if (fan1Enable & fanValue != fan1Value){
    fanValue = fan1Value;
  }
  if (fan2Enable & fanValue != fan2Value){
    fanValue = fan2Value;
  }
  if (fan3Enable & fanValue != fan3Value){
    fanValue = fan3Value;
  }

  if (!fanStop && (fan1Enable || fan2Enable || fan3Enable)){
    ledcWrite(ledChanel,fanValue);
  }
  else{
    ledcWrite(ledChanel,0);
  }
}

void pump(){
  unsigned long currentMillis = millis();
  if (fan1Enable & (pumpOnTime != pumpOnTime1 || pumpOffTime != pumpOffTime1)){
    pumpOnTime = pumpOnTime1;
    pumpOffTime = pumpOffTime1;
  }
  if (fan2Enable & (pumpOnTime != pumpOnTime2 || pumpOffTime != pumpOffTime2)){
    pumpOnTime = pumpOnTime2;
    pumpOffTime = pumpOffTime2;
  }
  if (fan3Enable & (pumpOnTime != pumpOnTime3 || pumpOffTime != pumpOffTime3)){
    pumpOnTime = pumpOnTime3;
    pumpOffTime = pumpOffTime3;
  }
  if (!pumpStop && (fan1Enable || fan2Enable || fan3Enable)){
    if (currentMillis - previousPumpMillis >= (digitalRead(PUMP) ? pumpOnTime : pumpOffTime)) {
    previousPumpMillis = currentMillis;

    // Ustaw odpowiedni stan pompy
    digitalWrite(PUMP, digitalRead(PUMP) ? LOW : HIGH);
    }
  }
  else {
     digitalWrite(PUMP, 0);
  }
}

void spark(){
  unsigned long currentMillis = millis();
  
  if (sparkStatusChanged){
    sparkStatusChanged = false;
    previousSparkMillis = currentMillis;
  }

  // Logic
  if (!sparkStop){
    if (isSparkButtonOn || (!isSparkButtonOn && previousSparkMillis + sparkOffTimeout*1000 > currentMillis )){
      digitalWrite(SPARK, 1);
    }else{
      digitalWrite(SPARK, 0);
    }
  }else{
    previousSparkMillis = currentMillis - sparkOffTimeout*1000;
    digitalWrite(SPARK, 0);
  }
  
}
