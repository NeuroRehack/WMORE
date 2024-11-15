#include <ArduinoBLE.h>

#define STRING_SIZE 100
/* WMORE Service UUID */
const char* wmoreStringServiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e99";

/* WMORE String Characterstics */
const char* wmoreString1CharUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9a";

BLEService wmoreStringService(wmoreStringServiceUuid);
BLEStringCharacteristic wmoreString1Char(wmoreString1CharUuid, BLERead | BLEBroadcast, STRING_SIZE);

// Advertising parameters should have a global scope. Do NOT define them in 'setup' or in 'loop'
const uint8_t completeRawAdvertisingData[] = {0x02,0x01,0x06,0x09,0xff,0x01,0x01,0x00,0x01,0x02,0x03,0x04,0x05};

// char my_string[STRING_SIZE] = {'\0'};
void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("failed to initialize BLE!");
    while (1);
  }
  
  BLE.setAdvertisedService(wmoreStringService);
  wmoreStringService.addCharacteristic(wmoreString1Char);
  


  BLE.addService(wmoreStringService);

  // wmoreString1Char.writeValue(my_string);
  wmoreString1Char.writeValue("This is my string I hope the whole thing sends\r\n");

  // Build advertising data packet
  BLEAdvertisingData advData;
  // If a packet has a raw data parameter, then all the other parameters of the packet will be ignored
  advData.setRawData(completeRawAdvertisingData, sizeof(completeRawAdvertisingData));  
  // Copy set parameters in the actual advertising packet
  BLE.setAdvertisingData(advData);

  // Build scan response data packet
  BLEAdvertisingData scanData;
  scanData.setLocalName("WMORE Test scanString Dev0");
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
    
    while (central.connected()) {

      // wmoreString1Char.writeValue(my_string);
      wmoreString1Char.writeValue("This is my string I hope the whole thing sends\r\n\r\nThis is another 50 chars totalling 100 chars !");

      delay(1000);
    }
  }
}
