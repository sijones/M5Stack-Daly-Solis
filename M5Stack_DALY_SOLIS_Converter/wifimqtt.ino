

void onMessageReceived(const String& topic, const String& message) 
{
  if (topic == mqttbasetopic + "/set/" + "ChargeMOS" && (message == "ON" || message == "OFF")) {
    bms_MOS("Charge",message);
    }
  else if (topic == mqttbasetopic + "/set/" + "DischargeMOS" && (message == "ON" || message == "OFF")) {
    bms_MOS("Discharge",message);
  }
  else if (topic == mqttbasetopic + "/set/" + "DischargeLimit") {
    _discurrent = message.toInt();
  }
  else if (topic == mqttbasetopic + "/set/" + "ChargeVoltage") {
    if (message.toInt() > 0)
      _chargevoltage = message.toInt();
  }
  else if (topic == mqttbasetopic + "/set/" + "SetSOC") {
    if (message.toInt() > 0) {
      if(message.toInt() == 10)
        bms_SOC_adjust(10);
      else if(message.toInt() == 100)
        bms_SOC_adjust(100);
      else
        client.publish(mqttbasetopic + "/LastMessage", "SOC Set has to be 10 or 100, Sent: " + message);
    }
      //mqtt_soc = message.toInt();
  }
  else if (topic == mqttbasetopic + "/set/" + "ChargeLimit") {
    _chargecurrent = message.toInt();
  }
  else if (topic == mqttbasetopic + "/set/" + "ForceCharge") {
    _ForceCharge = (message == "ON") ? true : false;
    client.publish(mqttbasetopic + "/ForceCharge", (_ForceCharge == true) ? "ON" : "OFF" ); }
  else if (topic == mqttbasetopic + "/set/" + "DischargeEnable") {
    _DischargeEnable = (message == "ON") ? true : false; 
    client.publish(mqttbasetopic + "/DischargeEnable", (_DischargeEnable == true) ? "ON" : "OFF" ); }
  else {
    client.publish(mqttbasetopic + "/LastMessage", "Command not recognised Topic: " + topic + " - Payload: " + message);
  }
}

void onConnectionEstablished()
{
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("MQTT Connected..." );
  client.subscribe(mqttbasetopic + "/set/#", onMessageReceived);
  M5.Lcd.println("MQTT Subscribed..." );
}
