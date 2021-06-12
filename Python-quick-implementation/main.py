import asyncio
from math import floor
from bleak import BleakClient
from enum import Enum
import pyautogui
from pytweening import easeInBack

address = "2C:BA:BA:2F:E1:DC"
MODEL_UUID = "00002a00-0000-1000-8000-00805f9b34fb"
INFO_UUID = "C8C51726-81BC-483B-A052-F7A14EA3D281"
COMMAND_UUID = "C8C51726-81BC-483B-A052-F7A14EA3D282"
class Modes(Enum):
    No_Comm = 0x0000
    Normal_Comm = 0x0100
    VR_Comm = 0x0800

pyautogui.FAILSAFE = False
async def notification_handler(sender, data):
            """Simple notification handler which prints the data received."""
            print("{0}:".format(data))

async def cantusenotify(client, INFO_UUID):
            while True:
                data = await client.read_gatt_char(INFO_UUID)
                print(data)
                print("Last Reading: {0} {1} {2}".format(data[54], data[55] ,data[56]))
                axisX = (((data[54] & 0xF) << 6) +((data[55] & 0xFC) >> 2))
                axisY = (((data[55] & 0x3) << 8) +((data[56] & 0xFF) >> 0))
                print(axisX, axisY)
                #pyautogui.moveTo(floor(axisX*6.095),floor(axisY*3.428), duration= 0.1, tween=easeInBack)


async def run(address):
    async with BleakClient(address) as client:

        #Prints the model and current comm state
        await client.write_gatt_char(COMMAND_UUID, bytearray.fromhex("0100"), response = True)
        model_number = await client.read_gatt_char(MODEL_UUID)
        current_mode = await client.read_gatt_char(COMMAND_UUID)
        #data = await client.read_gatt_char(INFO_UUID)
        print("Model Number : {0} ".format("".join(map(chr,model_number))))
        print("Current Mode : {0} ({1})".format("".join(current_mode.hex()), Modes(int("0x" + current_mode.hex(), base=16)).name))
        #print("Last Reading: {0}".format(data.hex()))


        await cantusenotify(client, INFO_UUID)

        #await client.start_notify(INFO_UUID, notification_handler)
        await asyncio.sleep(60)
        #await client.stop_notify(INFO_UUID)
            

        
    
        

loop = asyncio.get_event_loop()
loop.run_until_complete(run(address))
print("done")