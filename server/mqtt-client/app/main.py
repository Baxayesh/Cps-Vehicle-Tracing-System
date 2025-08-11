import logging
import struct
import math
from dataclasses import dataclass, asdict
from datetime import datetime
from typing import NamedTuple
import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion
from config import Config

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] [%(levelname)s] %(message)s',
)
logger = logging.getLogger("VehicleMonitor")
config = Config()
from zoneinfo import ZoneInfo
LOCAL_TZ = ZoneInfo("Asia/Tehran")

es = config.create_elasticsearch_client()


def ensure_index_exists(index_name: str):
    try:
        if not es.indices.exists(index=index_name):
            mapping = {
                "mappings": {
                    "properties": {
                        "location": {"type": "geo_point"},
                        "vehicle": {"type": "keyword"},
                        "altitude": {"type": "float"},
                        "acceleration_magnitude": {"type": "float"},
                        "velocity_magnitude": {"type": "float"},
                        "time": {"type": "date"}
                    }
                }
            }
            es.indices.create(index=index_name, body=mapping)
            logger.info(f"Created index '{index_name}' with mapping")
        else:
            logger.info(f"Index '{index_name}' already exists")
    except Exception as e:
        logger.error(f"Failed to check/create index '{index_name}': {e}")
        exit(1)


class Vector(NamedTuple):
    x: float
    y: float
    z: float

    def to_dict(self):
        return self._asdict()

    def magnitude(self):
        return math.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)


@dataclass
class VehicleStatus:
    time: datetime
    acceleration: Vector
    velocity: Vector
    angular_velocity: Vector
    orientation: Vector
    location: Vector
    is_location_dead_reckoned: bool
    location_freshness: int
    signal_strength: int
    battery_status: int


def parse_payload(payload: bytes) -> VehicleStatus | None:
    if len(payload) != 74:
        logger.warning(f"Unexpected payload size: {len(payload)} bytes")
        return None

    try:
        data = struct.unpack("<BBBBBH3f3f3f3f3fBIBb", payload)
    except struct.error as e:
        logger.error(f"Payload deserialization failed: {e}")
        return None

    # Validate datetime
    if not (0 <= data[0] <= 59 and 0 <= data[1] <= 59 and 0 <= data[2] <= 23 and
            1 <= data[3] <= 31 and 1 <= data[4] <= 12 and 2000 <= data[5] <= 2100):
        logger.warning("Invalid datetime fields")
        return None

    if not (0 <= data[23] <= 100):
        logger.warning(f"Invalid signal_strength: {data[23]}")
        return None

    return VehicleStatus(
        time=datetime(second=data[0], minute=data[1], hour=data[2],
                      day=data[3], month=data[4], year=data[5], tzinfo=LOCAL_TZ), #todo:timezone
        acceleration=Vector(data[6], data[7], data[8]),
        velocity=Vector(data[9], data[10], data[11]),
        angular_velocity=Vector(data[12], data[13], data[14]),
        orientation=Vector(data[15], data[16], data[17]),
        location=Vector(data[18], data[19], data[20]),
        is_location_dead_reckoned=bool(data[21]),
        location_freshness=data[22],
        signal_strength=data[23],
        battery_status=data[24]
    )


def status_to_es_doc(status: VehicleStatus) -> dict:
    return {
        "message_arrival": datetime.now(LOCAL_TZ).isoformat(),
        "time": status.time.isoformat(),
        "acceleration": status.acceleration.to_dict(),
        "acceleration_magnitude": status.acceleration.magnitude(),
        "velocity": status.velocity.to_dict(),
        "velocity_magnitude": status.velocity.magnitude(),
        "angular_velocity": status.angular_velocity.to_dict(),
        "orientation": status.orientation.to_dict(),
        "location": {
            "lat": status.location.y,
            "lon": status.location.x,
        },
        "vehicle": "cps-tracer",
        "altitude": status.location.z,
        "is_location_dead_reckoned": status.is_location_dead_reckoned,
        "location_freshness": status.location_freshness,
        "signal_strength": status.signal_strength,
        "battery_status": status.battery_status,
    }


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        logger.info("Connected to MQTT broker")
        client.subscribe(config.mqtt_topic)
        logger.info(f"Subscribed to topic: {config.mqtt_topic}")
    else:
        logger.error(f"Connection failed with reason code {reason_code}")


def on_message(client, userdata, msg):

    userdata
    status = parse_payload(msg.payload)

    if not status:
        return
    
    logger.info(
        f"New status @ {status.time.strftime('%Y-%m-%d %H:%M:%S')} | "
        f"Loc: ({status.location.x:.5f}, {status.location.y:.5f}, alt={status.location.z:.1f}m, "
        f"{'DR' if status.is_location_dead_reckoned else 'GNSS'}) | "
        f"Vel: |v|={status.velocity.magnitude():.2f} m/s | "
        f"Acc: |a|={status.acceleration.magnitude():.2f} m/s² | "
        f"Sig: {status.signal_strength}% | Bat: {status.battery_status}"
    )

    try:
        doc = status_to_es_doc(status)
        es.index(index=config.es_index, document=doc)
        logger.info(f"Data indexed at {status.time.isoformat()}")
    except Exception as e:
        logger.exception(f"Failed to index data: {e}")


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    logger.warning(f"Disconnected from MQTT broker with rc={reason_code}")


# Main
if __name__ == "__main__":
    ensure_index_exists(config.es_index)

    client = mqtt.Client(
        client_id=config.mqtt_client_id,
        protocol=mqtt.MQTTv311,
        callback_api_version=CallbackAPIVersion.VERSION2
    )
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect

    try:
        logger.info(f"Connecting to MQTT broker at {config.mqtt_server}:{config.mqtt_port}...")
        client.connect(config.mqtt_server, config.mqtt_port, keepalive=10)
    except Exception as e:
        logger.error(f"MQTT connection failed: {e}")
        exit(1)

    logger.info("Starting MQTT loop")
    try:
        client.loop_forever(timeout=2, retry_first_connection=True)
    except KeyboardInterrupt:
        logger.info("Disconnecting MQTT client")
        client.disconnect()
        logger.info("Client stopped")
    except Exception as e:
        logger.exception("Failed during MQTT loop")

    logger.error("MQTT client exited")