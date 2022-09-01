//#include <Arduino.h>
#include <M5Stack.h>
//#include <ESP32-Chimera-Core.h>
// alle von M5Stack benötigten Libs
//#include <WiFiClientSecure.h>
//#include <SimpleBLE.h>
//#include <WiFi.h>
//#include <WiFiClient.h>
#include <ESPmDNS.h>
//#include <HTTPClient.h>
//#include <sd_diskio.h>
//#include <sd_defines.h>
//#include <SD.h>
#include <Wire.h>
//#include <Ethernet.h>
#include <SPIFFS.h>
#include <vfs_api.h>
#include <FSImpl.h>
#include <FS.h>
#include <SPI.h>

//#include <WebServer.h>
//#include <AsyncUDP.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
//WebServer server(80);
#include <mcp_can.h>                        // Library für CAN Schnittstelle      https://github.com/coryjfowler/MCP_CAN_lib
#include "EspMQTTClient.h"

// BMS Variables

uint8_t mbuf[13];                           // shared message buffer, daly rs485 is 13 bytes
    // PIN FOR RS-485 DRIVER
    // measured parameters
uint16_t _bat_v = 0;                         // voltage, 10mV
int32_t  _bat_i = 0;                         // current, 0.1A  positive on charge !
int16_t _bat_temp = 0;                         // bat temp, 1C

    // Restkapazität
uint16_t _bat_cap4;                          // Byte 4
uint16_t _bat_cap5;                          // Byte 5
uint16_t _bat_cap6;                          // Byte 6
uint16_t _bat_cap7;                          // Byte 7

unsigned long _bat_rest_Capacity;

uint16_t _bat_cycle;                         // Batteriecycle
uint16_t _bat_chg_v;                         // charge voltage, mV
uint16_t _bat_chg_i;                         // charge current 0.01A
uint16_t _bat_dis_i;                         // discharge current 0.01A

uint16_t _bat_dis_v;                         // discharge voltage mV
uint16_t _bat_soc;                           // state of charge, % (*shared measurement parameter)
uint16_t mqtt_soc = -1;                     // MQTT Set SOC Command
uint16_t _bat_soh = 100;                     // state of health, % (no source - just fake)

uint16_t _bat_maxv;                          // max cell voltage
uint16_t _bat_minv;                          // min cell voltage
uint16_t _bat_maxv_10;                       // max cell voltage  Avg of 10 readings
uint16_t _bat_minv_10;                       // min cell voltage  Avg of 10 readings
uint16_t _bat_maxv_10_average;               // max cell voltage  average of 10 readings
uint16_t _bat_minv_10_average;               // min cell voltage  average of 10 readings
uint8_t _bat_min_max_samples = 3;
uint16_t _bat_min_allow_discharge = 3150;     // Min Battery Voltage after going to empty before allowing discharge
uint16_t _bat_min_increase_discharge = 3250;  // Min Battery before increasing the discharge current

uint8_t  _bat_v_10_Sample_Counter = 0;       // Counter for Samples

uint8_t  _bat_maxc;                          // cell number (1 at negative pole) with max voltage
uint8_t  _bat_minc;                          // cell number (1 at negative pole) with min voltage
uint16_t _bat_stat;                          // status Charge/Discharge 0 = stationary, 1 = charging, 2 = discharging

uint8_t  _bat_charge_MOS;                    // Status Charge MOS    0 = off, 1 = on
uint8_t  _bat_discharge_MOS;                 // Status Discharge MOS 0 = off, 1 = on

// End of BMS Variables

// Inverter Varables

byte mes[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };           // Message Buffer /  can-bms is 8 bytes

uint16_t _minCellVolt = 3000;           // Minimum voltage of the weakest cell  
uint16_t _maxCellVolt = 3600;           // Maximum voltage of the fullest cell     
uint32_t _chargevoltage = 56400;             // current charge voltage in mv  
uint32_t _chargecurrent = 5000;              // max charge current in ma    
uint32_t _chargecurrent_max = 100000;        // max charge current in ma    
uint32_t _inverter_charge_max = 100000;        // max charge current in ma 

uint32_t  _StartTime_Charging_Current_Adjustment = millis();   // Start Timer (Timer) 
uint16_t  _WaitingTime_Charging_Current_Adjustment = 2000;     // Waiting time in ms
bool   _Time_Charging_Current_Adjustment_Flag = false;      // If true, we are in the charging process and have exceeded 3410 mV once

uint32_t _discurrent_max = 100000;                             // max discharge current in ma   
uint32_t _discurrent = _discurrent_max;                         // discharge current in ma    
uint32_t _load_volt_offset = 0;
float _measuredcurrent = 0;

uint16_t _disvoltage = 45000;                // max discharge voltage in mv   --> is not currently supported by the SOLIS

uint16_t _SOH = 100;                         // SOH place holder               

uint16_t _bat_minV_loaded = 0;
uint16_t _bat_minV_allowed = 3000;
//uint32_t A1 = 0;                            // Millis Counter

bool _BatEntladenFlag = false;
bool _BatHighVoltageFlag = false;
bool _Bat3400VoltageFlag = false;
bool _DischargeEnable = true;
bool _ForceCharge = false;

int16_t _batCurrent = 0;

bool _batteryDataValid = false;               // checks whether the current data is correct

// End of Inverter Varables

// Other 

String mqttbasetopic = "emon/BMS/Battery1";

uint16_t BerechnungBusy = 0;

uint16_t BeepON = 0;

uint32_t LoopTimer; // Main loop every 1 second
uint32_t LoopTimerStats; // Statistics update loop (MQTT)

int freq = 25000;
int channel = 0;
int resolution_bits = 8;
int fanpin = 21;

enum speeds {
  FANOFF = 0,
  FANLOW = 25,
  FANMED = 64,
  FANHIGH = 96,
  FANFULL = 128
};
