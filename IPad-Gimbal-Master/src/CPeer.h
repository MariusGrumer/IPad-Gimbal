#pragma once
#include <Arduino.h>
#include "SStateData.h"
class CPeer
{
public:
    int mId;
    SStateData mStateData;

    // Verkettung
    CPeer *nachfolger;

    CPeer(int pId);
    SStateData getData();
    bool setData(float pVel, float pVer, float pHor);
    ~CPeer();
};

CPeer::CPeer(int pId) : mId(pId)
{
    this->nachfolger = NULL;
}

bool CPeer::setData(float pVel, float pVer, float pHor)
{
    mStateData.hor = pHor;
    mStateData.ver = pVer;
    mStateData.vel = pVel;
    return true;
}
SStateData CPeer::getData()
{
    return mStateData;
}
CPeer::~CPeer()
{
}
