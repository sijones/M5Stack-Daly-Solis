/*
 Name:    M5Stack_DALY_SOLIS_Converter_2021_Standard.ino
 Version: 2.1 vom 11.10.2021
 Author:  EH

************************************************** ************************************************** *******************************
* Tested with a SOLIS RHI-4.6K-48ES-5G inverter with software version: Model No: F2, software version: 33001F / 330022 / *
* 330024 I currently recommend firmware version 330024 or 350024. There have been significant improvements in the loading behavior of the *
* Battery detected. In older versions there were always 'pauses' when changing from unloading to loading mode. *
* *
* An EASTRON SDM630 V2 Modbus was used for energy measurement. *
* The inputs and outputs must be swapped with the EASTRON. The energy meter in the SOLIS must be set to GRID *
* and the BACKFLOW to 0 watts or 100 watts. *
* *
* I have used the EASTRON SDM630 energy meter for 0 feed. In the meantime, the SOLIS in D is evident *
* Supplied with a 3-phase energy meter as standard. Since this has no direct influence on the CAN communication with *
* the M5Stack, the energy meter supplied as standard should also work. But I haven't tested it. *
* *
* Personal hardware optimization tip for the SOLIS Hybrid: *
* The SOLIS Hybrid is only passively cooled on the outside. With longer charge / discharge cycles with 100 amps (5200W) it gets very hot. *
* It is well known that capacitors in particular suffer when the temperature rises. This shortens the service life. *
* Therefore it can be worthwhile if you simply have 4 x 80mm fans (preferably 48 volt fans) at the top of the SOLIS heat sink *
* lies down and actively sucks off the warm rising air. As a result, the SOLIS remains significantly cooler and appreciate that *
* especially the capacitors. A 10 degree increase in temperature of an electrolytic capacitor halves its service life. *
* *
* You can find more information here: *
* *
* https://www.elektronikpraxis.vogel.de/lebensurance-von-elektrolytkondensatoren-in-netzteile-a-536801/ *
* *
* Recommended fans (which are designed for 10 years of continuous operation): https://www.aliexpress.com/item/32845000468.html *
* *
************************************************** ************************************************** *******************************

 This software reads and prepares the data of a DALY Smart BMS via the RS485 interface
 collects the data for the SOLIS Hybrid Inverter and transfers the data to the SOLIS via the CAN interface every second.

 The software is an absolute minimum and can be refined and optimized at will. It became conscious
 Keeping it as simple as possible in order to give less experienced programmers the opportunity to understand the code.

 Hardware used (possible source of supply Germany):

 M5Stack Basic: https://eckstein-shop.de/M5StackESP32BasicCoreIoTDevelopmentKitArduino2FMicroPython2FUIFlow
 M5Stack Commu module: https://eckstein-shop.de/M5StackCOMMUModuleExtendRS4852FTTLCAN2FI2CPort (attention is delivered without cable)
 M5Stack connection cable: https://eckstein-shop.de/M5StackBuckledGroveCable100cm1pcs

 optional:

 Occasional communication faults with the DALY RS485 interface were reported. So far I have not been able to determine it myself.
 The problems could be solved with an isolated RS485 interface of the M5Stack. For the connection in the Commu module
 4 solder jumpers must be re-soldered and the UART connection of the commu module must be connected to the RS485 module.
 Nothing needs to be changed in the code for this. An isolated RS485 interface is always better than a non-isolated interface.

 isolated RS485 interface: https://eckstein-shop.de/M5StackIsolatedRS485Unit2CfC3BCrAusfallschutz2CC39Covercurrent protection


 **** Important: The esp.reset () command only resets the CPU, but not the M5-Stack Comm module
 **** The comm module can only be reset by a hardware reset (or power off / on of the M5 stack)
 **** It is also sufficient to pull the reset signal in the M5 stack to low with a free GPIO pin


 Used DALY Smart BMS: https://de.aliexpress.com/item/32921412658.html
 Model: 250A / BT UART 485 CAN
 Minimum firmware version required: 20210222.s19
 The USB / RS485 cable for the PC must be ordered separately at the same time


 Special libraries used:

 <mcp_can.h> Library for CAN interface https://github.com/coryjfowler/MCP_CAN_lib

 Reference:

The RS485 routines for the DALY BMS come from this source: https://github.com/justinschoeman/dalybms

The CAN routines for the SOLIS inverter come from this source: https://github.com/Tom-evnut/VEcan

The CAN interface of the SOLIS must be connected to the CAN interface of the M5 Stack comm module as described in the manual, PIN 4 (blue) and PIN 5 (white-blue)
The RS485 interface of the DALY-BMS must be connected to the RS485 interface of the M5-Stack Comm Module (including the ground).

Note: This software may be freely used or modified. No guarantee can be given for proper function!

The current DALY software PC version for the BMS basic settings 1.2.8 can be found here: https://www.dalyelec.cn/newsshow.php?cid=25&id=77&lang=1

Special DALY commands for control SMART BMS:

Discharge MOS off: A5 40 D9 08 00 00 00 00 00 00 00 00 C6
Discharge MOS on: A5 40 D9 08 01 00 00 00 00 00 00 00 C7

Charge MOS from: A5 40 DA 08 00 00 00 00 00 00 00 00 C7
Charge MOS on: A5 40 DA 08 01 00 00 00 00 00 00 00 C8

Set SOC to 10%: A5 40 21 08 15 03 0B 0C 39 2C 00 64 06
Set SOC to 100%: A5 40 21 08 15 03 0B 0C 3A 39 03 E8 9B

void startWebServer()
{
// Root-URL
server.on("/", handleRoot);

// further path
server.on("/m5", {
  server.send(200, "text/plain", "m5 path hello");
});

// relaistrigger
server.on("/11", RelaissendUDP);
// and further more if need.

// if path not valid
server.onNotFound(handleNotFound);

// server ok so, go
server.begin();
//Serial.println("HTTP server started");
}

void handleNotFound()
{
String message = "File Not Found\n\n";
message += "URI: ";
message += server.uri();
message += "\nMethod: ";
message += (server.method() == HTTP_GET) ? "GET" : "POST";
message += "\nArguments: ";
message += server.args();
message += "\n";
for (uint8_t i = 0; i < server.args(); i++) {
message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
}
server.send(404, "text/plain", message);
}

void handleRoot()
{
String content = String(WiFi_hostname)+"<br> this button trigger the relais same way button A at M5stack <a href="/11">RELAISTRIGGER</a></body></html>";
String s = "<!DOCTYPE html><html><head>";
s += "<meta name="viewport" content="width=device-width,user-scalable=0">";
s += "<title>";
s += "M5Stack";
s += "</title></head><body>";
s += content;
s += "</body></html>";
server.send(200, "text/html", "OK http connect: " + s);
}

//LAN 2Chanal Relais
void RelaissendUDP ()
{
// 2WayRelais
IPAddress 2channelRelais(10, 10, 10, 9);
unsigned int UDP2port = 6723; //sendePport 6723 UDP für 2WayRelais, TCP port 6722
const int UDP_PACKET_SIZE = 4; //packetgroesse wictig rüf Relaisansteuerung
byte packet2Buffer[ UDP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
boolean packet2Rcvd = true;

delay(10);
memset(packet2Buffer, 0, UDP_PACKET_SIZE);
sprintf((char*)packet2Buffer, "11:5"); //PCB Relais2, close for 5seconds
Udp.beginPacket(2channelRelais, UDP2port);
Udp.write(packet2Buffer, UDP_PACKET_SIZE);
Udp.endPacket();
delay(10);
memset(packet2Buffer, 0, UDP_PACKET_SIZE);
sprintf((char*)packet2Buffer, "12:4"); //PCB Relais2, close for 4seconds
Udp.beginPacket(2channelRelais, UDP2port);
Udp.write(packet2Buffer, UDP_PACKET_SIZE);
Udp.endPacket();
}


*/
