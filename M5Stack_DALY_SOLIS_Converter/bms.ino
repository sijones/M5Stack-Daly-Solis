
#define BMS_HOST_ADDR 0x40                  // host (us) rs485 addr (doc says 0x80, but their software uses 0x40)
#define BMS_SLAVE_ADDR 0x01                 // slave rs485 (doc says 0x01)

#define BMS_RX_TO 1000                      // Timeout guess how long we should wait for a response? (ms)

bool BMSUpdate() {

    bms_tx(0x90);                               // send CMD 90 (voltage, current, SOC)

    if (!bms_rx(0x90)) {
        goto fail;                              // failed to read command
    }
    else {
        _bat_v = bms_word(0) * 10;               // Battery voltage
        _bat_i = bms_word(4) - 30000;            // Battery current negative when charging, 30000 must be subtracted
        _bat_soc = (bms_word(6) / 10.0) + 0.5;   // Battery SOC including correct rounding
    }
    
    if (_bat_soc > 95)
    {
      _chargecurrent_max = 30000;
      //_chargevoltage = 57400;
    } else if (_bat_soc > 90) {
      //_chargevoltage = 57000;
      _chargecurrent_max = 60000;
    } else if (_bat_soc > 80) {
      //_chargecurrent_max = 90000;
      //_chargevoltage = 56500;
    } else if (_ForceCharge) {
      _chargecurrent_max = 80000;
      //_chargevoltage = 56000;
    } else {
      _chargecurrent_max = _inverter_charge_max;
    }
    _chargevoltage = 56400;
    _measuredcurrent = (((_bat_i / 10.0) * -1.0));
            
    delay(75);                                  // 20 ms Pause

    bms_tx(0x91);                               // send 91 (max/min cell voltage)

    if (!bms_rx(0x91)) {
       // goto fail;                              // failed to read command
    }
    else {
        _bat_maxv = bms_word(0);                 // maximum cell voltage
        _bat_maxc = bms_byte(2);                 // max voltage cell number
        _bat_minv = bms_word(3);                 // minimum cell voltage
        _bat_minc = bms_byte(5);                 // min voltage cell number

        if (_bat_v_10_Sample_Counter < _bat_min_max_samples) {      // 3 samples are read in and the average is calculated
            _bat_v_10_Sample_Counter++;
            _bat_minv_10 = _bat_minv_10 + _bat_minv;
            _bat_maxv_10 = _bat_maxv_10 + _bat_maxv;
        }
        else {
            _bat_v_10_Sample_Counter = 1;
            _bat_minv_10 = _bat_minv;
            _bat_maxv_10 = _bat_maxv;
        }

    }

    delay(75);                                  // 20 ms Pause

    bms_tx(0x92);                               // send 92 (max/min temp)

    if (!bms_rx(0x92)) {
        //goto fail;                              // Lesen war nicht erfolgreich
    }
    else {
        _bat_temp = ((int)bms_byte(0) - 40);        // Zelltemperatur. Es muss 40 abgezogen werden
    }

    delay(75);                                  // 20 ms Pause

    /*   Not necessary for SOLIS but can be read out for your own display */

    bms_tx(0x93);                               // send 93 (Charge/discharge Status / Remaining Capacity)

    if (!bms_rx(0x93)) {
      //goto fail;
    }
    else {
      _bat_stat = (bms_byte(0));               // 0 = stationary, 1 = charging, 2 = discharging
      _bat_charge_MOS = (bms_byte(1));         // charge MOS Status
      _bat_discharge_MOS = (bms_byte(2));      // discharge MOS Status
      _bat_rest_Capacity = ((bms_byte(4) * 16777216)) + ((bms_byte(5) * 65536)) + ((bms_byte(6) * 256)) + ((bms_byte(7)));  // Remaining Capacity in mAh
    }
    
    delay(75);

    /*   Not necessary for SOLIS but can be read out for your own display / evaluations */

    bms_tx(0x94);                              // send 94 (Charge discharge Cycle)

    if (!bms_rx(0x94)) {
        //goto fail;
    }
    else {
        _bat_cycle = bms_word(5);               // charge discharge cycle
    }

    // The display is now updated

    M5.Lcd.fillScreen(TFT_BLACK);

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("DALY BMS -> SOLIS 5G");
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.print("RS485    -> CAN Converter");
    M5.Lcd.drawFastHLine(0, 40, 319, 0x07FF);

    M5.Lcd.setCursor(0, 55);                      // Voltage
    M5.Lcd.print("Batt Volt  V : ");
    M5.Lcd.print(_bat_v / 100.0);

    M5.Lcd.setCursor(0, 75);                      // Current
    M5.Lcd.print("Batt-Curr     A : ");
    M5.Lcd.print(_measuredcurrent,1);

    M5.Lcd.setCursor(0, 95);                      // SOC
    M5.Lcd.print("Battery SOC    % : ");
    M5.Lcd.print(_bat_soc);

    M5.Lcd.setCursor(0, 115);                     // Remaining Capacity
    M5.Lcd.print("Battery Remaining Ah: ");
    M5.Lcd.print(_bat_rest_Capacity / 1000);

    M5.Lcd.setCursor(0, 135);                     // Battery cycles
    M5.Lcd.print("Battery Cycles : ");
    M5.Lcd.print(_bat_cycle);

    M5.Lcd.setCursor(0, 155);                     // Temperature
    M5.Lcd.print("Batt-Temp      C : ");
    M5.Lcd.print(_bat_temp);

    M5.Lcd.setCursor(0, 175);                     // Charge MOS
    M5.Lcd.print("Charge MOS-Fet : ");

    if (_bat_charge_MOS == 0) 
      M5.Lcd.print("OFF");
    else
      M5.Lcd.print("ON");
    
    M5.Lcd.setCursor(0, 195);                     // Discharge MOS
    M5.Lcd.print("Discharge MOS-Fet : ");

    if (_bat_discharge_MOS == 0) 
      M5.Lcd.print("OFF");
    else
      M5.Lcd.print("ON");

    // 0 = floating, 1 = charging, 2 = discharging
    switch (_bat_stat) {
    case 0:
        M5.Lcd.setCursor(0, 215);                   // Batt Status
        M5.Lcd.print("Batt-Status :   Floating");
        break;
    case 1:
        M5.Lcd.setCursor(0, 215);                   // Charging
        M5.Lcd.print("Batt-Status :   Charging");
        break;
    case 2:
        M5.Lcd.setCursor(0, 215);                   // Discharging
        M5.Lcd.print("Batt-Status :   Discharging");
        break;
    }


 /* ****** Discharge current ******

    Depending on the battery capacity, steps can be installed here to reduce the charging current, e.b. at the end of the storage capacity.
    Ideally, the discharge current should not exceed 0.5C in solar applications. This increases the life of the battery. 

    Average cell voltage of the last 3 measurements. If no measurement is yet available, the current value is taken
*/

    if (_bat_minv_10_average == 0) {
        _bat_minv_10_average = _bat_minv;
    }

    // Here the average of 3 samples is determined in order to avoid errors when measuring incorrectly or short fluctuations.
    if (_bat_v_10_Sample_Counter == _bat_min_max_samples) {
        _bat_minv_10_average = (_bat_minv_10 / _bat_v_10_Sample_Counter);
    }
  
    if (_bat_stat == 2)
    {
      int x = _measuredcurrent;
      if (x < 0){
        x *= -1;
      }
      if (x > 80) 
        _load_volt_offset = 260;
      else if(x > 60)
        _load_volt_offset = 220;
      else if(x > 40)
        _load_volt_offset = 180;
      else if(x > 30)
        _load_volt_offset = 120;
      else if(x > 20)
        _load_volt_offset = 75;
      else if(x > 10)
        _load_volt_offset = 45;
      else
        _load_volt_offset = 0;
    
      _bat_minV_loaded = _bat_minV_allowed - _load_volt_offset;
    
      if (_bat_minv < _bat_minV_loaded) {             // Lower end of battery reached, discharge is ended !
       // BatEntladenFlag = true;                               // Flag setzen
        client.publish(mqttbasetopic + "/Message", "BatVMin Triggered Low SOC Theshold. BattVMinAvg: " 
        + String(_bat_minv_10_average) + " - BatMinVAllowed: " 
        + String(_bat_minV_allowed) + " - Load Comp: " + String(_bat_minV_allowed - _load_volt_offset));

                                    // If the SOC value delivered by DALY is still greater than 10%, then set the value to 10%
        if (_bat_soc > 10) {         // Less than 10% must be allowed because the battery will continue to discharge easily
            bms_SOC_adjust(10);     // If you set the forced charging in the SOLIS to 10%, it will be briefly recharged from the network as soon as the SOC drops to 9%.
        }
      }
    }
    
    client.publish(mqttbasetopic + "/LoadOffSet", String(_load_volt_offset));
    client.publish(mqttbasetopic + "/LoadCalc", String(_bat_minV_loaded));
    
    if ((_bat_minv_10_average > 3100)) {                       // The battery has some charge, it can be discharged again. Adjust the threshold here as required
        _BatEntladenFlag = false;
    }

    if (_BatEntladenFlag == true) 
      _discurrent = 0;                                       // It may no longer be discharged. The SOLIS is still discharged with 0.4 - 0.6A (internal consumption)
    else
      _discurrent = _discurrent_max;                           // It can be discharged again with the maximum current

    client.publish(mqttbasetopic + "/DischargeLimit", String(int(_discurrent/1000)));
   //  ***** Charging current  ******
 
    // Average cell voltage of the last 3 measurements. If no measurement is available yet, the current value is used
    if (_bat_maxv_10_average == 0) {
        _bat_maxv_10_average = _bat_maxv;
    }

    // Here the average of 3 samples is determined in order to avoid errors when measuring incorrectly or short fluctuations.
    if (_bat_v_10_Sample_Counter == 3) {
        _bat_maxv_10_average = (_bat_maxv_10 / _bat_v_10_Sample_Counter);
    }

    if (_bat_minv_10_average > 3500) {           // Min cell voltage has exceeded 3425 mV.
        _BatHighVoltageFlag = true;
        bms_SOC_adjust(100);                    // Set SOC 100 % 
      } 

    if (_bat_maxv_10_average > 3610) {           // Set Flag, voltage higher then 3460
        _BatHighVoltageFlag = true;
        bms_SOC_adjust(100);                    // Set SOC to 100%
      }

    if (_bat_soc > 100) {
        _BatHighVoltageFlag = true;    
        bms_SOC_adjust(100);                    // Set SOC 100 %
      }

      
    if ((_bat_maxv_10_average < 3380) || (_bat_soc < 98)) {     // Flag reset
        _BatHighVoltageFlag = false;
      }
  

    if (_BatHighVoltageFlag == true) {           // Battery is full, stop charging current
        _chargecurrent = 5;
        goto End_charging_current;
    }
 
   // Dynamic charging current adjustment from 100 amps to 5 amps. The highest cell voltage is readjusted every 5 seconds
 
   if ((millis() - _StartTime_Charging_Current_Adjustment) > _WaitingTime_Charging_Current_Adjustment) {

    _StartTime_Charging_Current_Adjustment = millis();

    if (_bat_maxv_10_average > 3550) {

      _Time_Charging_Current_Adjustment_Flag = true;

      if (_chargecurrent > 13000) {
        _chargecurrent = _chargecurrent - 1000;       // Reduce the charging current by 1 ampere to a minimum of 3 amps
      }
    }
    else
    {
      if ((_chargecurrent < _chargecurrent_max) && (_Time_Charging_Current_Adjustment_Flag == false)) {
        if (_bat_maxv > 3450) {
          _chargecurrent = _chargecurrent + 1000;       // Increase the charging current by 1 ampere only if the FLAG is false
        } else if (_bat_maxv > 3400) {
          _chargecurrent = _chargecurrent + 3000;       // Increase the charging current by 3 ampere only if the FLAG is false
        } else if (_bat_maxv > 3320) {
          _chargecurrent = _chargecurrent + 5000;       // Increase the charging current by 5 ampere only if the FLAG is false
        } else if (_bat_maxv > 3200) {
          _chargecurrent = _chargecurrent + 8000;       // Increase the charging current by 8 ampere only if the FLAG is false
        } else {
          _chargecurrent = _chargecurrent + 10000;       // Increase the charging current by 10 ampere only if the FLAG is false
        } 
      }
    }
    if (_chargecurrent > _chargecurrent_max) _chargecurrent = _chargecurrent_max; // Make sure we don't exceed the max charge rate.
    client.publish(mqttbasetopic + "/ChargeLimit", String((_chargecurrent/1000)));
  }

  if (_bat_maxv_10_average < 3450) {
    _Time_Charging_Current_Adjustment_Flag = false;            // The voltage has dropped, the charging current can be increased again.
  }
 
End_charging_current:

    _batteryDataValid = 1;                           // correct data is available

      InverterUpdate();                                      // Since the data is valid, the data can be sent to the SOLIS. If necessary, further validations of the data can be made before sending.

     //  SerialOutput();                            // A data record with all BMS measured values is output via the serial interface

       return 1;
   
fail:

    _batteryDataValid = 0;                           // no correct data is available

    M5.Lcd.clear();

    M5.Lcd.fillScreen(TFT_RED);

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("BMS Read Error");
    M5.Lcd.setCursor(0, 30);
    M5.Lcd.print("CMD: ");
    M5.Lcd.print(mbuf[2]);
    M5.Lcd.setCursor(0, 60);
    String strcmd = "";

    switch (mbuf[2]) {
      case 0x90:
        strcmd = "SOC";
        break;
      case 0x91:
        strcmd = "Batt V";
        break;
    }
    M5.Lcd.print("CMD: " + strcmd);
   
    return 0;
}

// RS485 Transmit Routine
int bms_tx(uint8_t cmd) {
    int i;
    // prepare tx message
    mbuf[0] = 0xa5;                                     // sync char
    mbuf[1] = BMS_HOST_ADDR;                            // our addr
    mbuf[2] = cmd;                                      // our command
    mbuf[3] = 0x08;
    memset(mbuf + 4, 0, 8);                             // rest must be 0
    mbuf[12] = 0;

    // calculate checksum
    for (i = 0; i <= 11; i++) {
        mbuf[12] += mbuf[i];
    }

    Serial2.write(mbuf, 13);                            // send buffer
    Serial2.flush();                                    // sends all characters

    //delay(50);						// 500ms Pause    
    return 1;

}

// RS485 Receive Routine
int bms_rx(uint8_t cmd) {
    int i;
    uint8_t cs = 0;
    unsigned long start_time = millis();

    delay(100);                                             // 100 ms wait for an answer

    // receive buffer (13 bytes)
    for (i = 0; i <= 12; ) {

        if (Serial2.available() > 0) {                      // serial available?

            mbuf[i] = Serial2.read();                       // if so read it and check is it correct header, Data and Checksum

            if ((i == 0 && mbuf[i] != 0xa5) ||              // first byte = sync char
                (i == 1 && mbuf[i] != BMS_SLAVE_ADDR) ||    // 2nd byte = slave addr
                (i == 2 && mbuf[i] != cmd) ||               // 3rd byte = cmd
                (i == 3 && mbuf[i] != 0x08) ||              // 4th byte = fixed length (8)
                (i == 12 && mbuf[i] != cs)) {               // 13th byte = checksum

                // not frame header - reset
                i = 0;                                      // first byte
                cs = 0;                                     // clear checksum
            }
            else {
                cs += mbuf[i];                              // compute checksum
                i++;                                        // next byte
            }
        }

        // timeout?
        if ((millis() - start_time) > BMS_RX_TO) {
            return 0;                                       // Error
        }
    }

    return 1;                                               // managed to exit for loop with all 13 bytes validated
}

// read a byte from mbuf from data byte offset i
uint16_t bms_byte(int i) {
    return mbuf[i + 4];
}

// read a word from mbuf from data byte offset i
uint16_t bms_word(int i) {
    uint16_t w;
    i += 4;
    w = mbuf[i];
    w <<= 8;
    w += mbuf[i + 1];
    return w;
}

/* Update RS485 SOC on the DALY BMS

  The SOC value must be recalibrated in the BMS at each full charge or discharge point
  This is the only way to ensure that the SOC% display is as accurate as possible

a5 40 21 08 15 03 0b 0c 39 2c 00 64 06   SOC  10% 
a5 40 21 08 15 03 0b 0c 3a 39 03 e8 9b   SOC 100% 

*/

void bms_SOC_adjust(uint8_t SOC) {
        
    if (SOC == 10) {
        // prepare tx message
        mbuf[0] = 0xa5;                                 // sync char
        mbuf[1] = BMS_HOST_ADDR;                        // our addr 40
        mbuf[2] = 0x21;                                 // our command
        mbuf[3] = 0x08;
        mbuf[4] = 0x15;
        mbuf[5] = 0x03;
        mbuf[6] = 0x0b;
        mbuf[7] = 0x0c;
        mbuf[8] = 0x39;
        mbuf[9] = 0x2c;
        mbuf[10] = 0x00;                                // SOC Byte 0
        mbuf[11] = 0x64;                                // SOC Byte 1 in 0.1%    0064 => 100  = 10.0% SOC
        mbuf[12] = 0x06;

        Serial2.write(mbuf, 13);                        // send buffer
        Serial2.flush();

        delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes

        int incomingByte = 0;
        // Drop the rest of the pending frames. 
        while (Serial2.available()) {
            Serial2.read();
        }
        _bat_soc = 10;                                 // Adjust SOC value

    }

    if (SOC == 100) {
        // prepare tx message
        mbuf[0] = 0xa5;                                 // sync char
        mbuf[1] = BMS_HOST_ADDR;                        // our addr
        mbuf[2] = 0x21;                                 // our command
        mbuf[3] = 0x08;
        mbuf[4] = 0x15;
        mbuf[5] = 0x03;
        mbuf[6] = 0x0b;
        mbuf[7] = 0x0c;
        mbuf[8] = 0x3a;
        mbuf[9] = 0x39;
        mbuf[10] = 0x03;                                // SOC Byte 0
        mbuf[11] = 0xe8;                                // SOC Byte 1 in 0.1%    03e8 => 1000  = 100% SOC
        mbuf[12] = 0x9b;

        Serial2.write(mbuf, 13);                        // send buffer
        Serial2.flush();

        delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes

        int incomingByte = 0;

        // Drop the rest of the pending frames. 
        while (Serial2.available()) {
            incomingByte = Serial2.read();
        }
        _bat_soc = 100;                                  // Adjust SOC value
    }

}

// ********  DALY  RS485  Routine End **********


void ButtonAStartMOS() {

// The DISCHARGE-MOS is switched on and then the CHARGE-MOS is switched on
  M5.Speaker.beep();
  delay(5);
  M5.Speaker.mute();
  bms_MOS_ON();
    
}

void ButtonBStopMOS() {

// The charge / discharge current is reduced to 0 and transmitted to the SOLIS
// It is waited for 2 seconds for the SOLIS to turn off the power
// The CHARGE MOS is switched off and then the DISCHARGE MOS is switched off

  M5.Speaker.beep();
  delay(5);
  M5.Speaker.mute();
    _chargecurrent = 0;        // Set charge current to 0.
    _discurrent = 0;           // Set discharge current to 0.
    
    InverterUpdate();                  // Send data to SOLIS

    delay(2000);              // 2 Second wait
    
    bms_MOS_OFF();

}

void bms_MOS_ON()
{
  /*
  _chargecurrent = _chargecurrent_max;
  _discurrent = _discurrent_max;
  bms_MOS("Charge","ON");
  bms_MOS("Discharge","ON");
  */
}
void bms_MOS_OFF() {
  /*
  bms_MOS("Charge","OFF");
  bms_MOS("Discharge","OFF");
  */
}

void bms_MOS(String MOS, String State) {

/*
  int incomingByte = 0;
  uint16_t cs = 0;
  if (MOS == "" || State == "")
    { 
      return;
    }

        // prepare tx message Turn ON Discharge-MOS
        // A5 40 D9 08 01 00 00 00 00 00 00 00 C7
    mbuf[0] = 0xA5;                                 // sync char
    mbuf[1] = BMS_HOST_ADDR;                        // our addr 40
    mbuf[2] = (MOS == "Charge") ? 0xDA : 0xD9;     // our command
    mbuf[3] = 0x08;
    mbuf[4] = (State == "ON") ? 0x01 : 0x00;
    mbuf[5] = 0x00;
    mbuf[6] = 0x00;
    mbuf[7] = 0x00;
    mbuf[8] = 0x00;
    mbuf[9] = 0x00;
    mbuf[10] = 0x00;                                
    mbuf[11] = 0x00;  
    mbuf[12] = 0; 
    for (int i=0;i<=11;i++)
    {
      mbuf[12] += mbuf[i];
    }                             
    
    Serial2.write(mbuf, 13);                        // send buffer
    Serial2.flush();

    delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes

    while (Serial2.available()) {                   // Discard the response from the BMS          
      incomingByte = Serial2.read();
    }
          
    delay(100);                                     // 500mS wait    
  
  else if (MOS == "Discharge" || MOS == "") {
        // prepare tx message  Turn ON Charge-MOS
        // A5 40 DA 08 01 00 00 00 00 00 00 00 C8  
        mbuf[0] = 0xA5;                                 // sync char
        mbuf[1] = BMS_HOST_ADDR;                        // our addr 40
        mbuf[2] = 0xDA;                                 // our command
        mbuf[3] = 0x08;
        mbuf[4] = 0x01;
        mbuf[5] = 0x00;
        mbuf[6] = 0x00;
        mbuf[7] = 0x00;
        mbuf[8] = 0x00;
        mbuf[9] = 0x00;
        mbuf[10] = 0x00;                                
        mbuf[11] = 0x00;                                
        mbuf[12] = 0xC8;

        Serial2.write(mbuf, 13);                        // send buffer
        Serial2.flush();

        //delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes
       
        while (Serial2.available()) {                   // Discard the response from the BMS 
          incomingByte = Serial2.read();
          }
          
        delay(20);                                     // 500mS warten        
       } 

       */ //whole function commented out
}

/*
void bms_MOS_OFF(String MOS) {
           
      int incomingByte = 0;

      if (MOS == "Charge" || MOS == "") {
        // prepare tx message  Turn OFF Charge-MOS
        // A5 40 DA 08 00 00 00 00 00 00 00 00 C7
        mbuf[0] = 0xA5;                                 // sync char
        mbuf[1] = BMS_HOST_ADDR;                        // our addr 40
        mbuf[2] = 0xDA;                                 // our command
        mbuf[3] = 0x08;
        mbuf[4] = 0x00;
        mbuf[5] = 0x00;
        mbuf[6] = 0x00;
        mbuf[7] = 0x00;
        mbuf[8] = 0x00;
        mbuf[9] = 0x00;
        mbuf[10] = 0x00;                                
        mbuf[11] = 0x00;                                
        mbuf[12] = 0xC7;

        Serial2.write(mbuf, 13);                        // send buffer
        Serial2.flush();

        //delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes

               
        while (Serial2.available()) {                   // Discard the response from the BMS 
         incomingByte = Serial2.read();
          }
         
         delay(20);                                    // 500 mS wait
            
        }
        else if (MOS == "Discharge" || MOS == "") {
        // prepare tx message  Turn OFF Discharge-MOS
        // A5 40 D9 08 00 00 00 00 00 00 00 00 C6
        mbuf[0] = 0xA5;                                 // sync char
        mbuf[1] = BMS_HOST_ADDR;                        // our addr 40
        mbuf[2] = 0xD9;                                 // our command
        mbuf[3] = 0x08;
        mbuf[4] = 0x00;
        mbuf[5] = 0x00;
        mbuf[6] = 0x00;
        mbuf[7] = 0x00;
        mbuf[8] = 0x00;
        mbuf[9] = 0x00;
        mbuf[10] = 0x00;                                
        mbuf[11] = 0x00;                                
        mbuf[12] = 0xC6;

        Serial2.write(mbuf, 13);                        // send buffer
        Serial2.flush();

        //delay(100);                                     // 100 ms wait the DALY BMS send a response with 13 bytes
        
        while (Serial2.available()) {                   // Discard the response from the BMS 
          incomingByte =  Serial2.read();
          }
          
         delay(20);                                    // 500 mS wait   
        }
    }
*/

