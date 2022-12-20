#pragma once
#include "CPeer.h"

CPeer::CPeer(int pId) : mId(pId)
{
    this->nachfolger = NULL;

    this->mStateData.hor = 0;
    this->mStateData.ver = 0;
    this->mStateData.vel_hor = 0;
    this->mStateData.vel_ver = 0;
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
