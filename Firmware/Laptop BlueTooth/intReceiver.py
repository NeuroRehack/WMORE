import asyncio
import time
from bleak import BleakScanner
from bleak import BleakClient
import os
import keyboard

async def main():
    # Look for WMORE devices
    wmore_dev = None
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == "WMORE Test scanData Dev0":
            wmore_dev = d
            print()
            print(f"WMORE Dev Found: {d.address} --- {d.name}")
            print()
            break
    if wmore_dev is None:
        print("NO WMORE DEVICES FOUND")
        return
    # Connecting to found device
    async with BleakClient(wmore_dev.address) as client:
        print("Connecting to device")
        print()
        data_service = None
        data_char_1 = None
        data_char_2 = None
        data_char_3 = None
        print("Device Services --------------------------------")
        for service in client.services:
            print(f"Service: {service}")

            

            try:

                for characteristic in service.characteristics:
                    print("Device Characteristics -------------------------")
                    print(f"Characteristic: {characteristic}")

                    data_char_1 = service.get_characteristic("18ee1516-016b-4bec-ad96-bcb96d166e9a")
                    if data_char_1 is not None:
                        print("FOUND THE RIGHT SERVICE AND CHARACTERISTIC")
                        print(type(data_char_1))
                        data_service = service
                        data_char_2 = service.get_characteristic("18ee1516-016b-4bec-ad96-bcb96d166e9b")
                        data_char_3 = service.get_characteristic("18ee1516-016b-4bec-ad96-bcb96d166e9c")
                        break

                    else:
                        print("CHARACTERISTIC IS INCORRECT")
                        continue

                    
            except Exception as e:
                print(f"Error -------------------------\r\n{e}")
                exit(1)

            if data_service is None or data_char_1 is None or data_char_2 is None or data_char_3 is None:
                print("Service and Characteristics NOT found (major w)")
                print("Continuing loop")
                continue
            else:
                break

        print("\r\n\r\n\r\n")
        print("Service and Characterstics found (major w)")
        print("-----------------------------------------------")

        print(f"Data Service: {data_service}\r\n")
        print(f"Data Characteristic 1: {data_char_1}\r\n")
        print(f"Data Characteristic 2: {data_char_2}\r\n")
        print(f"Data Characteristic 3: {data_char_3}\r\n")
        
        time.sleep(1)
        os.system('cls')



        # Reading from Characteristics
        while True:
            if keyboard.is_pressed('s'):
                c = input(f"Enter Y to Disconnect from {wmore_dev.name}")
                if c == "Y":
                    print(f"Disconnecting from {wmore_dev.name}")
                    value = await client.disconnect()
                    if value:
                        print(f"Device disconnected successfully")
                        exit(0)
                else:
                    print(f"Connection to {wmore_dev.name} will persist.")


            data_1 = await client.read_gatt_char(data_char_1)
            data_2 = await client.read_gatt_char(data_char_2)
            data_3 = await client.read_gatt_char(data_char_3)
            # os.system('cls')

            # data_1 = client.read_gatt_char(data_char_1)
            # data_2 = client.read_gatt_char(data_char_2)
            # data_3 = client.read_gatt_char(data_char_3)
            

            # print(f"Data Point 1: {int(data_1)}")
            # print(f"Data Point 2: {int(data_2)}")
            # print(f"Data Point 2: {int(data_3)}")
            # print(f"Data Point 1: {data_1}")
            # print(f"Data Point 1 Type: {type(data_1)}")
            print(f"Data Point 1: {int.from_bytes(data_1, 'little')}")
            print(f"Data Point 2: {int.from_bytes(data_2, 'little')}")
            print(f"Data Point 3: {int.from_bytes(data_3, 'little')}")
            # print(f"Data Point 2: {type(data_2)}")
            # print(f"Data Point 2: {type(data_3)}")
            # time.sleep(0.1)
            # os.system('cls')
            





        time.sleep(1)
    value = await client.disconnect()

    print(f"Disconnected from device {value}")

asyncio.run(main())