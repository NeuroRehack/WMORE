#include <ArduinoBLE.h>


/* String serice UUID */
const char* stringServiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e98";
/* String Characteristics */
const char* strSendCharacteristiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9c";
const char* strRecvCharacteristiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9d";

const char* wmoreServiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e99";
const char* wmoreDataSendCharacteristiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9a";
const char* wmoreDataRecvCharacteristiceUuid = "18ee1516-016b-4bec-ad96-bcb96d166e9b";



BLEService wmoreStreamingService(wmoreServiceUuid);
BLEIntCharacteristic wmoreSendDataCharacteristic(wmoreDataSendCharacteristiceUuid, BLERead | BLEBroadcast);
BLEIntCharacteristic wmoreRecvDataCharacteristic(wmoreDataRecvCharacteristiceUuid, BLEWrite | BLEBroadcast);

BLEService myStringService(stringServiceUuid);
BLEStringCharacteristic strSendCharacteristic(strSendCharacteristiceUuid, BLERead | BLEBroadcast, 50);
BLEStringCharacteristic strRecvCharacteristic(strSendCharacteristiceUuid, BLEWrite | BLEBroadcast, 50);

// BLEStringCharacteristic wmoreStrSendDataCharacteristic(wmoreDataStrSendCharacteristiceUuid, BLERead | BLEBroadcast);
// BLEStringCharacteristic wmoreStrRecvDataCharacteristic(wmoreDataStrRecvCharacteristiceUuid, BLEWrite | BLEBroadcast);

// Advertising parameters should have a global scope. Do NOT define them in 'setup' or in 'loop'
const uint8_t completeRawAdvertisingData[] = {0x02,0x01,0x06,0x09,0xff,0x01,0x01,0x00,0x01,0x02,0x03,0x04,0x05};   



void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("failed to initialize BLE!");
    while (1);
  }
  
  BLE.setAdvertisedService(wmoreStreamingService);
  wmoreStreamingService.addCharacteristic(wmoreSendDataCharacteristic);
  wmoreStreamingService.addCharacteristic(wmoreRecvDataCharacteristic);

  BLE.setAdvertisedService(myStringService);
  wmoreStreamingService.addCharacteristic(strSendCharacteristic);
  wmoreStreamingService.addCharacteristic(strRecvCharacteristic);

  BLE.addService(wmoreStreamingService);
  BLE.addService(myStringService);

  wmoreSendDataCharacteristic.writeValue(1);
  strSendCharacteristic.writeValue("Hello\r\n");

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
  delay(500);

  if (central) {
    Serial.printf("Connected to: %s\r\n", central.deviceName());

    while (central.connected()) {
      if (wmoreRecvDataCharacteristic.written()) {
        int num = wmoreRecvDataCharacteristic.value();
        
        Serial.printf("Recved: %d\r\n", num);
        num += 1;
        Serial.printf("Sending: %d\r\n", num);
        
        wmoreSendDataCharacteristic.writeValue(num);
        Serial.printf("Sent!");

        if (num == 1) {
          int i = 0;
          while (!wmoreRecvDataCharacteristic.written()) {
          // while (i < 10) {
            wmoreSendDataCharacteristic.writeValue(i);
            Serial.printf("Sending: %d\r\n", i);
            delay(1000);
            i++;
          }
        }


        delay(10);
      }

      if (strRecvCharacteristic.written()) {
        Serial.print("Recved Message. Sending it back to Central\r\n");
        // char* recvStr = (uint8_t*) (strRecvCharacteristic.value());
        String recvStr = strRecvCharacteristic.value();
        strSendCharacteristic.writeValue(recvStr);
        // Serial.printf("Recieved Message: %s", (char*) recvStr);
      }
    }
  }

}