#include "analog.h"

#include <string>
#include <vector>
#include <Arduino.h>
#include <esp_log.h>
#include "../sensor_registry.h"
#include "../prometheus.h"
#include "config.h"

#ifdef USE_ANALOG
#ifndef ANALOG_READ_GPIOS
#error "Definition of ANALOG_READ_GPIOS is required!"
#endif

using namespace std;

static const char* TAG = "analog_metric";

static vector<metric_t> analog_metric(const char* name) {
    vector<metric_t> metrics;
    for (uint8_t pin : {ANALOG_READ_GPIOS}) {
        metric_t analog = {
            .labels = {{ "gpio", to_string(pin) }},
            .value  = to_string(analogRead(pin))
        };
        metrics.push_back(analog);
    }

    return metrics;
}

esp_err_t analog_init() {
    // Setup reading area 0 - 4095
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    metric_metadata_t analogMeta = {
        .name = "esp32_sensor_analog_read",
        .type = "gauge",
        .help = "Analog readings from ADC GPIO pins. Goes from 0 (0 V) to 4095 (3.3 V) but not completly linear.",
        .metric_getter = &analog_metric
    };
    prometheus_register_metric(analogMeta);

    bool isFirst = true;
    string pinsStr;
    for (uint8_t pin : {ANALOG_READ_GPIOS}) {
        if (!isFirst) {
            pinsStr += ", ";
        }
        pinsStr += to_string(pin);
        isFirst = false;
    }

    ESP_LOGI(TAG, "Initialized analog metric reader for pins %s", pinsStr.c_str());
    return ESP_OK;
}

REGISTER_SENSOR("analog", analog_init)
#endif