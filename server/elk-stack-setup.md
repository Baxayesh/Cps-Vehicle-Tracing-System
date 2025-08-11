# How to Set Up the ELK Stack with TLS

This guide provides step-by-step instructions to deploy the ELK Stack (Elasticsearch, Logstash, Kibana) using the `docker-elk` repository with TLS enabled. For additional details, refer to [https://github.com/deviantony/docker-elk/tree/tls](https://github.com/deviantony/docker-elk/tree/tls).

## Prerequisites

- Docker and Docker Compose installed
- Git installed
- Basic knowledge of Docker and environment configuration

## Setup Instructions

1. **Clone the docker-elk Repository**
   Clone the `docker-elk` repository into a directory named `elk-stack` using the `tls` branch.

   ```sh
   git clone --branch tls https://github.com/deviantony/docker-elk.git elk-stack
   ```

2. **Modify the .env File (Optional)**
   Navigate to the `elk-stack` directory and review the `.env` file. Update variables such as `ELASTIC_VERSION` or other settings if necessary.

3. **Remove Unnecessary Components**
   Delete the `logstash` directory, `extensions` directory, and any other components not required for your setup.

4. **Update Image Names (Optional)**
   In the `docker-compose.yml` file, you may change the Elasticsearch image reference from `docker.elastic.co/elasticsearch/elasticsearch:${ELASTIC_VERSION:-9.0.3}` to `elasticsearch:${ELASTIC_VERSION:-9.0.3}` for simplicity, if desired.

5. **Configure Elasticsearch License**
   In the Elasticsearch configuration file (e.g., `elasticsearch.yml`), set the license type to `basic` by updating the following:

   ```yaml
   xpack.license.self_generated.type: basic
   ```

6. **Pull Required Docker Images**
   Download the necessary Docker images for Elasticsearch, Kibana, and Logstash (if needed).

   ```sh
   docker pull elasticsearch:9.0.3
   docker pull kibana:9.0.3
   ```

7. **Run Setup Commands**
   Execute the following commands to set up the ELK Stack. Be cautious, as some commands may delete existing data.

   ```sh
   docker compose --profile=setup down -v # WARNING: This removes all previous Elasticsearch data
   docker compose --profile=setup build
   docker compose up tls # Note the certificate fingerprint from the logs
   docker compose up setup
   ```

8. **Start the ELK Stack**
   Launch the ELK Stack in detached mode.

   ```sh
   docker compose up -d
   ```

9. **Access Kibana and Elasticsearch**
   - Access Kibana at `http://localhost:5601`.
   - Access Elasticsearch at `https://localhost:9200`.

10. **Create API Keys in Kibana**
    - Navigate to `http://localhost:5601/app/management/security/api_keys` in Kibana.
    - Create an API key for the data pipeline with **full access**.
    - Create an API key for the geocoding engine with **read-only access**.
    - Save the encoded API keys for use in the next step.

## Notes

- Ensure the certificate fingerprint is correctly copied from the logs in step 7.
- If Logstash is not needed, skip pulling its image and remove related configurations.
- Always verify the `.env` file settings to match your environment.
- The `docker compose down -v` command deletes all existing Elasticsearch data, so use it cautiously.
