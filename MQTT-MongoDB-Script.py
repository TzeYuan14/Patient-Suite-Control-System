import pymongo
import paho.mqtt.client as mqtt
from datetime import datetime

# MongoDB configuration
mongo_client = pymongo.MongoClient("mongodb://localhost:27017/")
db = mongo_client["patientMonitoring"]
collection = db["patientMonitoring"]

# MQTT configuration
mqtt_broker_address = '35.239.167.63'
mqtt_topic = 'IoTpatientmonitoring'

def on_message(client, userdata, message):
	payload = message.payload.decode('utf-8')
	print(f'Received message: {payload}')

	# Convert MQTT timestamp to datetime
 	timestamp = datetime.utcnow() # Use current UTC time
 	datetime_obj = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")

	sensor_data = payload.split(",")
    	temperature = sensor_data[0].split(":")[1]
    	humidity = sensor_data[1].split(":")[1]
	infrared = sensor_data[2].split(":")[1]
	moisture = sensor_data[3].split(":")[1]
	button1 = sensor_data[4].split(":")[1]
	button2 = sensor_data[5].split(":")[1]
	mq2 = sensor_data[6].split(":")[1]

 	# Process the payload and insert into MongoDB with proper timestamp
 	document = {
 		'timestamp': datetime_obj,
 		'temperature': temperature,
   		'humidity': humidity,
		'patient fall detection': infrared,
		'biowaste moisture': moisture,
		'window curtain status': button1,
		'call button': button2,
		'smoke value': mq2
 	}
 	collection.insert_one(document)
	print('Data ingested into MongoDB')

client = mqtt.Client()
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Subscribe to MQTT topic
client.subscribe(mqtt_topic)

# Start the MQTT loop
client.loop_forever()

# To find the latest data
# db.iot.find().sort({_id: -1}).limit(10) 
