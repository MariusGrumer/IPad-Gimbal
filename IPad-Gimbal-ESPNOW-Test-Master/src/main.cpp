#include <Arduino.h>

// Libs
#include <WiFi.h>
#include "CEspNow.h"

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:
  WiFi.mode(WIFI_AP_STA);
  chan = WiFi.channel();

  initESP_NOW();
}

void loop()
{
  outgoingSetpoints.msgType = DATA;
  outgoingSetpoints.id = 0;
  outgoingSetpoints.temp = random(0, 40);
  outgoingSetpoints.hum = random(0, 100);
  outgoingSetpoints.readingId = counter++;
  esp_now_send(NULL, (uint8_t *)&outgoingSetpoints, sizeof(outgoingSetpoints));
  delay(5000);
}