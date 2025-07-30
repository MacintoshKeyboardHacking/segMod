/* https://www.youtube.com/@macintoshkeyboardhacking/streams */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define UART    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define SERVICE2_UUID       "FE95"
#define SERVICE2_CHA1       "0014"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic *pTxCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint meCount = 0;
uint sendUpdate = 0;

char MANUFDAT[8]={ 0x4E, 0x42, 0x24, 0x00, 0x00, 0x00, 0x00, 0xDB };  
/*
    "esx":      bytearray( [ 0x4E, 0x42, 0x21, 0x00, 0x00, 0x00, 0x00, 0xDE ] ), // Ninebot KickScooter ES 424E2100000000DE
    "m365":     bytearray( [ 0x4E, 0x42, 0x21, 0x00, 0x00, 0x00, 0x00, 0xDF ] ), // Xiaomi M365 424E2100000000DF
    "esxclone": bytearray( [ 0x4E, 0x42, 0x21, 0x02, 0x00, 0x00, 0x00, 0xDC ] ), // Ninebot ESx Clone? 424E2100000000DE
    "m365pro":  bytearray( [ 0x4E, 0x42, 0x22, 0x01, 0x00, 0x00, 0x00, 0xDC ] ), // Xiaomi M365 Pro 424E2201000000DC
    "max":      bytearray( [ 0x4E, 0x42, 0x24, 0x02, 0x00, 0x00, 0x00, 0xD9 ] ), // Ninebot Max 424E2402000000D9
    "max555":   bytearray( [ 0x4E, 0x42, 0x24, 0x00, 0x00, 0x00, 0x00, 0xDB ] ) };  // Ninebot Max BLE555 424E2201000000DC
    23 Ninebot KickScooter Air
    31 Ninebot Gokart Pro
    31 Ninebot Gokart Lambo
    2c Ninebot Kickscooter F
key_ids = { '_pro_keys_char_uuid': "00000014-0000-1000-8000-00805f9b34fb",
            '_max_keys_char_uuid': "0000fe95-0000-1000-8000-00805f9b34fb" }
*/


// not used
void strToBin(byte bin[], char const str[]) {
  for (size_t i = 0; str[i] and str[i + 1]; i += 2) {
    char slice[] = {0, 0, 0};
    strncpy(slice, str + i, 2);
    bin[i / 2] = strtol(slice, nullptr, 16);
  }
}


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


class MyCallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
      Serial.println("CHAREAD");
  }
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("RX:");
        for (int i = 0; i < rxValue.length(); i++){
          if(rxValue[i]<16) Serial.print("0");
          Serial.print(rxValue[i], HEX);          
        }
        Serial.println(":");
        sendUpdate=true;
      }
      
    }
};


// write desired manufacturing datablock to 0xfe95/0x0014 
class tweakCB: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *tweakCha) {
      String manStr = tweakCha->getValue();

      if (manStr.length() > 0) {
        Serial.print("TWEAK:");
        for (int i = 0; i < manStr.length(); i++){
          if(manStr[i]<16) Serial.print('0');
          Serial.print(manStr[i], HEX);          
        }
        Serial.println();
      }
      BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
      BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
      oAdvertisementData.setManufacturerData(manStr);
      pAdvertising->setAdvertisementData(oAdvertisementData);
   }
};


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  // BLE Device
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(UART);

  // UART TX
  pTxCharacteristic = pService->createCharacteristic( UART_TX, BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902 *p2902Descriptor = new BLE2902();
  p2902Descriptor->setIndications(true);
  // p2902Descriptor->setNotifications(true);
  pTxCharacteristic->addDescriptor(p2902Descriptor);

  // UART RX
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic( \
                      UART_RX, \
                      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  // Xiaomi 0xfe95 services
  BLEService *pService2 = pServer->createService(SERVICE2_UUID);
  BLECharacteristic *pService2C = pService2->createCharacteristic("0014", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pService2C->setValue(MANUFDAT); 
  pService2C->setCallbacks(new tweakCB());
  pService2->start();

/*
BLECharacteristic *pSC0001 = pService2->createCharacteristic("0001", BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
pSC0001->addDescriptor(new BLE2902());
pSC0001->setCallbacks(new MyCallbacks());
BLECharacteristic *pSC0002 = pService2->createCharacteristic("0002", BLECharacteristic::PROPERTY_READ);
BLECharacteristic *pSC0004 = pService2->createCharacteristic("0004", BLECharacteristic::PROPERTY_READ);
BLECharacteristic *pSC0010 = pService2->createCharacteristic("0010", BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic *pSC0013 = pService2->createCharacteristic("0013", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
*/

//scan_resp_advertisementData->setManufacturerData(MANUFDAT);
//scan_resp_advertisementData->setShortName("poostick");
//void BLEAdvertising::setAppearance(uint16_t appearance) {
//void BLEAdvertising::setAdvertisementType(esp_ble_adv_type_t adv_type) {

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(UART);
  pAdvertising->addServiceUUID(SERVICE2_UUID);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  

  String manStr(MANUFDAT, 8);
  // kinda feels like it's being done twice... redundant...
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setManufacturerData(manStr);
  pAdvertising->setAdvertisementData(oAdvertisementData);
 
  BLEDevice::startAdvertising();
  Serial.println("Ready...");
}


void loop() {
    if (deviceConnected && !oldDeviceConnected) {
        Serial.println("connected");
        oldDeviceConnected = deviceConnected;
    }
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("disconnected");
        delay(500);
        BLEDevice::startAdvertising();
        oldDeviceConnected = deviceConnected;
    }

    if (Serial.available() > 0) {
        String packet=Serial.readString();
        if (packet.length()>0) {
          pTxCharacteristic->setValue((uint8_t*)&packet,packet.length());
          pTxCharacteristic->notify();
          sendUpdate=false;
        }
    }
    delay(10);
}
