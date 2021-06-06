#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

//Variables to save values from HTML form
String ssid;
String pass;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";

unsigned long previousMillis = 0;
const long wifiWaitInterval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

// Timer variables (get sensor readings)
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

//-----------------FUNCTIONS TO HANDLE SPIFFS AND FILES-----------------//
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- frite failed");
  }
}

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

  unsigned long currentMillis = millis();
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

void setup()
{
  Serial.begin(115200);
  initSPIFFS();
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);

  if (initWiFi())
  {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });
    server.serveStatic("/", SPIFFS, "/");

    events.onConnect([](AsyncEventSourceClient *client)
                     {
                       if (client->lastId())
                       {
                         Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
                       }
                     });
    server.addHandler(&events);

    server.begin();
  }
  else
  {
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

    server.serveStatic("/", SPIFFS, "/");

    // Get the parameters submited on the form
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                int params = request->params();
                for (int i = 0; i < params; i++)
                {
                  AsyncWebParameter *p = request->getParam(i);
                  if (p->isPost())
                  {
                    // HTTP POST ssid value
                    if (p->name() == PARAM_INPUT_1)
                    {
                      ssid = p->value().c_str();
                      Serial.print("SSID set to: ");
                      Serial.println(ssid);
                      writeFile(SPIFFS, ssidPath, ssid.c_str());
                    }
                    // HTTP POST pass value
                    if (p->name() == PARAM_INPUT_2)
                    {
                      pass = p->value().c_str();
                      Serial.print("Password set to: ");
                      Serial.println(pass);
                      writeFile(SPIFFS, passPath, pass.c_str());
                    }
                  }
                }
                request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: ");
                delay(3000);
                // After saving the parameters, restart the ESP32
                ESP.restart();
              });
    server.begin();
  }
}
void loop()
{
  // If the ESP32 is set successfully in station mode...
  if (WiFi.status() == WL_CONNECTED)
  {
    //...Send Events to the client with sensor readins and update colors every 30 seconds
    if (millis() - lastTime > timerDelay)
    {

      lastTime = millis();
    }
  }
}
