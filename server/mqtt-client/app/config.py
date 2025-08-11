import os
from elasticsearch import Elasticsearch

class Config:
    def __init__(self):
        self.mqtt_server = os.getenv("MQTT_SERVER", "broker.hivemq.com")
        self.mqtt_port = int(os.getenv("MQTT_PORT", 1883))
        self.mqtt_topic = os.getenv("MQTT_TOPIC", "ut-cps/vehicle-monitoring")
        self.mqtt_client_id = os.getenv("MQTT_CLIENT_ID", "python_vehicle_listener")
        
        self.es_host = os.getenv("ES_HOST", "https://localhost:9200")
        self.es_index = os.getenv("ES_INDEX", "vehicle-status")

        es_username = os.getenv("ES_USER")
        es_password = os.getenv("ES_PASSWORD")

        if es_username and es_password:
            self.es_auth_data = (es_username, es_password)
        else:
            self.es_auth_data = None

        self.es_ca_certificate = os.getenv("ES_CA_CERT", None)

    def create_elasticsearch_client(self):

        if(self.es_ca_certificate):
            return Elasticsearch(
                hosts=[self.es_host],
                basic_auth=self.es_auth_data,
                ca_certs=self.es_ca_certificate
            )
        else:
            return Elasticsearch(
                hosts=[self.es_host],
                basic_auth=self.es_auth_data,
            )