#pragma once
#include "CPeerList.h"

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
        Serial.print("vel_hor: ");
        Serial.println(p->mStateData.vel_hor);
        Serial.print("vel_ver: ");
        Serial.println(p->mStateData.vel_ver);
        Serial.print("ver: ");
        Serial.println(p->mStateData.ver);
        Serial.print("hor: ");
        Serial.println(p->mStateData.hor);

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
            return false;
        }
    }
    pStateData = p->mStateData;
    return true;
}

bool CPeerList::setDataFromPeer(int pId, SStateData pStateData, bool override)
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
            return false;
        }
        p = p->nachfolger;
    }
    if (override == true)
        p->setData(pStateData);
    else
    {
        p->mStateData.hor = pStateData.hor;
        p->mStateData.ver = pStateData.ver;
        p->mStateData.vel_hor = pStateData.vel_hor;
        p->mStateData.vel_ver = pStateData.vel_ver;
    }
    return true;
}

String CPeerList::getResponseList()
{
    String response = "";
    CPeer *p = head;
    while (p != NULL)
    {
        response += p->mId;
        if (p->nachfolger != NULL)
            response += ",";
        p = p->nachfolger;
    }
    return response;
}

CPeerList::CPeerList()
{
    head = end = NULL;
}

CPeerList::~CPeerList()
{
}
