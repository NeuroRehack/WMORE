import asyncio
import time
from bleak import BleakScanner
from bleak import BleakClient
import os
from MyPlotter import Simulation
import Parse
import threading

global recv
recv = 0
MSG_LEN = 42


class Streaming_Client():
    def __init__(self, client_name, service, streaming_char):
        self.client_name = client_name
        self._device = None
        self.client = None
        self.service = service
        self.streaming_char = streaming_char

    def set_device(self, device):
        self._device = device
    
    def get_device(self):
        return self._device

    async def read_from_client(self) -> str:
        string = None
        string = await self.client.read_gatt_char(self.streaming_char)
        global recv 
        recv += 1
        return string
    
    def set_client(self, client):
        self.client = client

    def get_client(self) -> BleakClient:
        return self.client
        



async def find_device(dev_name: str):
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == dev_name:
            return d
    raise ValueError(f"Device '{dev_name}' not found")

async def main():
    d = None
    d = await find_device("WMORE Logger Streamer 0")
    print(d.name)
    streamer = Streaming_Client(d.name, "18ee1516-016b-4bec-a596-bcb96d160405", "18ee1516-016b-4bec-ad96-bcb96d160406")
    streamer.set_device(d)
    async with BleakClient(streamer.get_device().address) as c:
        streamer.set_client(c)
        print("Connected to device")
        
        try:
            
            for s in streamer.client.services:
                print(f"Services: {s}")

                for c in s.characteristics:
                    print(f"Characteristics: {c}")
            # d = []
            # t = threading.Thread(target=Simulation().run, args=(d,))
            # t.start()
            sim = Simulation(filter_width=5)
            start_time = time.time()
            
            while True:
                string = await streamer.read_from_client()
                # delta = time.time() - start_time
                # print(F"MSGS/SEC: {recv / delta}\r\nTIME: {delta}\r\nBITS PER SECOND: {(recv * MSG_LEN * 8) / (delta)}\r\nBYTES PER SECOND: {(recv * MSG_LEN) / (delta)}")
                # print(F"BITS PER SECOND: {(recv * MSG_LEN * 8) / (time.time() - start_time)}")
                # print(F"BYTES PER SECOND: {(recv * MSG_LEN) / (time.time() - start_time)}")
                sim.run(string.decode('utf-8'))
                print(Parse.parse_time(string.decode('utf-8')))
                # print(f"Received String: {string.decode('utf-8')}")
                


        except Exception as e:
            value = await streamer.get_client().disconnect()
            if value:
                print("Disconnected successfully")
                print(e)
    

    
asyncio.run(main())