#include <vector>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_https_server.h>
#include "prometheus.h"
#include "config.h"
#include "pins.h"
#include "secrets.h"

using namespace std;

static const char* TAG = "iot_sensor";
const char* Version    = "v0.1-dev";

vector<metric_t> info_metric(const char* name) {
    metric_t info {
        .labels = {{ "device", WiFi.getHostname() }, { "version", Version }},
        .value = "1"
    };
    return { info };
}

vector<metric_t> rssi_metric(const char* name) {
    metric_t rssi { .value = to_string(WiFi.RSSI()) };
    return { rssi };
}

void register_metrics() {
    metric_metadata_t infoMeta = {
        .name = "esp32_sensor_info",
        .type = "counter",
        .help = "Generall information about the sensor.",
        .metric_getter = &info_metric
    };
    prometheus_register_metric(infoMeta);

    metric_metadata_t rssiMeta = {
        .name = "esp32_sensor_wifi_rssi",
        .type = "gauge",
        .help = "The RSSI (signal strength) of the connected network.",
        .metric_getter = &rssi_metric
    };
    prometheus_register_metric(rssiMeta);
}

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
    httpd_stop(server);
    while (true) {
        digitalWrite(STATUS_LED, HIGH);
        delay(100);
        digitalWrite(STATUS_LED, LOW);
        delay(100);
    }
}

void WiFiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected. Reason: %s", info.wifi_sta_disconnected.reason);
    }
}

void setup() {
    Serial.begin(115200);
    ESP_LOGI(TAG, "IoT Sensor %s - %s", WiFi.getHostname(), Version);
    pinMode(STATUS_LED, OUTPUT);  

    ESP_LOGI(TAG,"Try connecting to %s...", ssid);
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
    ESP_LOGI(TAG, "Successfully connected to %s!", WiFi.BSSIDstr().c_str());

    register_metrics();

    esp_err_t start_code = start_mtls_server();
    if (start_code == ESP_OK) {
        ESP_LOGI(TAG, "Started listening on %s:9100", WiFi.localIP().toString().c_str());
    }
    else {
        ESP_LOGE(TAG, "An error occurred while starting HTTPS server: %s (%i)", esp_err_to_name(start_code), start_code);
        halt_system();
    }
}

void loop() {
    ESP_LOGD(TAG, "Running, WiFi RSSI: %i", WiFi.RSSI());
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