#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FileUtils.h>

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

//Variables to save values from HTML form
String ssid;
String pass;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

void serveStaticFromSpiffs()
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

void getFormDataAndRestart()
{
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
                  request->send(200, "text/plain", "Done. ESP will restart");
                  delay(3000);
                  // After saving the parameters, restart the ESP32
                  ESP.restart();
              });
}

void serveWifiManager()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifimanager.html", "text/html"); });
    server.serveStatic("/", SPIFFS, "/");
}
