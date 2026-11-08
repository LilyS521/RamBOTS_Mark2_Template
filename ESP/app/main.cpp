/************************************************
This is the code given to the ESP 32 so that it can communicate with the app. 
Not fully documented.
The errors for the incude will remain there. Using platformIO gets rid of them if implemeted correctly, 
but the specific ESP 32 we are using must be specified.
version: 2.0
*************************************************/

/************************************************
Date: 10/7/2026
Programmer: Lily Solheim
Reviewer:
Changes made: Code was created
Next: Comments for each function need to be made.
Recomendations:
*************************************************/
/************************************************
Date: 11/7/2026
Programmer: Lily Solheim
Reviewer:
Changes made: added comments for most of the code. Also added a characteristic that
takes in a command. There is a dummy command handler.
Next: Comments for each function need to be made.
Recomendations:
*************************************************/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2902.h>

//Universally unique ID (UUID)
//every service, characteristic and descriptor will have a UUID

//https://www.uuidgenerator.net/#google_vignette

//service
#define SVC_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
//Used for Transmit—data sent from the board to the central device (Notify / Read)
#define TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
//Transmit data from app to esp characteristic that handles commands
#define RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *tx;
BLECharacteristic *rx;
//not connected initally
bool connected = false;

//BLE callback is an event-driven handler that executes automatically when a specific BLE event occurs
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s)    { connected = true; }
  //start advertising make the ESP data visible for bluetooth connection
  void onDisconnect(BLEServer *s) { connected = false; BLEDevice::startAdvertising(); }
};

void sendMessage(const String &msg) {
  if (!connected) return;
  tx->setValue((msg + "\n").c_str());
  tx->notify();
}

//dummy handle command function
void handleCmd(const String &cmd){
  if(cmd == "ping"){
    sendMessage("pong");
  }
  else if(cmd == "led on"){
    digitalWrite(2, HIGH);
    sendMessage("led on");
  }
  else if(cmd == "led off"){
    digitalWrite(2, LOW);
    sendMessage("led off");
  }
  else{
    sendMessage("Unknown command");
  }
}

//sets up callbacks for rx/command characteristics
class CmdCharaCallbacks : public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *c){
    String cmd = c->getValue().c_str();
    cmd.trim();
    handleCmd(cmd);
  }
};

void setup() {
  pinMode(2, OUTPUT);

  //initialize the device called ESP32-Msgs
  BLEDevice::init("ESP32-Msgs");

  //create server
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());

  //create service
  BLEService *svc = server->createService(SVC_UUID);

  //create characteristic. Characteristics and descriptors can have a UUID and a value
  tx = svc->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  rx = svc->createCharacteristic(RX_UUID, BLECharacteristic::PROPERTY_WRITE);
  rx->setCallbacks(new CmdCharaCallbacks()); 

  //create decriptor and add it
  tx->addDescriptor(new BLE2902());

  //start service after the characteristics are set up
  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID);   // lets the app find your ESP32

  //starts the advertising
  adv->start();
}

//sends check in message every 2000ms or ever 2s
void loop() {
  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    sendMessage("Hello from ESP32");
  }
}