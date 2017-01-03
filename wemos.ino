#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <functional>
#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

UpnpBroadcastResponder upnpBroadcastResponder;

Switch *alexa_switch1 = NULL;
Switch *alexa_switch2 = NULL;
Switch *alexa_switch3 = NULL;
Switch *alexa_switch4 = NULL;

// Callback prototypes
void alexa_switch1On();
void alexa_switch1Off();
void alexa_switch2On();
void alexa_switch2Off();
void alexa_switch3On();
void alexa_switch3Off();
void alexa_switch4On();
void alexa_switch4Off();

// Set Relay Pins
int relayOne = 12;
int relayTwo = 13;
int relayThree = 14;
int relayFour = 16;

// Names each relay/outlet/device is known by -- these are set during config
char alexa_name1[100] = "";
char alexa_name2[100] = "";
char alexa_name3[100] = "";
char alexa_name4[100] = "";

const char* AP_Name = "EchoBase1";
char saved_ssid[100] = "";
char saved_psk[100] = "";

// Flag for saving config data
bool shouldSaveConfig = false;
bool forceConfigPortal = false;

WiFiManager wifiManager;
  
// Callback notifying us of the need to save config
void saveConfigCallback () {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}


void setup()
{
  Serial.begin(115200);
  
  // -- WifiManager handling

  // 5-second delay in case you wish to observe boot-up
  for(int i=0; i < 5; i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Booting");
  
  // Clean FS, for testing... consider enabling if jumper is in flash mode, etc.
  // SPIFFS.format();

  // Set the flash/boot pin for input so we can read if the jumper is present
  pinMode(0, INPUT);
  // If the jumper is in "flash" mode (i.e., pin 0 is grounded), we will be enabling the config portal
  forceConfigPortal = (digitalRead(0) == 0);
  if(forceConfigPortal) {  
    Serial.println("Jumper set for flash - will trigger config portal");
  } else {
    Serial.println("Jumper set for boot - will attempt autoconnect, else config portal");
  }
    
  // Read configuration from FS json
  Serial.println("Mounting ESP8266 integrated filesystem...");
  if (SPIFFS.begin()) {
    Serial.println("Mounted file system");
    if (SPIFFS.exists("/config.json")) {
      Serial.println("Found existing config; reading file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Opened config file for reading");
        size_t size = configFile.size();
        Serial.print("File size (bytes) = ");
        Serial.println(size);
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        Serial.println("Parsed JSON content:");
        json.printTo(Serial);
        Serial.println();
        if (json.success()) {
          strcpy(alexa_name1, json["alexa_name1"]);
          strcpy(alexa_name2, json["alexa_name2"]);
          strcpy(alexa_name3, json["alexa_name3"]);
          strcpy(alexa_name4, json["alexa_name4"]);
          Serial.println("Parsed Alexa relay name #1: " + String(alexa_name1));
          Serial.println("Parsed Alexa relay name #2: " + String(alexa_name2));
          Serial.println("Parsed Alexa relay name #3: " + String(alexa_name3));
          Serial.println("Parsed Alexa relay name #4: " + String(alexa_name4));
        } else {
          Serial.println("** ERROR ** Failed to load/parse JSON config");
        }
      } else {
        Serial.println("No JSON file found in filesystem");
      }
    }
  } else {
    Serial.println("** ERROR ** Failed to mount ESP8266's integrated filesyste,m");
  }

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_alexa_name1("alexa_name1", "Device #1 name", alexa_name1, 100);
  WiFiManagerParameter custom_alexa_name2("alexa_name2", "Device #2 name", alexa_name2, 100);
  WiFiManagerParameter custom_alexa_name3("alexa_name3", "Device #3 name", alexa_name3, 100);
  WiFiManagerParameter custom_alexa_name4("alexa_name4", "Device #4 name", alexa_name4, 100);

  // Set the function that will be called to save the custom parameter after config
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Hand the parameter defintions to the WifiManager for use during config
  wifiManager.addParameter(&custom_alexa_name1);
  wifiManager.addParameter(&custom_alexa_name2);
  wifiManager.addParameter(&custom_alexa_name3);
  wifiManager.addParameter(&custom_alexa_name4);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  if(forceConfigPortal) {
    wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    // Force config portal while jumper is set for flashing
     if (!wifiManager.startConfigPortal(AP_Name)) {
          Serial.println("** ERROR ** Failed to connect with new config / possibly hit config portal timeout; Resetting in 3sec...");
          delay(3000);
          //reset and try again, or maybe put it to deep sleep
          ESP.reset();
          delay(5000);
        }
  } else {
    // Autoconnect if we can
    
    // Fetches ssid and pass and tries to connect; if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(AP_Name)) {
      Serial.println("** ERROR ** Failed to connect with new config / possibly hit timeout; Resetting in 3sec...");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  }

  // --- If you get here you have connected to the WiFi ---
  Serial.println("Connected to wifi");
  // Save the connect info in case we need to reconnect
  WiFi.SSID().toCharArray(saved_ssid, 100);
  WiFi.psk().toCharArray(saved_psk, 100);

  // Read updated parameters
  strcpy(alexa_name1, custom_alexa_name1.getValue());
  strcpy(alexa_name2, custom_alexa_name2.getValue());
  strcpy(alexa_name3, custom_alexa_name3.getValue());
  strcpy(alexa_name4, custom_alexa_name4.getValue());
  Serial.println("Read configured Alexa relay name #1: " + String(alexa_name1));
  Serial.println("Read configured Alexa relay name #2: " + String(alexa_name2));
  Serial.println("Read configured Alexa relay name #3: " + String(alexa_name3));
  Serial.println("Read configured Alexa relay name #4: " + String(alexa_name4));
          
  // Save the custom parameters to the ESP8266 filesystem if changed
  if (shouldSaveConfig) {
    Serial.println("Saving config to ESP8266 filesystem");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["alexa_name1"] = alexa_name1;
    json["alexa_name2"] = alexa_name2;
    json["alexa_name3"] = alexa_name3;
    json["alexa_name4"] = alexa_name4;
    Serial.println("Attempting to open config JSON file for writing");
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("** ERROR ** Failed to open JSON config file for writing");
    } else {
      json.printTo(Serial);
      Serial.println();
      json.printTo(configFile);
      configFile.close();
      Serial.println("File write complete");
    }
  }

  Serial.print("SSID: " );
  Serial.println(WiFi.SSID());
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // -- ALEXA setup/handling --
   
    upnpBroadcastResponder.beginUdpMulticast();
    
    // Define your switches here. Max 14
    // Format: Alexa invocation name, local port no, on callback, off callback
    alexa_switch1 = new Switch(alexa_name1, 80, alexa_switch1On, alexa_switch1Off);
    alexa_switch2 = new Switch(alexa_name2, 81, alexa_switch2On, alexa_switch2Off);
    alexa_switch3 = new Switch(alexa_name3, 82, alexa_switch3On, alexa_switch3Off);
    alexa_switch4 = new Switch(alexa_name4, 83, alexa_switch4On, alexa_switch4Off);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*alexa_switch1);
    upnpBroadcastResponder.addDevice(*alexa_switch2);
    upnpBroadcastResponder.addDevice(*alexa_switch3);
    upnpBroadcastResponder.addDevice(*alexa_switch4);

    // Set relay pins to outputs
     pinMode(12,OUTPUT); 
     pinMode(13,OUTPUT);
     pinMode(14,OUTPUT);
     pinMode(16,OUTPUT);
       

}

/* If disconnected from Wifi, enter a blocking loop that periodically attempts reconnection */
void reconnectIfNecessary() {
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected; Attempting reconnect to " + String(saved_ssid) + "...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(saved_ssid, saved_psk);
    // Output reconnection status info every second over the next 10 sec
    for( int i = 0; i < 10 ; i++ )  {
      delay(1000);
      Serial.print("WiFi status = ");
      if( WiFi.status() == WL_CONNECTED ) {
        Serial.println("Connected");
        break;
      } else {
        Serial.println("Disconnected");
      }
    }
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Failure to establish connection after 10 sec. Will reattempt connection in 2 sec");
      delay(2000);
    }
  }
}

void loop()
{
  // Ensure wifi is connected (won't return until it has connected)
  reconnectIfNecessary();
  // Respond to any Alexa/discovery requests
  upnpBroadcastResponder.serverLoop();
  // Respond to any UPnP control requests
  alexa_switch1->serverLoop();
  alexa_switch2->serverLoop();
  alexa_switch3->serverLoop();
  alexa_switch4->serverLoop();
}

void alexa_switch1On() {
    Serial.print("Switch 1 turn on ...");
    digitalWrite(relayOne, HIGH);   // sets relayOne on
}
void alexa_switch1Off() {
    Serial.print("Switch 1 turn off ...");
    digitalWrite(relayOne, LOW);   // sets relayOne off
}

void alexa_switch2On() {
    Serial.print("Switch 2 turn on ...");
    digitalWrite(relayTwo, HIGH);   // sets relayTwo on
}
void alexa_switch2Off() {
    Serial.print("Switch 2 turn off ...");
    digitalWrite(relayTwo, LOW);   // sets relayTwo off
}

void alexa_switch3On() {
    Serial.print("Switch 3 turn on ...");
    digitalWrite(relayThree, HIGH);   // sets relayThree on
}
void alexa_switch3Off() {
    Serial.print("Switch 3 turn off ...");
    digitalWrite(relayThree, LOW);   // sets relayThree off
}

void alexa_switch4On() {
    Serial.print("Switch 4 turn on ...");
    digitalWrite(relayFour, HIGH);   // sets relayFour on
}
void alexa_switch4Off() {
    Serial.print("Switch 4 turn off ...");
    digitalWrite(relayFour, LOW);   // sets relayFour off
}
