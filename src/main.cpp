#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "PubSubClient.h"

#include "secrets.h"
#include "settings.h"

#include "ADS1X15.h"

// declare telnet server (do NOT put in setup())
WiFiServer TelnetServer(port);
WiFiClient Telnet;

// WiFiClient WifiClient;
PubSubClient MQTTclient(Telnet);
// Init ADS1115
ADS1115 ADS(0x48);

void handleTelnet()
{
  if (TelnetServer.hasClient())
  {
    // client is connected
    if (!Telnet || !Telnet.connected())
    {
      if (Telnet)
        Telnet.stop();                   // client disconnected
      Telnet = TelnetServer.available(); // ready for new client
    }
    else
    {
      TelnetServer.available().stop(); // have client, block new conections
    }
  }

  if (Telnet && Telnet.connected() && Telnet.available())
  {
    // client input processing
    while (Telnet.available())
      Serial.write(Telnet.read()); // pass through
                                   // do other stuff with client input here
  }
}

void doMeas(String sStringNr, long lVDDRead)
{
  double dcurrent;
  delay(MeasSettling);
  long lADCRead = 0;
  for (int i = 0; i < iSampleCnt; i++) {  // do some averaging
    lADCRead = lADCRead + ADS.readADC(3);
    }
  lADCRead = lADCRead / iSampleCnt;

  dcurrent = ((lADCRead - lVDDRead / 2) * ADS.toVoltage(1)) / ACS712Senstivity;
  Serial.print(sStringNr);
  Serial.println(dcurrent);
  MQTTclient.publish((hostname + "/Current_" + sStringNr).c_str(), ((String)dcurrent).c_str());
  return;
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  delay(1000); // serial delay

  //connect to WiFi
  Serial.printf("Connecting to %s ", wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname.c_str()); //define hostname
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println();

  // OTA Stuff
  ArduinoOTA.setPort(iOTAPort);
  ArduinoOTA.setHostname(hostname.c_str());

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_FS
                         type = "filesystem";
                       }

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       Serial.println("Start updating " + type);
                     });

  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         Serial.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         Serial.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         Serial.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         Serial.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         Serial.println("End Failed");
                       }
                     });

  Serial.println("Starting OTA");
  ArduinoOTA.begin();

  // TELNET Stuff
  TelnetServer.begin();
  Serial.print("Starting telnet server on port " + (String)port);
  TelnetServer.setNoDelay(true); // ESP BUG ?
  Serial.println();
  delay(100);

  // MQTT Stuff
  MQTTclient.setServer(MQTTBroker, MQTTPort);

  // ADS1115
  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);
  ADS.begin();

  Serial.println(F("\nSET\tACTUAL\n=================="));
  for (uint32_t speed = 50000; speed <= 1000000; speed += 50000)
  {
    ADS.setWireClock(speed);
    Serial.print(speed);
    Serial.print("\t");
    Serial.println(ADS.getWireClock());
  }

  ADS.setWireClock(100000);
  Serial.println();
  ADS.setGain(0); // 6.144V

  pinMode(String1, OUTPUT);
  pinMode(String2, OUTPUT);
  pinMode(String3, OUTPUT);
  pinMode(String4, OUTPUT);
  pinMode(String5, OUTPUT);
  pinMode(String6, OUTPUT);

  digitalWrite(String1, HIGH);
  digitalWrite(String2, HIGH);
  digitalWrite(String3, HIGH);
  digitalWrite(String4, HIGH);
  digitalWrite(String5, HIGH);
  digitalWrite(String6, HIGH);
}

void loop()
{
  ArduinoOTA.handle();
  handleTelnet();
  Telnet.println("uptime: " + (String)millis() + " ms");
  {
    while (!MQTTclient.connected())
    {
      MQTTclient.connect(hostname.c_str());
      delay(1000);
    }
  }

  delay(1000);

long lVDDRead = 0;
for (int i = 0; i < iSampleCnt; i++) {  
    lVDDRead = lVDDRead + ADS.readADC(1);
    }
  lVDDRead = lVDDRead / iSampleCnt;

  double dVDD = lVDDRead * ADS.toVoltage(1);
  Serial.print("VDD");
  Serial.println(dVDD);
  MQTTclient.publish((hostname + "/VDD").c_str(), ((String)dVDD).c_str());

  String sTopic;
  sTopic = hostname + "/IP Adresse";
  MQTTclient.publish(sTopic.c_str(), WiFi.localIP().toString().c_str());

  digitalWrite(String1, LOW);
  doMeas("String1", lVDDRead);
  digitalWrite(String1, HIGH);

  digitalWrite(String2, LOW);
  doMeas("String2", lVDDRead);
  digitalWrite(String2, HIGH);

  digitalWrite(String3, LOW);
  doMeas("String3", lVDDRead);
  digitalWrite(String3, HIGH);

  digitalWrite(String4, LOW);
  doMeas("String4", lVDDRead);
  digitalWrite(String4, HIGH);

  digitalWrite(String5, LOW);
  doMeas("String5", lVDDRead);
  digitalWrite(String5, HIGH);

  digitalWrite(String6, LOW);
  doMeas("String6", lVDDRead);
  digitalWrite(String6, HIGH);
}
