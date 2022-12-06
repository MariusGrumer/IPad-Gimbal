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
