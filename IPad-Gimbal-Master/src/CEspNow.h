#pragma once

#include <esp_now.h>
unsigned int recvId;
esp_now_peer_info_t slave;
int chan;
uint8_t broadcastAddress1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
enum MessageType
{
    PAIRING,
    DATA,
};
MessageType messageType;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message
{
    uint8_t msgType;
    uint8_t id;
    SStateData pStateData;
    unsigned int readingId;
} struct_message;

typedef struct struct_pairing
{ // new structure for pairing
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingData;

void printMAC(const uint8_t *mac_addr)
{
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
}

bool addPeer(const uint8_t *peer_addr)
{ // add pairing
    memset(&slave, 0, sizeof(slave));
    const esp_now_peer_info_t *peer = &slave;
    memcpy(slave.peer_addr, peer_addr, 6);

    slave.channel = chan; // pick a channel
    slave.encrypt = 0;    // no encryption
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if (exists)
    {
        // Slave already paired.
        Serial.println("Already Paired");
        return true;
    }
    else
    {
        esp_err_t addStatus = esp_now_add_peer(peer);
        if (addStatus == ESP_OK)
        {
            // Pair success
            Serial.println("Pair success");
            myPeers.addPeer(pairingData.id); // Peer zu Liste hinzufügen
            newPeerConn = true;
            return true;
        }
        else
        {
            Serial.println("Pair failed");
            return false;
        }
    }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("Last Packet Send Status: ");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
    printMAC(mac_addr);
    Serial.println();
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    Serial.print(len);
    Serial.print(" bytes of data received from : ");
    printMAC(mac_addr);
    Serial.println();
    String payload;
    uint8_t type = incomingData[0]; // first message byte is the type of message
    switch (type)
    {
    case DATA: // the message is data type
        memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
        myPeers.setDataFromPeer(incomingReadings.id, incomingReadings.pStateData, true);
        recvId = incomingReadings.id;
        break;

    case PAIRING: // the message is a pairing request
        memcpy(&pairingData, incomingData, sizeof(pairingData));
        Serial.println(pairingData.msgType);
        Serial.println(pairingData.id);
        Serial.print("Pairing request from: ");
        printMAC(mac_addr);
        Serial.println();
        Serial.println(pairingData.channel);
        if (pairingData.id > 0)
        { // do not replay to server itself
            if (pairingData.msgType == PAIRING)
            {

                // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
                WiFi.softAPmacAddress(pairingData.macAddr);
                pairingData.channel = chan;
                Serial.println("send response");
                addPeer(mac_addr);
                pairingData.id = 0; // 0 is server -> reset after adding Peer
                esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&pairingData, sizeof(pairingData));
            }
        }
        break;
    }
}

void initESP_NOW()
{
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}
