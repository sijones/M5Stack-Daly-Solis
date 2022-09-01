// **** SOLIS CAN ******

// Variables for sending the parameters via the CAN bus
// The values overwrite the values stored in the inverter if they are smaller (e.g. charging current / discharging current)
// The SOLIS inverter must be set to the 'User Battery' setting.
// The charging voltages in the SOLIS must be set high enough that the M5Stack and not the SOLIS stop charging.
// Especially if the battery voltage measurement of the SOLI deviates, the switch-off voltages must be added accordingly.

// If the charging current only increases to approx. 30A / 1500 watt after switching the SOLIS Hybrid off and on again, you have to
// once in the Battery User Setting and simply save again without changing the parameters. This behavior
// could be found in several SOLIS hybrids. I don't know the exact cause of this behavior.
#define CAN_INT 15                          //CAN Init Pin for M5Stack

MCP_CAN CAN(12);                            //set CS pin for can controller

  void InverterSetup() {

    M5.Lcd.clear();

    M5.Lcd.setCursor(5, 30);
    M5.Lcd.printf("CAN Test");
    M5.Lcd.setCursor(5, 50);

    // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        M5.Lcd.printf("MCP2515 Init successfully!");
    }
    else {
        M5.Lcd.clear();
        M5.Lcd.fillScreen(TFT_RED);
        M5.Lcd.setCursor(0, 110);
        M5.Lcd.println("Error Initializing MCP2515...");
        M5.Lcd.println("Please restart the M5 stack!");
        delay(60000);

    }
    CAN.setMode(MCP_NORMAL);                             // Change to normal mode to allow messages to be transmitted
    delay(2000);

}

void InverterUpdate()
{

    // Battery charge and discharge parameters
    mes[0] = lowByte(uint16_t(_chargevoltage / 100));              // Maximum battery voltage
    mes[1] = highByte(uint16_t(_chargevoltage / 100));
    mes[2] = lowByte(int16_t(_chargecurrent / 100));              // Maximum charging current 
    mes[3] = highByte(int16_t(_chargecurrent / 100));
    mes[4] = lowByte((_DischargeEnable == false) ? 0 : int16_t(_discurrent / 100));                 // Maximum discharge current 
    mes[5] = highByte((_DischargeEnable == false) ? 0 : int16_t(_discurrent / 100));
    mes[6] = lowByte(int16_t(_disvoltage / 100));                 // Currently not used by SOLIS
    mes[7] = highByte(int16_t(_disvoltage / 100));                // Currently not used by SOLIS

    CAN.sendMsgBuf(0x351, 0, 8, mes);

    delay(20);

    // Current SOC / SOH of the battery

    mes[0] = lowByte((_ForceCharge == false) ? _bat_soc : (_bat_soc > 97) ? _bat_soc : 1);
    mes[1] = highByte((_ForceCharge == false) ? _bat_soc : (_bat_soc > 97) ? _bat_soc : 1);
    mes[2] = lowByte(_SOH);
    mes[3] = highByte(_SOH);
    mes[4] = 0;
    mes[5] = 0;
    mes[6] = 0;
    mes[7] = 0;

    CAN.sendMsgBuf(0x355, 0, 8, mes);

    delay(20);

    // Current measured values of the BMS battery voltage, battery current, battery temperature

    mes[0] = lowByte(uint16_t(_bat_v));
    mes[1] = highByte(uint16_t(_bat_v));
    mes[2] = lowByte(int16_t(_bat_i));
    mes[3] = highByte(int16_t(_bat_i));
    mes[4] = lowByte(int16_t(_bat_temp * 10));
    mes[5] = highByte(int16_t(_bat_temp * 10));

    CAN.sendMsgBuf(0x356, 0, 8, mes);

    delay(20);

}
