#pragma once
#include "CPeer.h"

class CPeerList
{
private:
    CPeer *head;
    CPeer *end;

public:
    CPeerList();
    ~CPeerList();
    bool addPeer(int pId);
    bool isEmpty();
    bool deleteList();
    bool getDataFromPeer(int pId, SStateData &pStateData);
    bool setDataFromPeer(int pId, SStateData pStateData);
    void printList();
};