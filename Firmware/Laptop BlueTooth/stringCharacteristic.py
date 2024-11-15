import asyncio
import time
from bleak import BleakScanner
from bleak import BleakClient
import os
import keyboard
global recv
recv = 0
MSG_LEN = 512
async def read_from_client(client, uuid) -> str:
    string = None
    string = await client.read_gatt_char(uuid)
    global recv 
    recv += 1
    return string

async def main():
    # Look for WMORE devices
    wmore_dev = None
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == "WMORE Test scanString Dev0":
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
        string_service = None
        string_char_1 = None

        print("Device Services --------------------------------")
        for service in client.services:
            print(f"Service: {service}")
            

            try:

                for characteristic in service.characteristics:
                    print("Device Characteristics -------------------------")
                    print(f"Characteristic: {characteristic}")

                    string_char_1 = service.get_characteristic("18ee1516-016b-4bec-ad96-bcb96d166e9a")
                    if string_char_1 is not None:
                        print("FOUND THE RIGHT SERVICE AND CHARACTERISTIC")
                        print(type(string_char_1))
                        string_service = service
                        break

                    else:
                        print("CHARACTERISTIC IS INCORRECT")
                        continue

                    
            except Exception as e:
                print(f"Error -------------------------\r\n{e}")
                exit(1)

            if string_service is None or string_char_1 is None:
                print("Service and Characteristics NOT found (major w)")
                print("Continuing loop")
                continue
            else:
                break

        if string_service is None or string_char_1 is None:
            print("NOTHING FOUND\r\nPROGRAM EXITTING...")
            value = await client.disconnect()
            exit(0)

        print("\r\n\r\n\r\n")
        print("Service and Characterstics found (major w)")
        print("-----------------------------------------------")

        print(f"String Service: {string_service}\r\n")
        print(f"String Characteristic 1: {string_char_1}\r\n")


        
        time.sleep(1)
        os.system('cls')



        # Reading from Characteristics
        start_time = time.time()
        while True:
            global recv
            delta = time.time() - start_time
            if (recv % 10 == 0) and (delta > 0):
                

                print(F"Delta: {delta}")
                print(F"BITS PER SECOND: {(recv * MSG_LEN * 8) / (delta)}")
                print(F"BYTES PER SECOND: {(recv * MSG_LEN) / (delta)}")
                print(F"TRANSACTIONS PER SECOND: {recv / (delta)}")
                print()
                      
            # if keyboard.is_pressed('s'):
            #     c = input(f"Enter Y to Disconnect from {wmore_dev.name}")
            #     if c == "Y":
            #         print(f"Disconnecting from {wmore_dev.name}")
            #         value = await client.disconnect()
            #         if value:
            #             print(f"Device disconnected successfully")
            #             exit(0)
            #     else:
            #         print(f"Connection to {wmore_dev.name} will persist.")


            # Read from client


            # string_1 = await client.read_gatt_char(string_char_1)
            string_1 = await read_from_client(client, string_char_1)
            # os.system('cls')

            # data_1 = client.read_gatt_char(data_char_1)
            # data_2 = client.read_gatt_char(data_char_2)
            # data_3 = client.read_gatt_char(data_char_3)
            

            # print(f"Data Point 1: {int(data_1)}")
            # print(f"Data Point 2: {int(data_2)}")
            # print(f"Data Point 2: {int(data_3)}")
            # print(f"Data Point 1: {data_1}")
            # print(f"Data Point 1 Type: {type(data_1)}")
            # print(f"Data Point 1: {str.from_bytes(string_1, 'little')}")

            if string_1 is not None:
                # print(f"String 1: {string_1.decode('utf8')}")
                
                string_1 = None
            else:
                print(f"String 1: ~NO DATA~")

            # print(f"Data Point 2: {type(data_2)}")
            # print(f"Data Point 2: {type(data_3)}")
            # time.sleep(0.1)
            # os.system('cls')
            





        time.sleep(1)
    value = await client.disconnect()

    print(f"Disconnected from device {value}")

asyncio.run(main())