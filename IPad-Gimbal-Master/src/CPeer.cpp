#pragma once
#include "CPeer.h"

CPeer::CPeer(int pId) : mId(pId)
{
    this->nachfolger = NULL;
    this->mStateData.angleServo1 = 0;
    this->mStateData.angleServo2 = 0;
    this->mStateData.hor = 0;
    this->mStateData.ver = 0;
    this->mStateData.servoVel = 0;
    this->mStateData.vel = 0;
}

bool CPeer::setData(SStateData pStateData)
{
    mStateData = pStateData;
    Serial.println("Setting Peer data to: ");
    Serial.print("A1 value:");
    Serial.println(mStateData.angleServo1);
    Serial.print("A2 value:");
    Serial.println(mStateData.angleServo2);
    return true;
}
SStateData CPeer::getData()
{
    return mStateData;
}
CPeer::~CPeer()
{
}
