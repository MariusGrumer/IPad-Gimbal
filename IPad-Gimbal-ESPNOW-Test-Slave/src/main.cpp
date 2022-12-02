#include <Arduino.h>

// Libs
#define LED_BUILTIN 13

#include "CEspNowSlave.h"
#include <WiFi.h>

void setup()
{
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Client Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  start = millis();

#ifdef SAVE_CHANNEL
  EEPROM.begin(10);
  lastChannel = EEPROM.read(0);
  Serial.println(lastChannel);
  if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL)
  {
    channel = lastChannel;
  }
  Serial.println(channel);
#endif
  pairingStatus = PAIR_REQUEST;
}

void loop()
{

  if (autoPairing() == PAIR_PAIRED)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      // Save the last time a new reading was published
      previousMillis = currentMillis;
      // Set values to send
      myData.msgType = DATA;
      myData.id = BOARD_ID;
      esp_err_t result = esp_now_send(serverAddress, (uint8_t *)&myData, sizeof(myData));
    }
  }
}