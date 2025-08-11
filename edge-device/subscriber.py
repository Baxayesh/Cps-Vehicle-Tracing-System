
from datetime import datetime
import paho.mqtt.client as mqtt
import struct
from dataclasses import dataclass
from typing import NamedTuple
import binascii


# MQTT Configuration
MQTT_SERVER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "ut-cps/vehicle-monitoring"
MQTT_CLIENT_ID = "python_vehicle_listener"


# Define data structures
class Vector(NamedTuple):
    x: float
    y: float
    z: float

@dataclass
class VehicleStatus:
    time: datetime
    acceleration: Vector     # m/s^2, global frame
    velocity: Vector         # m/s, global frame
    angular_velocity: Vector # rad/s
    orientation: Vector      # Euler angles (roll, pitch, yaw in rad)
    location: Vector         # lat/lon
    is_location_dead_reckoned: bool
    location_freshness: int  # milliseconds
    signal_strength: int     # 0–100
    battery_status: int      # Percentage (0–100) or negative for adapter power

# Callback when the client connects to the MQTT broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(MQTT_TOPIC, qos=1)  # Subscribe with QoS 1
        print(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"Connection failed with code {rc}")

# Callback when a message is received
def on_message(client, userdata, msg):
    payload_len = len(msg.payload)

    print(f"\nReceived message on topic {msg.topic}: {payload_len} bytes")

    try:
        # Deserialize binary data (little-endian)
        data = struct.unpack("<BBBBBH3f3f3f3f3fBIBb", msg.payload)

        # Validate fields
        if not (0 <= data[0] <= 59 and 0 <= data[1] <= 59 and 0 <= data[2] <= 23 and
                1 <= data[3] <= 31 and 1 <= data[4] <= 12 and 2000 <= data[5] <= 2100):
            print("Error: Invalid datetime fields")
            return
        if not (0 <= data[23] <= 100):
            print(f"Error: Invalid signal_strength {data[23]} (expected 0–100)")
            return

        # Create VehicleStatus object
        vehicle_status = VehicleStatus(
            time = datetime(
                second=data[0],
                minute=data[1],
                hour=data[2],
                day=data[3],
                month=data[4],
                year=data[5]
            ),
            acceleration=Vector(x=data[6], y=data[7], z=data[8]),
            velocity=Vector(x=data[9], y=data[10], z=data[11]),
            angular_velocity=Vector(x=data[12], y=data[13], z=data[14]),
            orientation=Vector(x=data[15], y=data[16], z=data[17]),
            location=Vector(x=data[18], y=data[19], z=data[20]),
            is_location_dead_reckoned=bool(data[21]),
            location_freshness=data[22],
            signal_strength=data[23],
            battery_status=data[24]
        )


    except struct.error as e:
        print(f"Error deserializing payload: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

# Initialize MQTT client
client = mqtt.Client(client_id=MQTT_CLIENT_ID, protocol=mqtt.MQTTv311)

# Set callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
try:
    client.connect(MQTT_SERVER, MQTT_PORT, keepalive=60)
except Exception as e:
    print(f"Failed to connect to MQTT broker: {e}")
    exit(1)

# Start the loop to process messages
print("Starting MQTT client...")
try:
    client.loop_forever()
except KeyboardInterrupt:
    print("\nDisconnecting from MQTT broker")
    client.disconnect()
    print("Client stopped")