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

  for(int i=0; i < 5; i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Booting");
  
  //clean FS, for testing if jumper is in flash mode
  // SPIFFS.format();
  
  pinMode(0, INPUT);
  forceConfigPortal = (digitalRead(0) == 0);
  if(forceConfigPortal) {  
    Serial.println("Jumper set for flash - will trigger config portal");
  } else {
    Serial.println("Jumper set for boot - will attempt autoconnect, else config portal");
  }
    
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(alexa_name1, json["alexa_name1"]);
          strcpy(alexa_name2, json["alexa_name2"]);
          strcpy(alexa_name3, json["alexa_name3"]);
          strcpy(alexa_name4, json["alexa_name4"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_alexa_name1("alexa_name1", "Device #1 name", alexa_name1, 100);
  WiFiManagerParameter custom_alexa_name2("alexa_name2", "Device #2 name", alexa_name2, 100);
  WiFiManagerParameter custom_alexa_name3("alexa_name3", "Device #3 name", alexa_name3, 100);
  WiFiManagerParameter custom_alexa_name4("alexa_name4", "Device #4 name", alexa_name4, 100);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
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
          Serial.println("failed to connect and hit timeout");
          delay(3000);
          //reset and try again, or maybe put it to deep sleep
          ESP.reset();
          delay(5000);
        }

    
  } else {
    // Autoconnect if we can
    
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(AP_Name)) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  }


  // --- If you get here you have connected to the WiFi ---
  Serial.println("connected...yeey :)");

  // Read updated parameters
  strcpy(alexa_name1, custom_alexa_name1.getValue());
  strcpy(alexa_name2, custom_alexa_name2.getValue());
  strcpy(alexa_name3, custom_alexa_name3.getValue());
  strcpy(alexa_name4, custom_alexa_name4.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["alexa_name1"] = alexa_name1;
    json["alexa_name2"] = alexa_name2;
    json["alexa_name3"] = alexa_name3;
    json["alexa_name4"] = alexa_name4;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    Serial.println();
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.print("SSID: " );
  Serial.println(WiFi.SSID());
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // -- ALEXA handling --
   
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

    //Set relay pins to outputs
       pinMode(12,OUTPUT); 
       pinMode(13,OUTPUT);
       pinMode(14,OUTPUT);
       pinMode(16,OUTPUT);
       

}
 
void loop()
{
	 if(WiFi.status() == WL_CONNECTED) {
      Serial.print(':');
      upnpBroadcastResponder.serverLoop();
      alexa_switch1->serverLoop();
      alexa_switch2->serverLoop();
      alexa_switch3->serverLoop();
      alexa_switch4->serverLoop();
	 } else {
      Serial.println("Disconnected while in loop(); Attempting reconnect...");  
      WiFi.begin();
      if(WiFi.status() != WL_CONNECTED) {
        delay(2000);
      }
	 }
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
