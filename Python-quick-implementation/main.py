import asyncio
from bleak import BleakClient
from enum import Enum
import mouse

address = "2C:BA:BA:2F:E1:DC"
MODEL_UUID = "00002a00-0000-1000-8000-00805f9b34fb"
INFO_UUID = "C8C51726-81BC-483B-A052-F7A14EA3D281"
COMMAND_UUID = "C8C51726-81BC-483B-A052-F7A14EA3D282"
class Modes(Enum):
    No_Comm = 0x0000
    Normal_Comm = 0x0100
    VR_Comm = 0x0800

async def notification_handler(sender, data):
            """Simple notification handler which prints the data received."""
            print("{0}:".format(data))

async def cantusenotify(client, seconds, INFO_UUID):
            while True:
                data = await client.read_gatt_char(INFO_UUID)
                print("Last Reading: {0}".format(data.hex()))
                axisX = (((data[54] & 0xF) << 6) +((data[55] & 0xFC) >> 2)) & 0x3FF;
                axisY = (((data[55] & 0x3) << 8) +((data[56] & 0xFF) >> 0)) & 0x3FF;
                mouse.move(axisX, axisY, absolute=True, duration= 0.03)


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


        await cantusenotify(client, 60, INFO_UUID)

        #await client.start_notify(INFO_UUID, notification_handler)
        await asyncio.sleep(60)
        #await client.stop_notify(INFO_UUID)
            

        
    
        

loop = asyncio.get_event_loop()
loop.run_until_complete(run(address))
print("done")