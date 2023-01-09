#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

struct Settings
{
  char serviceIDForButton1[2];
  char serviceIDForButton2[2];
  char gatewayIP[16];
  char port[5];
  char section[7];
} settings;

#define BUTTON_PIN_1 0
#define BUTTON_PIN_2 2

struct Button
{
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {BUTTON_PIN_1, 0, false};
Button button2 = {BUTTON_PIN_2, 0, false};

void sendRequest(int buttonID)
{
  if (WiFi.status() == WL_CONNECTED)
  {

    WiFiClient client;
    HTTPClient http;

    String host = settings.gatewayIP;
    int port = String(settings.port).toInt();
    String uri = "/service";

    String httpRequestData;
    StaticJsonDocument<48> doc;

    doc["gender"] = settings.section;
    doc["button_id"] = buttonID;

    if (!serializeJson(doc, httpRequestData))
    {
      Serial.println("Json serialize failed");
    }

    http.begin(client, host, port, uri, false);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(httpRequestData);

    // httpResponseCode will be negative on error
    if (httpResponseCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpResponseCode);

      // file found at server
      if (httpResponseCode == HTTP_CODE_OK)
      {
        const String &payload = http.getString();
        Serial.println("Received response:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    }
    else
    {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  }
}

void IRAM_ATTR button_1_pressed()
{
  Serial.println("button_1_pressed");
  button1.pressed = true;
}

void IRAM_ATTR button_2_pressed()
{
  Serial.println("button_2_pressed");
  button2.pressed = true;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  Serial.println("Settings loaded");

  settings.serviceIDForButton1[1] = '\0';
  settings.serviceIDForButton2[1] = '\0';
  settings.section[6] = '\0';
  settings.gatewayIP[15] = '\0';
  settings.port[4] = '\0';

  WiFiManager wm;
  WiFiManagerParameter gatewayIP("gatewayIP", "Gateway IP", settings.gatewayIP, 15);
  WiFiManagerParameter gatewayPort("gatewayPort", "Gateway Port", settings.port, 4);

  WiFiManagerParameter serviceIDForButton1("serviceIDForButton1", "ID for Button_1", settings.serviceIDForButton1, 2);
  WiFiManagerParameter serviceIDForButton2("serviceIDForButton2", "ID for Button_2", settings.serviceIDForButton2, 2);
  WiFiManagerParameter facilitySection("facilitySection", "Facility Section", settings.section, 7);

  wm.addParameter(&gatewayIP);
  wm.addParameter(&gatewayPort);

  wm.addParameter(&serviceIDForButton1);
  wm.addParameter(&serviceIDForButton2);
  wm.addParameter(&facilitySection);

  // SSID & password parameters already included
  wm.setConfigPortalTimeout(120);
  wm.autoConnect("MLBD_IoT_Button", "Mlbd@1234");

  Serial.print("Config done");
  strncpy(settings.gatewayIP, gatewayIP.getValue(), 16);
  strncpy(settings.port, gatewayPort.getValue(), 5);

  strncpy(settings.serviceIDForButton1, serviceIDForButton1.getValue(), 2);
  strncpy(settings.serviceIDForButton2, serviceIDForButton2.getValue(), 2);
  strncpy(settings.section, facilitySection.getValue(), 7);

  settings.serviceIDForButton1[1] = '\0';
  settings.serviceIDForButton2[1] = '\0';
  settings.section[6] = '\0';
  settings.gatewayIP[15] = '\0';
  settings.port[4] = '\0';

  Serial.print("Service Select ID for Button 1: ");
  Serial.println(settings.serviceIDForButton1);
  Serial.print("Service Select ID for Button 2: ");
  Serial.println(settings.serviceIDForButton2);
  Serial.print("Gateway IP: ");
  Serial.println(settings.gatewayIP);
  Serial.print("Gateway Port: ");
  Serial.println(settings.port);
  Serial.print("Section: ");
  Serial.println(settings.section);

  EEPROM.put(0, settings);
  if (EEPROM.commit())
  {
    Serial.println("Settings saved");
  }
  else
  {
    Serial.println("EEPROM error");
  }

  EEPROM.begin(512);
  EEPROM.get(0, settings);
  Serial.println("Settings loaded");
  Serial.print("Service Select ID for Button 1: ");
  Serial.println(settings.serviceIDForButton1);
  Serial.print("Service Select ID for Button 2: ");
  Serial.println(settings.serviceIDForButton2);
  Serial.print("Gateway IP: ");
  Serial.println(settings.gatewayIP);
  Serial.print("Gateway Port: ");
  Serial.println(settings.port);
  Serial.print("Section: ");
  Serial.println(settings.section);

  pinMode(button1.PIN, INPUT_PULLUP);
  pinMode(button2.PIN, INPUT_PULLUP);
  attachInterrupt(button1.PIN, button_1_pressed, ONLOW_WE);
  attachInterrupt(button2.PIN, button_2_pressed, ONLOW_WE);
}

void loop()
{
  if (button1.pressed)
  {
    button1.pressed = false;
    sendRequest(1);
  }

  if (button2.pressed)
  {
    button2.pressed = false;
    sendRequest(2);
  }
}