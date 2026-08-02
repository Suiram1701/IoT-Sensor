#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_https_server.h>
#include "config.h"
#include "secrets.h"

#define STATUS_LED 2

const char* version  = "v0.1-dev";

std::string build_metric_header(const char* name, const char* type, const char* help) {
    return "# HELP " + std::string(name) + " " + std::string(help)
       + "\n# TYPE " + std::string(name) + " " + std::string(type) + "\n";
}

std::string build_metric(const char* name, std::initializer_list<std::pair<std::string_view, std::string_view>> labels, const char* value) {
    std::string metric = name;
    if (labels.size() > 0) {
        metric += '{';
        bool first = true;
        for (auto& [k, v] : labels) {
            if (!first && labels.size() > 1)
                metric += ',';
            metric += k;
            metric += "=\"";
            metric += v;
            metric += '"'; 
            first = false;
        }
        metric += '}';
    }

    return metric + " " + value + "\n";
}

// response += build_metric_header("esp32_sensor_", "gauge", "");
// response += build_metric("esp32_sensor_", "");

esp_err_t prometheus_endpoint_handler(httpd_req_t *req) {
    digitalWrite(STATUS_LED, HIGH);

    std::string response = build_metric_header("esp32_sensor_info", "counter", "Generall information about the sensor.");
    response += build_metric("esp32_sensor_info", {{ "device", WiFi.getHostname() }, { "version", version }}, "1");

    response += build_metric_header("esp32_sensor_wifi_rssi", "gauge", "The RSSI (signal strength) of the connected network.");
    response += build_metric("esp32_sensor_wifi_rssi", {}, std::to_string(WiFi.RSSI()).c_str());

    response += "# EOF";

    httpd_resp_set_type(req, "application/openmetrics-text; version=1.0.0; charset=utf-8");
    httpd_resp_send(req, response.c_str(), response.length());

    digitalWrite(STATUS_LED, LOW);
    return ESP_OK;
}

httpd_uri_t prometheus_endpoint = {
    .uri = "/metrics",
    .method = HTTP_GET,
    .handler = prometheus_endpoint_handler
};

static httpd_handle_t server = NULL;
esp_err_t start_mtls_server() {
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.port_insecure  = -1;
    conf.port_secure    = 9100;

    conf.servercert     = (const uint8_t*)server_crt;
    conf.servercert_len = sizeof(server_crt);
    conf.prvtkey_pem    = (const uint8_t*)server_key;
    conf.prvtkey_len    = sizeof(server_key);
    conf.cacert_pem     = (const uint8_t*)ca_crt;
    conf.cacert_len     = sizeof(ca_crt);
    
    conf.httpd.max_uri_handlers = 1;
    conf.httpd.max_open_sockets = 2;
    conf.httpd.lru_purge_enable = true;

    esp_err_t start_code = httpd_ssl_start(&server, &conf);
    if (start_code == ESP_OK) {
        httpd_register_uri_handler(server, &prometheus_endpoint);
    }
    return start_code;
}

void halt_system() {
    while (true) {
        digitalWrite(STATUS_LED, HIGH);
        delay(100);
        digitalWrite(STATUS_LED, LOW);
        delay(100);
    }
}

void WiFiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.printf("WiFi disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
  }
}

void setup() {
    pinMode(STATUS_LED, OUTPUT);
  
    Serial.begin(115200);
    Serial.print("IoT Sensor ");
    Serial.print(WiFi.getHostname());
    Serial.print("- Version ");
    Serial.println(version);

    Serial.print("Try connecting to ");
    Serial.println(ssid);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(WiFiEventHandler);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(STATUS_LED, HIGH);
        delay(50);
        digitalWrite(STATUS_LED, LOW);
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Successfully connected to !");
    Serial.println(WiFi.BSSIDstr());

    esp_err_t start_code = start_mtls_server();
    if (start_code == ESP_OK) {
        Serial.print("Started listening on ");
        Serial.print(WiFi.localIP());
        Serial.println(":9100");
    }
    else {
        Serial.print("An error occurred while starting HTTPS server: ");
        Serial.print(esp_err_to_name(start_code));
        Serial.print(" (");
        Serial.print(start_code);
        Serial.println(")");
        halt_system();
    }
}

void loop() {
    Serial.print("Running, WiFi RSSI: ");
    Serial.println(String(WiFi.RSSI()));
    delay(5000);
}

extern "C" void app_main() {
    initArduino();
    setup();
    while (true) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1));     // Time to run other tasks like WiFi
    }
}