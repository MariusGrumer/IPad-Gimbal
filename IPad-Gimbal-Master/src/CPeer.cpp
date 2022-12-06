#pragma once
#include "CPeer.h"

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
