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
    bool setData(SStateData pStateData);
    ~CPeer();
};

CPeer::CPeer(int pId) : mId(pId)
{
    this->nachfolger = NULL;
}

bool CPeer::setData(SStateData pStateData)
{
    mStateData = pStateData;
    return true;
}
SStateData CPeer::getData()
{
    return mStateData;
}
CPeer::~CPeer()
{
}
