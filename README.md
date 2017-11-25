# RF Power Outlets Controlled By Amazon Alexa

This code will create five FauxMo smart-home devices that emulate Belkin We-Mo switches. This is a third generation fork of code originally by kakopappa and can be found at  https://github.com/kakopappa/arduino-esp8266-alexa-multiple-wemo-switch/tree/master/wemos.

## Parts Used

* [NodeMCU ESP8266 (ESP-12E)](https://www.amazon.com/HiLetgo-Version-NodeMCU-Internet-Development/dp/B010O1G1ES/ref=sr_1_3?ie=UTF8&qid=1484281926&sr=8-3&keywords=esp8266)
* [Etekcity Wireless Remote Control Electrical Outlet Switch](https://www.amazon.com/Etekcity-Wireless-Electrical-Household-Appliances/dp/B00DQELHBS/ref=sr_1_3?ie=UTF8&qid=1484282201&sr=8-3&keywords=Etekcity)
* [433Mhz RF Transmitter/Receiver](https://www.amazon.com/UCEC-XY-MK-5V-Transmitter-Receiver-Raspberry/dp/B017AYH5G0/ref=sr_1_2?ie=UTF8&qid=1484282236&sr=8-2&keywords=433mhz+rf+transmitter+and+receiver)

## Workflow

- Detect frequency of signals sent from your wireless remote. This will vary per remote. [Example](https://www.princetronics.com/how-to-read-433-mhz-codes-w-arduino-433-mhz-receiver/)
- Alter [codebase](https://github.com/alukach/Amazon-Alexa-RF-Outlets-Integration/blob/631d47781fa11780adb4bf20f0367c76cae697ff/wemos.ino#L62-L70) to reflect your wireless remote's frequencies.
- Load code onto device via [Arduino software](https://www.arduino.cc/en/Main/Software)
- Boot device
- Connect to ESP8266 device's wifi hotspot: `EchoBase1`
- On some browsing devices, configuration window will open automatically. If not, visit [`192.168.4.1`](192.168.4.1)
- Configure WiFi settings.
  ![image](https://user-images.githubusercontent.com/897290/33226629-aad409ae-d14f-11e7-87b7-044701467710.png)
- Use Alexa to search for smart home devices. If all worked, Belkin WeMo should be found.
