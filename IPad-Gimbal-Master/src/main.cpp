#include <ESPmDNS.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h> //https://github.com/Links2004/arduinoWebSockets
#include <WebSocketsServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer and https://github.com/me-no-dev/AsyncTCP
#include <nvs_flash.h>
#include "webpage.h"
#include <Arduino.h>
#include "CPeerList.h"

CPeerList myPeers;
bool newPeerConn = false;
#include "CEspNow.h"

// HS Wlan
// 56616967766283031728

#define stepperTimeout 4000

// AP SSID and Pass
const char *APssid = "ESP32 Setup";
const char *APpassword = "ESP32Setup";

// dns setup
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1337;

// Webserver init
AsyncWebServer server(http_port);
WebSocketsServer webSocket = WebSocketsServer(ws_port);

// Create EEPROM namespace objects
Preferences credentials;
Preferences wifiMeta;
Preferences favPos;
int credentialCounter;

// init functions
void respond(byte *payload, int length, uint8_t client_num);
String scanWifi();
String eepromAction(String ws_payload);
void storeCreds(String payload, Preferences *pref);
bool storePosition(String payload, Preferences *pref);
bool storeWifiMeta(String payload, Preferences *pref);
void showEEPROM();
void initEEPROM();
String getAllFavPos();
bool wifiSetup(int wifiTimeout);
bool staticIPSetup();
void setupAP();
void initmDNS();
String scanWifi();
String splitString(String data, char separator, int index);
void gimbalTest();
void stepperTimeoutCheck();
void websocketPing();

bool espNowSendToPeer(unsigned int Id, SStateData pStateData);
bool getNumOfPeers(unsigned int &numOfPeers);

/*########################## FUNCTIONS ##############################*/

// ########## Web Server Handling ##########
//  Callback send homepage
void onIndexRequest(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("HTTP GET request from [");
    // Serial.print(remote_ip.toString());
    Serial.print("]: ");
    // Serial.println(request->url());
    request->send(200, "text/html", webpageCode);
}

// Callback not found
void onPageNotFound(AsyncWebServerRequest *request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.print("[");
    Serial.print(String(remote_ip));
    Serial.print("] HTTP GET request of ");
    Serial.println(request->url());
    request->send(404, "text/plan", "Page not found!\n    404");
}

// ########## WebSocket Handling ##########
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length)
{

    String cmdRaw;
    byte incoming[length];

    for (int i = 0; i < length; i++)
    {
        cmdRaw += (char)payload[i];
    }
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.print("[");
        Serial.print(client_num);
        Serial.println("] Disconnected!");
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.print("Client [");
        Serial.print(client_num);
        Serial.print("] connecting with ");
        Serial.println(ip.toString());
    }
    break;
    case WStype_TEXT:

        Serial.print("\n[");
        Serial.print(client_num);
        Serial.print("] ");
        Serial.print(cmdRaw);
        for (int i = 0; i < length; i++)
        {
            incoming[i] = payload[i];
        }
        Serial.print("\n");
        respond(incoming, length, client_num);
        break;

    // For everything else: do nothing
    case WStype_ERROR:
        Serial.println("Unknown ERROR");
        break;
    default:
        Serial.print("Unknown WS_TYPE: ");
        Serial.println(WStype_t(type));
        break;
    }
}

void respond(byte *payload, int length, uint8_t client_num)
{
    /*

    Protcol format: cmd:[val],[val],....
    Protocol cmdset:
    0: send available WiFi networks
    1: EEPROM Request
    2: reboot ESP
    3: rotate cam
    4: get/set stepper position
    5: get num of peers

    */

    char cmd = payload[0];
    String data;
    for (int i = 2; i < length; i++)
    { // cmd+":" --> [0, 1, data, data, ...]
        data += char(payload[i]);
    }
    Serial.print("payload: ");
    Serial.println(data);

    String response;
    int count = 0;

    Serial.print("Command: ");
    Serial.println(cmd);

    // Return wifi Signals in Range
    if (cmd == '0')
    {
        Serial.println(" (return wifi signals)");
        response = "0:";
        response += scanWifi();
        webSocket.sendTXT(client_num, response);
        // Serial.println(response);

        // EEPROM request
    }
    else if (cmd == '1')
    {
        // 1: EEPROM request Format: 1:dataID,instruction,Key,Value
        /*
              dataID.         0: wifi credentials
                              1: wifiMeta
                              2: favPos
              instruction:    0: delete
                              1: store
                              2: read
          */
        Serial.println(" (EEPROM Action)");
        response = "1:";
        response += eepromAction(data);

        webSocket.sendTXT(client_num, "1:credentials_stored");

        // Restart ESP
    }
    else if (cmd == '2')
    {
        Serial.println(" (Restart ESP)");
        webSocket.sendTXT(client_num, "3:Restarting");
        ESP.restart();

        // Rotate camera
    }
    else if (cmd == '3')
    {
        // Recv: 3:camID,hor,ver,hor_vel,ver_vel
        // Send: 3:camId,hor,ver

        String camId = splitString(data, ',', 0);
        String hor = splitString(data, ',', 1);
        String ver = splitString(data, ',', 2);
        String hor_vel = splitString(data, ',', 3);
        String ver_vel = splitString(data, ',', 4);

        Serial.print("rotate camera | ID:");
        Serial.println(camId);

        SStateData mStateData; // Saving Incoming Data

        if (!myPeers.getDataFromPeer(camId.toInt(), mStateData)) // Data in chained List
        {
            Serial.print("no Peer found with ID: ");
            Serial.println(camId);
        }

        // writing received data in struct

        mStateData.hor = hor.toFloat();
        mStateData.ver = ver.toFloat();
        mStateData.vel_hor = hor_vel.toFloat();
        mStateData.vel_ver = ver_vel.toFloat();
        if (mStateData.hor / mStateData.vel_hor > mStateData.ver / mStateData.vel_ver)
        {
            // horizontal langsamer
            mStateData.vel_ver = abs((mStateData.ver / mStateData.hor)) * mStateData.vel_hor;
        }
        else
        {
            // vertikal langsamer
            mStateData.vel_hor = abs((mStateData.hor / mStateData.ver)) * mStateData.vel_ver;
        }

        if (mStateData.vel_hor > 100)
        {
            mStateData.vel_hor = 100;
        }

        if (mStateData.vel_ver > 100)
        {
            mStateData.vel_ver = 100;
        }

        // sending received data to peer
        if (!espNowSendToPeer(camId.toInt(), mStateData))
        {
            Serial.print("Error sending data to peer | ID: ");
            Serial.println(camId);
        }

        // store data in linked list
        if (!myPeers.setDataFromPeer(camId.toInt(), mStateData, false))
        {
            Serial.print("no Peer found with ID: ");
            Serial.println(camId);
        }

        // answer to client
        response = "3:";
        response += camId.toInt();
        // response += ",";
        //  response += mStateData.angleServo1; // get horizonzal axis Value (x)
        // response += ",";
        //  response += mStateData.angleServo2; // get vertical axis Value (y)
        webSocket.broadcastTXT(response);

        // get/set stepper Pos
    }
    else if (cmd == '4')
    {
        // 4:get/set, x, y
        String getSet = splitString(data, ',', 0); // get = 0, set = 1
        String hor = splitString(data, ',', 1);
        String ver = splitString(data, ',', 2);
        String camId = splitString(data, ',', 3);

        SStateData mStateData;
        if (camId == "")
        {
            unsigned int numPeers;
            if (!getNumOfPeers(numPeers))
                Serial.println("no Peers connected");
            response = "5:";
            response += numPeers;
            webSocket.sendTXT(client_num, response);
            return;
        }
        if (getSet == "0")
        { // get
            Serial.print(" return stepper positions for ID: ");
            Serial.println(camId);
        }
        else if (getSet == "1")
        { // set
            Serial.println(" (setting stepper positions)");
            if (hor == "")
                hor = "0";
            if (ver == "")
                ver = "0";
            myPeers.getDataFromPeer(camId.toInt(), mStateData);
            mStateData.hor = hor.toFloat();
            mStateData.ver = ver.toFloat();
            Serial.println("in cmd 4 before send to peer");
            if (!myPeers.setDataFromPeer(camId.toInt(), mStateData, false))
            {
                Serial.print("no Peer found with ID ");
                Serial.println(camId);
            }
            espNowSendToPeer(camId.toInt(), mStateData);
        }
        // mStateData.angleServo1 = 0;
        // mStateData.angleServo2 = 0;
        if (!myPeers.getDataFromPeer(camId.toInt(), mStateData))
        {
            Serial.print("no Peer found with ID ");
            Serial.println(camId);
        }

        response = "4:";
        response += 0; // mStateData.angleServo1; // get horizontal axis Value (x)
        response += ",";
        response += 0; // mStateData.angleServo2; // get vertical axis Value (y)
        response += ",";
        response += camId;
        if (!webSocket.sendTXT(client_num, response))
        {
            Serial.println("could not send cmd 4");
        }
    }
    else if (cmd == '5')
    {
        String mode = splitString(data, ',', 0);

        unsigned int numPeers;
        if (!getNumOfPeers(numPeers))
            Serial.println("no Peers connected");
        Serial.print("num of Peers: ");
        Serial.println(numPeers);
        response = "5:";

        if (mode == "0") // send num of peers
        {
            response += "0,";
            response += numPeers;

            if (!webSocket.sendTXT(client_num, response))
            {
                Serial.println("could not send num of Peers");
            }
        }
        if (mode == "1") // send peerlist
        {

            response += "1,";
            response += myPeers.getResponseList();
            Serial.println(response);
            if (!webSocket.sendTXT(client_num, response))
            {
                Serial.println("could not send num of Peers");
            }
        }
    }
    else if (cmd == '6')
    {
        response = "6:";
        response += getAllFavPos();
    }
    else
    {
        Serial.println("(Unknown command!)");
        webSocket.sendTXT(client_num, "100:Unkown Command");
    }
}

bool sendOnNewConn()
{
    String response = "5:1,";
    response += myPeers.getResponseList();
    webSocket.broadcastTXT(response);
    return true;
}

bool sendOnDataRecv()
{
    if (recvId > 0)
    {
        SStateData myData;
        myPeers.getDataFromPeer(recvId, myData);
        String response;
        response = "3:";
        response += recvId;
        webSocket.broadcastTXT(response);
        recvId = 0;
        return true;
    }
    return false;
}

// ########## EEPROM Handling ########## ///EEPROM WILL BE REPLACE WITH ESP32 SPIFFS
String eepromAction(String ws_payload)
{

    /*
    dataID.         0: wifi credentials
                    1: wifiMeta
                    2: favPos
                    3: factory Reset
    instruction:    0: delete
                    1: store
                    2: read
    */

    String dataID = splitString(ws_payload, ',', 0);
    String instruction = splitString(ws_payload, ',', 1);
    String key = splitString(ws_payload, ',', 2);

    String returnVal = "";

    Serial.println("---");
    Serial.print("dataID: ");
    Serial.println(dataID);
    Serial.print("instruction: ");
    Serial.println(instruction);

    Preferences eeprom;
    if (dataID == "0")
    {
        eeprom.begin("Credentials");
        if (instruction == "0")
        {
            eeprom.clear();
            Serial.println("Deleting creds");
        }
        else if (instruction == "1")
            storeCreds(ws_payload, &eeprom);
        else if (instruction == "2")
            eeprom.getString(key.c_str());
        else
            returnVal = "";
    }
    else if (dataID == "1")
    {
        eeprom.begin("wifiMeta");
        if (instruction == "0")
            eeprom.remove(key.c_str());
        else if (instruction == "1")
            storeWifiMeta(ws_payload, &eeprom);
        else if (instruction == "2")
            eeprom.getString(key.c_str());
        else
            returnVal = "";
    }
    else if (dataID == "2")
    {
        eeprom.begin("FavPos");
        if (instruction == 0)
            eeprom.remove(key.c_str());
        else if (instruction == "1")
            returnVal = storePosition(ws_payload, &eeprom);
        else if (instruction == "2")
            eeprom.getString(key.c_str());
        else
            returnVal = "";
    }
    else if (dataID == "3")
    {
        nvs_flash_erase(); // erase the NVS partition and
        nvs_flash_init();  // initialize the NVS partition.
        initEEPROM();      // Create all namespaces
    }
    else
    {
    }

    return returnVal;
}

void storeCreds(String payload, Preferences *pref)
{ // index ssid=2, pass=3
    String ssid = splitString(payload, ',', 2);
    String pass = splitString(payload, ',', 3);

    Serial.print("ssid: ");
    Serial.println(ssid);
    Serial.print("pass: ");
    Serial.println(pass);

    pref->putString("ssid", ssid.c_str());
    pref->putString("pass", pass.c_str());
    int temp = pref->getUInt("counter") + 1;
    pref->putUInt("counter", temp);
    Serial.println("Creds stored!");
}

bool storePosition(String payload, Preferences *pref)
{
    String name = splitString(payload, ',', 2);
    String posX = splitString(payload, ',', 3);
    String posY = splitString(payload, ',', 4);

    if (pref->getString(name.c_str()) == "")
        return false;

    String val = String(posX);
    val += ":";
    val += String(posY);
    pref->putString(name.c_str(), val);
    return true;
}

bool storeWifiMeta(String payload, Preferences *pref)
{ // webhost checks IP validity (javascript Regex)
    String staticIP = splitString(payload, ',', 2);
    String mDNS = splitString(payload, ',', 3);

    Serial.print("IP: ");
    Serial.println(staticIP);
    Serial.print("mDNS: ");
    Serial.println(mDNS);

    if (staticIP != "")
        pref->putString("staticIP", staticIP);
    if (mDNS != "")
        pref->putString("mDNS", mDNS);
    return true;
}

void showEEPROM()
{
    // show wifi credentials
    Serial.println("------ NVS ------");
    credentialCounter = credentials.getUInt("counter", 0);
    if (credentialCounter > 0)
    {
        Serial.print("Found ");
        Serial.print(credentialCounter);
        Serial.println(" stored WiFi Credentials:");
        String eepromSSID = credentials.getString("ssid");
        String eepromPASS = credentials.getString("pass");
        Serial.print("SSID: ");
        Serial.println(eepromSSID);
        Serial.print("Pass: ");
        Serial.println(eepromPASS);
    }
    else
    {
        Serial.print("Found ");
        Serial.print(credentialCounter);
        Serial.println(" stored WiFi Credentials");
    }

    // show wifi Metadata
    String staticIP = wifiMeta.getString("staticIP");
    String mDNS = wifiMeta.getString("mDNS");

    if (staticIP != NULL || mDNS != NULL)
    {
        Serial.println("Found WiFi metadata: ");
        if (staticIP != NULL)
        {
            Serial.print("StaticIP: ");
            Serial.println(staticIP);
        }
        if (mDNS != NULL)
        {
            Serial.print("mDNS: ");
            Serial.println(mDNS);
        }
    }

    // list saved Positions
    // getAllFavPos();
    Serial.println("-----------------");
}

void initEEPROM()
{
    // Setup EEPROM | creating namespaces if they dont exist
    // Create Wifi Namespace
    credentials.begin("Credentials", false);
    // Create Wifi Meta Namespace (static ip and mDNS name)
    wifiMeta.begin("wifiMeta", false);
    // Create position Namespace
    favPos.begin("FavPositions", false);
}

String getAllFavPos()
{
    String returnVal = "";
    int entryCount = (sizeof(favPos) / sizeof(String)) /* - favPos.freeEntries()*/;
    Serial.print(entryCount);
    Serial.println(" saved positions found!");
    for (int i = 0; i < entryCount; i++)
    {
        returnVal;
    }
    return returnVal;
}

// ########## WiFi Handling ##########
bool wifiSetup(int wifiTimeout)
{ // timeout in sec
  // connecting to wifi with timeout
    bool returnVal = false;

    Serial.println("Starting Wifi setup");
    String eepromSSID = credentials.getString("ssid");
    String eepromPASS = credentials.getString("pass");

    bool staticIP = false;
    // Check if wifi credentials are saved on EEPROM
    if (credentialCounter > 0)
    {
        Serial.println("Connecting...");
        // set up static IP if necessary
        staticIP = staticIPSetup();

        // try every saved wifi (currently only one supported)
        // Setup WiFi with dynamic IP if static failed
        if (WiFi.status() != WL_CONNECTED)
        {
            for (int i = 0; i < credentialCounter; i++)
            {

                WiFi.begin(eepromSSID.c_str(), eepromPASS.c_str());
                int timeStamp = millis() + wifiTimeout * 1000;
                while (((WiFi.status() != WL_CONNECTED) && (timeStamp >= millis())))
                {
                    Serial.print(" .");
                    delay(100);
                }
            }
        }
    }
    // Setup wifi as Access Point
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.disconnect(); // Close all connections
        setupAP();
        Serial.print("ESP started network '");
        Serial.print(APssid);
        Serial.print("'. Connect via ");
        Serial.println(WiFi.softAPIP());
    }
    else
    {
        Serial.print("ESP connected to ");
        Serial.print(WiFi.SSID());
        Serial.print(" with ");
        if (staticIP)
            Serial.print("static IP ");
        else
            Serial.print("dynamic IP ");
        Serial.println(WiFi.localIP());
        initmDNS();
    }

    return true;
}

bool staticIPSetup()
{
    bool returnVal = false;

    String IP = wifiMeta.getString("staticIP");
    if (IP != "")
    {
        // Default
        IPAddress gateway;

        IPAddress subnet;
        IPAddress primaryDNS;   // optional
        IPAddress secondaryDNS; // optional
        String eepromSSID = credentials.getString("ssid");
        String eepromPASS = credentials.getString("pass");

        // Try to connect with dynamic ip to WIFI to get Router information
        Serial.println("Fetching WiFi data to setup static IP:");

        if (WiFi.begin(eepromSSID.c_str(), eepromPASS.c_str()))
        {
            while (WiFi.status() != WL_CONNECTED)
            {
            }; // Wait for Connection
            if (WiFi.status() == WL_CONNECTED)
            {

                Serial.print("Subnet Mask: ");
                subnet = WiFi.subnetMask();
                Serial.println(subnet);

                Serial.print("Gateway IP: ");
                gateway = WiFi.gatewayIP();
                Serial.println(gateway);

                Serial.print("DNS 1: ");
                primaryDNS = WiFi.dnsIP(0);
                Serial.println(primaryDNS);

                Serial.print("DNS 2: ");
                secondaryDNS = WiFi.dnsIP(2);
                Serial.println(secondaryDNS);

                // construct staticIP from EEPROM
                int ip_number[4];

                for (int i = 0; i < 4; i++)
                {
                    String temp = splitString(IP, '.', i);
                    ip_number[i] = temp.toInt();
                }
                Serial.print("StaticIP: ");
                Serial.print(ip_number[0]);
                Serial.print(".");
                Serial.print(ip_number[1]);
                Serial.print(".");
                Serial.print(ip_number[2]);
                Serial.print(".");
                Serial.println(ip_number[3]);
                IPAddress local_IP(ip_number[0], ip_number[1], ip_number[2], ip_number[3]);
                returnVal = true;

                // setup staticIP
                // WiFi.mode(WIFI_AP_STA);
                if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
                {
                    Serial.println("Failed to configure STA WiFi!");
                    returnVal = false;
                }
            }
        }
    }
    return returnVal;
}

void setupAP()
{
    // Start wifi as acces point if no network is reachable

    if (!WiFi.softAP(APssid, APpassword))
    {
        Serial.println("SoftAP failiure!\nNo nonnection possible!");
        while (true)
        {
        }
        // Network connection failed and access Point failed
        // ESP is unusable without connection
        // Programm stalls
    }
    // WiFi.mode(WIFI_AP_STA);
}

// Create mDNS name in local network
void initmDNS()
{
    String dnsName = wifiMeta.getString("mDNS");
    if (dnsName == "")
        dnsName = "cam"; // default Name if nothing is stored

    if (MDNS.begin(dnsName.c_str()))
    {
        Serial.print("ESP reachable under http://");
        Serial.print(dnsName);
        Serial.println(".local");
    }
    else
    {
        Serial.println("Error setting up MDNS responder");
    }
}

// Scan for visible networks
String scanWifi()
{
    int stamp = millis();
    String result;
    // Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    // Serial.println("scan done");
    if (n == 0)
    {
        result = "";
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            // Print SSID for each network found
            result += WiFi.SSID(i);
            if (i != n - 1)
                result += ",";
        }
    }
    Serial.print("WifiScan time: ");
    Serial.println(millis() - stamp);
    return result;
}

// ########## Utility functions ##########
String splitString(String data, char separator, int index)
{
    bool found = false;
    int runningIndex = 0;
    String chopped = "";

    for (int i = 0; (i < data.length()) && (!found); i++)
    {
        char currChar = data.charAt(i);

        if (currChar == separator || i == data.length())
        { // catch EOL
            if (runningIndex >= index)
            {
                found = true;
            }
            else
            {
                runningIndex++;
                chopped = "";
            }
        }
        else
        {
            chopped += currChar;
        }
    }
    return chopped;
}

void websocketPing()
{
    static int timestamp = millis();
    String payload = "PING";
    size_t length = sizeof(payload) / sizeof(uint8_t);

    int pingTime = 5000;

    if ((millis() - timestamp) >= pingTime)
    {

        webSocket.broadcastPing(payload);
        timestamp = millis();
    }
}

// Send Data via ESP NOW to any Peer
bool espNowSendToPeer(unsigned int Id, SStateData pStateData)
{
    outgoingSetpoints.msgType = DATA;          // Type of Data
    outgoingSetpoints.id = 0;                  // ID of this ESP (Master = 0)
    outgoingSetpoints.pStateData = pStateData; // Data
    outgoingSetpoints.readingId = Id;          // Target ID
    esp_err_t result;
    result = esp_now_send(NULL, (uint8_t *)&outgoingSetpoints, sizeof(outgoingSetpoints)); // send data
    if (result == ESP_OK)
        return true;
    return false;
}

bool getNumOfPeers(unsigned int &numOfPeers)
{
    numOfPeers = 0;
    esp_now_peer_num_t numberOfPeers;
    if (esp_now_get_peer_num(&numberOfPeers) == ESP_OK && myPeers.isEmpty() == false)
    {
        numOfPeers = numberOfPeers.total_num;
    }
    else
    {
        return false;
    }

    return true;
}
//----------------------------------- ??-Controller loop -----------------------------------
void setup()
{

    int stamp = millis();
    Serial.begin(921600);

    Serial.println("Booting ESP...");

    initEEPROM();
    showEEPROM();

    // start wifi either AP or via network
    WiFi.useStaticBuffers(true);
    WiFi.mode(WIFI_AP_STA); // for ESP NOW needed
    wifiSetup(10);

    Serial.println("Webhost ready");

    // There must be a WiFi connection beyond this point (AP or Router)
    server.on("/", HTTP_GET, onIndexRequest);
    server.onNotFound(onPageNotFound);

    server.begin();
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    Serial.print("(");
    Serial.print(millis() - stamp);
    Serial.println("ms)\n");
    Serial.println("------------ Protocol Log Start ------------");

    // Init ESP-NOW
    chan = WiFi.channel();

    initESP_NOW();
    Serial.println("ESP NOW ready");
}

int lastSent = 0;

void loop()
{
    webSocket.loop();
    // stepperTimeoutCheck();
    if (millis() - lastSent > 5000)
    {
        lastSent = millis();
        // myPeers.printList(); // print peerlist every 10 sec
    }
    if (sendOnDataRecv())
    {
        Serial.println("Data actualized to socket");
    }
    if (newPeerConn)
    {
        sendOnNewConn();
        newPeerConn = false;
    }
}
