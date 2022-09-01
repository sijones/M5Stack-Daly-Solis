void fansetup() {
  ledcSetup(channel, freq, resolution_bits);
  ledcAttachPin(fanpin, channel);
}

void updatefanspeed(speeds speed) {
    ledcWrite(channel, speed);
}