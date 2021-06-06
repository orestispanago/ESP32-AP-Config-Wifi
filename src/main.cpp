#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <ServerUtils.h>

unsigned long currentMillis;
unsigned long previousMillis;
const long wifiWaitInterval = 10000;
unsigned long taskInterval = 30000;

bool initWiFi()
{
  if (ssid == "")
  {
    Serial.println("Undefined SSID");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= wifiWaitInterval)
    {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  Serial.println(WiFi.localIP());
  return true;
}

void setupAP()
{
  Serial.println("Setting AP (Access Point)");
  WiFi.softAP("ESP-WIFI-MANAGER", NULL);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void setup()
{
  Serial.begin(115200);
  initSPIFFS();
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);

  if (initWiFi())
  {
    serveStaticFromSpiffs();
  }
  else
  {
    setupAP();
    serveWifiManager();
    getFormDataAndRestart();
    server.begin();
  }
}
void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis > taskInterval)
    {
      Serial.println("Hi");
      previousMillis = currentMillis;
    }
  }
}
