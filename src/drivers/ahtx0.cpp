#include "ahtx0.h"

#include <string>
#include <vector>
#include <esp_log.h>
#include <Adafruit_AHTX0.h>
#include "../sensor_registry.h"
#include "../prometheus.h"
#include "config.h"

#ifdef USE_AHTX0
#ifndef AHTX0_I2C_ADDRESS
#error "Definition of AHTX0_I2C_ADDRESS is required!" 
#endif

using namespace std;

static const char* TAG = "ahtx0";

static Adafruit_AHTX0  Ahtx0;
static Adafruit_Sensor* Temperature;
static Adafruit_Sensor* Humidity;

static vector<metric_t> status_metric(const char* name) {
    metric_t temp = { .value = to_string(Ahtx0.getStatus()) };
    return { temp };
}

static vector<metric_t> temperature_metric(const char* name) {
    sensors_event_t event;
    Temperature->getEvent(&event);

    metric_t temp = { .value = to_string(event.temperature) };
    return { temp };
}

static vector<metric_t> humidity_metric(const char* name) {
    sensors_event_t event;
    Humidity->getEvent(&event);

    metric_t humid = { .value = to_string(event.relative_humidity) };
    return { humid };
}

void ahtx0_register_metrics() {
    metric_metadata_t statusMeta = {
        .name = "esp32_sensor_ahtx0_status",
        .type = "gauge",
        .help = "Information about the current state of the module.",
        .metric_getter = &status_metric
    };
    prometheus_register_metric(statusMeta);

    metric_metadata_t tempMeta = {
        .name = "esp32_sensor_ahtx0_temperature",
        .type = "gauge",
        .help = "The last read temperature from the module in degree celcius.",
        .metric_getter = &temperature_metric
    };
    prometheus_register_metric(tempMeta);

    metric_metadata_t humidMeta = {
        .name = "esp32_sensor_ahtx0_relative_humidity",
        .type = "gauge",
        .help = "The last read humidity (relative) in %",
        .metric_getter = &humidity_metric
    };
    prometheus_register_metric(humidMeta);
}

esp_err_t ahtx0_init() {
    if (!Ahtx0.begin(&Wire, 0, AHTX0_I2C_ADDRESS)) {
        ESP_LOGE(TAG, "Failed to initialize ahtx0!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sensor initialized on %#010x", AHTX0_I2C_ADDRESS);
    Temperature = Ahtx0.getTemperatureSensor();
    Temperature->printSensorDetails();
    Humidity = Ahtx0.getHumiditySensor();
    Humidity->printSensorDetails();

    ahtx0_register_metrics();
    ESP_LOGI(TAG, "AHTX0 specific metrics registered");
    return ESP_OK;
}

REGISTER_SENSOR("ahtx0", ahtx0_init)
#endif