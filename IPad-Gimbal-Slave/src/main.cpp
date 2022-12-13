
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "SStateData.h"
#include "StepperMaster.h"
#include "Gimbal.h"
#define BOARD_ID 2
unsigned long currentMillis = millis();
unsigned long previousMillis = 0; // Stores last time temperature was published
const long interval = 10000;      // Interval at which to publish sensor readings
unsigned long start;              // used to measure Pairing time
unsigned int readingId = BOARD_ID;
bool answer = false;

float absolute_angle_one = 0;
float absolute_angle_two = 0;

SStateData mStateData;

// Stepper init          IN1, IN2, IN3, IN4
StepperMaster stepperOne(16, 17, 18, 19); // Horizontal axis
StepperMaster stepperTwo(27, 26, 25, 33); // Vertical axis
Gimbal gimbal(&stepperOne, &stepperTwo, 900);
int stamp;

#include "CEspNowSlave.h"

bool sendData(SStateData pStateData);
void respond_2(int ID, SStateData Servo_state);

void setup()
{
    mStateData.hor = 0;
    mStateData.ver = 0;
    mStateData.vel_hor = 0;
    mStateData.vel_ver = 0;

    Serial.begin(921600);
    Serial.println();
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
        if (answer == true)
        {
            respond_2(BOARD_ID, mStateData);
            if (!sendData(mStateData))
                Serial.println("error Sending Data");
            answer = false;
        }
    }
}

bool sendData(SStateData pStateData)
{
    espNowSendData.msgType = DATA;
    espNowSendData.id = BOARD_ID;
    espNowSendData.readingId = BOARD_ID;
    espNowSendData.pStateData = pStateData;
    esp_err_t result = esp_now_send(serverAddress, (uint8_t *)&espNowSendData, sizeof(espNowSendData));
    if (result == ESP_OK)
    {
        return true;
    }
    return false;
}

void respond_2(int ID, SStateData Servo_state)
{

    if (ID == BOARD_ID)
    {
        stamp = millis();
        gimbal.rotateTo(Servo_state.hor, Servo_state.ver, Servo_state.vel_hor, Servo_state.vel_ver);
        absolute_angle_one = stepperOne.getAbsoluteAngle();
        absolute_angle_two = stepperTwo.getAbsoluteAngle();

        stepperOne.setAbsoluteAngle(Servo_state.hor);
        stepperTwo.setAbsoluteAngle(Servo_state.ver);
    }
}