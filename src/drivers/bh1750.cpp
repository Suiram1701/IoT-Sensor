#include <string>
#include <vector>
#include <esp_log.h>
#include <Wire.h>
#include <BH1750.h>
#include "../sensor_registry.h"
#include "../prometheus.h"
#include "config.h"

#ifdef USE_BH1750
#ifndef BH1750_I2C_ADDRESS
#define BH1750_I2C_ADDRESS 0x23
#endif

using namespace std;

static const char* TAG = "bh1750";

BH1750 Gy302;

vector<metric_t> light_metric(const char* name) {
    metric_t light = { .value = to_string(Gy302.readLightLevel()) };
    return { light };
}

esp_err_t bh1750_init() {
    if (!Wire.begin()) {
        ESP_LOGE(TAG, "Failed to initialize I2C!");
        return ESP_FAIL;
    }

    if (!Gy302.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_I2C_ADDRESS, &Wire)) {
        ESP_LOGE(TAG, "Failed to initialize BH1750!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sensor initialized on %#010x", BH1750_I2C_ADDRESS);

    metric_metadata_t lightMeta = {
        .name = "esp32_sensor_bh1750_brightness",
        .type = "gauge",
        .help = "The read ambient brightness in lux.",
        .metric_getter = &light_metric
    };
    prometheus_register_metric(lightMeta);
    return ESP_OK;
}

REGISTER_SENSOR("bh1750", bh1750_init)
#endif