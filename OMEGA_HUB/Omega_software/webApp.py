from flask import Flask, request, jsonify, render_template
import json

CONNECTED = 0   # Device has only just been connected
BUSY = 1        # Files being downloaded or rtc being set
CHARGING = 2    # Files downloaded rtc set

COORDINATOR = 0
LOGGER = 1
DEBUG = 1

app = Flask(__name__)

stored_devices = []
class DeviceObject:
    def __init__(self, comport, type, status, id):
        self.comport = comport
        self.type = type
        self.status = status
        self.id = id
        
    def get_device_info(self):
        status_dict = {CONNECTED:"connected", BUSY:"busy", CHARGING:"charging"}
        type_dict = {COORDINATOR : "coordinator",LOGGER : "logger"}
        return {"comport": self.comport, "type": type_dict[self.type], "status": status_dict[self.status], "id": self.id}
               
    


@app.route('/receive_data', methods=['POST'])
def receive_data():
    global stored_devices  # Use the global variable

    if request.method == 'POST':
        # Deserialize JSON data to a list of dictionaries
        data = json.loads(request.data)
        stored_devices = []
        # Convert each dictionary to a DeviceInfo object
        stored_devices = [device_data for device_data in data]
        [print(deviceObj) for deviceObj in stored_devices]

    # devices = [
    #     DeviceObject("/dev/ttyUSB0",0,1,2),
    #     DeviceObject("/dev/ttyUSB0",0,1,0),
    # ]
    # [print(deviceObj.get_device_info()) for deviceObj in devices]
    return "hi"

@app.route('/', methods=['GET'])
def show_receive_data():
    return render_template('template.html', devices=stored_devices)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)

