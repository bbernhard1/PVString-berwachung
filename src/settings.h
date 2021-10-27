int led_builtin = 2;

// Wifi Settings
String hostname = "ESP_PVStromsensor";

// Telnet Settings
int port = 23;

// MQTT Settings
const char* MQTTBroker = "10.0.0.1"; //IPS Server
const int MQTTPort = 1883;  // IPS MQTT Port

// OTA Settings
int iOTAPort = 8266;    // ESP32: 3232, ESP8266: 8266

// String definition
int String1 = 2;
int String2 = 0;
int String3 = 14;
int String4 = 12;
int String5 = 13;
int String6 = 16;

int MeasSettling = 100;   // MilliSeconds
int iSampleCnt = 100;

double ACS712Senstivity = 0.1; // mV/A
