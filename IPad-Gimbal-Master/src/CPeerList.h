#pragma once
#include "CPeer.h"

class CPeerList
{
private:
public:
    CPeer *head;
    CPeer *end;

    CPeerList();
    ~CPeerList();
    bool addPeer(int pId);
    bool isEmpty();
    bool deleteList();
    bool getDataFromPeer(int pId, SStateData &pStateData);
    bool setDataFromPeer(int pId, SStateData pStateData, bool override);
    void printList();
    String getResponseList();
};