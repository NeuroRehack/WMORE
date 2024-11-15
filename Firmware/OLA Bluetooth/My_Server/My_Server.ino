#include <ArduinoBLE.h>


/* WMORE Service UUID */
const char* wmoreDataServiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e99";

/* WMORE Data Characterstics */
const char* wmoreData1CharUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9a";
const char* wmoreData2CharUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9b";
const char* wmoreData3CharUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9c";


BLEService wmoreDataService(wmoreDataServiceUuid);
BLEIntCharacteristic wmoreData1Char(wmoreData1CharUuid, BLERead | BLEBroadcast);
BLEIntCharacteristic wmoreData2Char(wmoreData2CharUuid, BLERead | BLEBroadcast);
BLEIntCharacteristic wmoreData3Char(wmoreData3CharUuid, BLERead | BLEBroadcast);

// Advertising parameters should have a global scope. Do NOT define them in 'setup' or in 'loop'
const uint8_t completeRawAdvertisingData[] = {0x02,0x01,0x06,0x09,0xff,0x01,0x01,0x00,0x01,0x02,0x03,0x04,0x05};

int d1 = 139;
int d2 = 300;
int d3 = 600;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("failed to initialize BLE!");
    while (1);
  }
  
  BLE.setAdvertisedService(wmoreDataService);
  wmoreDataService.addCharacteristic(wmoreData1Char);
  wmoreDataService.addCharacteristic(wmoreData2Char);
  wmoreDataService.addCharacteristic(wmoreData3Char);


  BLE.addService(wmoreDataService);

  wmoreData1Char.writeValue(d1);
  wmoreData2Char.writeValue(d2);
  wmoreData3Char.writeValue(d3);

  // Build advertising data packet
  BLEAdvertisingData advData;
  // If a packet has a raw data parameter, then all the other parameters of the packet will be ignored
  advData.setRawData(completeRawAdvertisingData, sizeof(completeRawAdvertisingData));  
  // Copy set parameters in the actual advertising packet
  BLE.setAdvertisingData(advData);

  // Build scan response data packet
  BLEAdvertisingData scanData;
  scanData.setLocalName("WMORE Test scanData Dev0");
  // Copy set parameters in the actual scan response packet
  BLE.setScanResponseData(scanData);
  
  BLE.advertise();

  Serial.println("advertising ...");
}

void loop() {
  // BLE.poll();
  

  BLEDevice central = BLE.central();

  Serial.println("- Discovering Central Device");
  delay(1000);
  

  

  if (central) {
    Serial.printf("Connected to: %s\r\n", central.deviceName());
    d1 = 0;
    d2 = 0;
    d3 = 0;
    while (central.connected()) {
      d1++;
      d2++;
      d3++;

      wmoreData1Char.writeValue(d1);
      wmoreData2Char.writeValue(d2);
      wmoreData3Char.writeValue(d3);

      // delay(1000);
    }
  }
}
