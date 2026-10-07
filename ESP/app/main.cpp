/************************************************
This is the code given to the ESP 32 so that it can communicate with the app. 
Not fully documented.
The errors for the incude will remain there. Using platformIO gets rid of them if implemeted correctly, 
but the specific ESP 32 we are using must be specified.
version: 1.0
*************************************************/

/************************************************
Date: 10/7/2026
Programmer: Lily Solheim
Reviewer:
Changes made: Code was created
Next: Comments for each function need to be made.
Recomendations:
*************************************************/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SVC_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *tx;
bool connected = false;

class Callbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s)    { connected = true; }
  void onDisconnect(BLEServer *s) { connected = false; BLEDevice::startAdvertising(); }
};

void sendMessage(const String &msg) {
  if (!connected) return;
  tx->setValue((msg + "\n").c_str());
  tx->notify();
}

void setup() {
  BLEDevice::init("ESP32-Msgs");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new Callbacks());

  BLEService *svc = server->createService(SVC_UUID);
  tx = svc->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  tx->addDescriptor(new BLE2902());
  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID);   // lets the app find your ESP32
  adv->start();
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    sendMessage("Hello from ESP32");
  }
}