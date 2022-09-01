#include "includes.h"

EspMQTTClient client(
    "", // Wifi SSID
    "", // Wifi PAssword
    "1.1.1.1", // MQTT Server IP
    "emon",    // Omit this parameter to disable client authentification
    "emonmqtt",    // Omit this parameter to disable client authentification
    "SmartBMS", // MQTT Client ID
    1883); // MQTT Port

void setup() {

    M5.begin();                               // Start M5Stack 

    //M5.Power.begin();
    M5.Speaker.setBeep(900, 1000);            // Initialise loudspeaker beep 

    M5.Lcd.setTextColor(TFT_WHITE);           // Display Text Color
    M5.Lcd.setTextFont(1);                    // Text Font
    M5.Lcd.setTextSize(2);                    // Display Text Grösse

    M5.Lcd.setBrightness(125);                // M5Stack background brightness   0-255
    M5.Speaker.mute();

    M5.Lcd.fillScreen(TFT_BLACK);

    //Serial.setTimeout(1000);                  // Set Serial timeout to 1000 ms
    //Serial.begin(115200);                   // Communication with computer. May no longer be set for M5Stack. Outputs a data record every second if activated

    Serial2.setTimeout(2000);                 // Set Serial2 timeout to 2000 ms
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  // RS485 interface of the M5 Stack CommModule for Daly BMS

    InverterSetup();                               // Initialise the CAN interface
  
    M5.Lcd.fillScreen(TFT_BLACK);
    client.enableHTTPWebUpdater();
    client.enableOTA("emonmqtt",8266);
    client.enableMQTTPersistence();
    
    //OTAUpdate();

    //startWebServer(); // starting webserver
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
//    M5.Lcd.println(String(WiFi_hostname));
//    M5.Lcd.println(WiFi.localIP());
  //  M5.Lcd.println((client.isMqttConnected()) ? "MQTT Connected" : "MQTT Disconnected");
    M5.Lcd.setTextSize(2);
    LoopTimer = millis();
  fansetup();
  updatefanspeed(FANOFF);  
}

void loop() {

    M5.update();  
    client.loop();
    if ((millis() - LoopTimer) > 2000) {          // 1 BMS Loop per second
      LoopTimer = millis();
      BMSUpdate();                             // If the BMS reads from us, if the data has been successfully read, the SOLIS is updated
      uint32_t _tempcurrent;
      if (_measuredcurrent >= 0)
        _tempcurrent = _measuredcurrent;
      else
        _tempcurrent = (_measuredcurrent * -1);
      if (_tempcurrent > 85)
        updatefanspeed(FANFULL);
      else if (_tempcurrent > 70)
        updatefanspeed(FANHIGH); 
      else if (_tempcurrent > 45)
        updatefanspeed(FANMED);
      else if (_tempcurrent > 25)
        updatefanspeed(FANLOW);
      else
        updatefanspeed(FANOFF);
    }

    if ((millis() - LoopTimerStats) > 4000) {          // Every 5 seconds run these commands
      LoopTimerStats = millis();
 
      client.publish(mqttbasetopic + "/BattStatus", String(_bat_stat));
      client.publish(mqttbasetopic + "/BattV", String(_bat_v));
      client.publish(mqttbasetopic + "/BattTemp", String(_bat_temp));
      client.publish(mqttbasetopic + "/BattA", String(_measuredcurrent));
      client.publish(mqttbasetopic + "/BattMaxV", String(_bat_maxv_10_average));
      client.publish(mqttbasetopic + "/BattMinV", String(_bat_minv_10_average));
      client.publish(mqttbasetopic + "/ChargeVoltage", String(_chargevoltage)); 
      client.publish(mqttbasetopic + "/ChargeMOS", (_bat_charge_MOS == 0) ? String("OFF") : String("ON"));
      client.publish(mqttbasetopic + "/DischargeMOS", (_bat_discharge_MOS == 0) ? String("OFF") : String("ON"));
      client.publish(mqttbasetopic + "/BattSOC", String(_bat_soc));
      client.publish(mqttbasetopic + "/DischargeEnable", (_DischargeEnable == true) ? "ON" : "OFF" );
      client.publish(mqttbasetopic + "/ForceCharge", (_ForceCharge == true) ? "ON" : "OFF" );
    }

                                           
    /*
    if (M5.BtnA.isPressed()) {
      ButtonAStartMOS();
   
      M5.Lcd.fillScreen(TFT_GREEN);
      M5.Lcd.setTextColor(TFT_BLACK);           // Display Text Color
    
      M5.Lcd.setCursor(0, 80);
      M5.Lcd.println(" Key A was pressed");
      M5.Lcd.println(" ");
      M5.Lcd.println(" ");
      M5.Lcd.println(" DALY MOS switched on");
     
      delay(1000);
   
      M5.Lcd.setTextColor(TFT_WHITE);           // Display Text Color
    } 
    
    else if (M5.BtnB.isPressed()) {
     ButtonBStopMOS();
     } 
    else if (M5.BtnC.isPressed()) {
     
      if (BeepON == 0) {
        BeepON = 1;
      }
      else {
        BeepON = 0;
      }
 
      M5.update();  
      delay(1000);
      M5.Lcd.setTextColor(TFT_WHITE);           // Display Text Color
    }
    */
}
