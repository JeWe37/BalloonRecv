#!/usr/bin/env python


import asyncio
import websockets
from datetime import datetime
from time import time

flag = False
lat = 515711000

def genData():
    global flag, lat
    flag = not flag
    lat -= 100000
    if flag:
        return "1500,1.5,2.5,2.5,20.5,90001.5,1001.5,20.5,1.5,2.5,3.5,4.5,0.5,1.5,2.5,3.5,4.5,2.25,10.5,81058000,"+str(lat)+",999.5,270.5,45.5,"+str(int(time()))
    else:
        return "2000,7.5,6.5,6.5,21.5,89000.5,990.5,21.5,4.5,3.5,2.5,1.5,4.5,3.5,2.5,1.5,0.5,2.75,11.5,81059000,"+str(lat)+",1000.5,260.5,40.5,"+str(int(time()))

async def periodic(websocket, path):
    while True:
        await websocket.send('{{"rssi":{},"snr":{},"freqErr":{},"data":"{}"}}'.format(int(time()), 42.5, int(time()), genData()))
        await asyncio.sleep(2)

start_server = websockets.serve(periodic, "localhost", 8080)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
