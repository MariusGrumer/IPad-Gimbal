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

bool CPeerList::isEmpty()
{
    return (head == NULL) ? true : false;
}
bool CPeerList::addPeer(int pId)
{
    // create new element
    CPeer *newPeer = new CPeer(pId);
    // list empty
    if (isEmpty())
        end = head = newPeer;
    else
    {
        // last to new element
        end->nachfolger = newPeer;
        // new gets last element
        end = newPeer;
    }
    return true;
}
void CPeerList::printList()
{
    CPeer *p = head;
    while (p != NULL)
    {
        Serial.print("Peer mit ID: ");
        Serial.println(p->mId);
        Serial.println("besitzt folgende Daten: ");
        Serial.print("vel: ");
        Serial.println(p->mStateData.vel);
        Serial.print("ver: ");
        Serial.println(p->mStateData.ver);
        Serial.print("hor: ");
        Serial.println(p->mStateData.hor);

        Serial.print("servoVel: ");
        Serial.println(p->mStateData.servoVel);
        Serial.print("angleServo1: ");
        Serial.println(p->mStateData.angleServo1);
        Serial.print("angleServo2: ");
        Serial.println(p->mStateData.angleServo2);
        p = p->nachfolger;
    }
}
bool CPeerList::deleteList()
{
    if (isEmpty())
        return true;
    while (head->nachfolger != NULL)
    {
        // search prelast element
        CPeer *prelastPeer = head;
        while (prelastPeer->nachfolger != end)
        {
            prelastPeer = prelastPeer->nachfolger;
        }
        delete end;

        prelastPeer->nachfolger = NULL;
        end = prelastPeer;
    }
    delete head;
    return true;
}

bool CPeerList::getDataFromPeer(int pId, SStateData &pStateData)
{
    if (this->isEmpty())
    {
        return false;
    }
    CPeer *p = head;
    while (p->mId != pId) // go trough list and find Peer with ID
    {
        p = p->nachfolger;
        if (p == NULL) // no Id found
        {
            Serial.println("no peer with ID found");
            return false;
        }
    }
    pStateData = p->mStateData;
    return true;
}

bool CPeerList::setDataFromPeer(int pId, SStateData pStateData)
{
    if (this->isEmpty())
    {
        return false;
    }
    CPeer *p = head;
    while (p->mId != pId) // go trough list and find Peer with ID
    {
        if (p == end) // no ID found
        {
            Serial.println("no peers with matching ID found");
            return false;
        }
        p = p->nachfolger;
    }
    p->setData(pStateData);
    return true;
}
CPeerList::CPeerList()
{
    head = end = NULL;
}

CPeerList::~CPeerList()
{
}
