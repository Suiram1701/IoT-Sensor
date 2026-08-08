#include "ens160.h"

#include <string>
#include <vector>
#include <Wire.h>
#include <esp_log.h>
#include <ScioSense_ENS16x.h>
#include "../sensor_registry.h"
#include "../prometheus.h"
#include "config.h"

#ifdef USE_ENS160
#ifndef ENS160_I2C_ADDRESS
#error "Definition of ENS160_I2C_ADDRESS is required!"
#endif

using namespace std;

static const char* TAG = "ens160";

static ENS160 Ens160;
static bool ReadingAllowed;

// Fetches data for other metric of the ens160 and will be invoked first because its registered before the other metrics
static vector<metric_t> status_metric(const char* name) {
    Result result = Ens160.update();
    if (result != RESULT_OK) {
        ESP_LOGE(TAG, "Updating ens160 data failed with error code %i", result);
        ReadingAllowed = false;
    }
    else {
        ReadingAllowed = true;
    }
    
    metric_t status = { .value = to_string(Ens160.getDeviceStatus()) };
    return { status };
}

static vector<metric_t> quality_metric(const char* name) {
    if (!ReadingAllowed) {
        return { };
    }

    metric_t quality = { .value = to_string(Ens160.getAirQualityIndex_UBA()) };
    return { quality };
}

static vector<metric_t> eco2_metric(const char* name) {
    if (!ReadingAllowed) {
        return { };
    }

    metric_t eco2 = { .value = to_string(Ens160.getEco2()) };
    return { eco2 };
}

static vector<metric_t> tvoc_metric(const char* name) {
    if (!ReadingAllowed) {
        return { };
    }

    metric_t tvoc = { .value = to_string(Ens160.getTvoc()) };
    return { tvoc };
}

static vector<metric_t> gpr_metric(const char* name) {
    if (!ReadingAllowed) {
        return { };
    }

    metric_t gpr0 = {
        .labels = {{ "gpr", "0" }},
        .value  = to_string(Ens160.getRs0())
    };
    metric_t gpr1 = {
        .labels = {{ "gpr", "1" }},
        .value  = to_string(Ens160.getRs1())
    };
    metric_t gpr2 = {
        .labels = {{ "gpr", "2" }},
        .value  = to_string(Ens160.getRs2())
    };
    metric_t gpr3 = {
        .labels = {{ "gpr", "3" }},
        .value  = to_string(Ens160.getRs3())
    };
    return { gpr0, gpr1, gpr2, gpr3 };
}

void ens160_register_metrics() {
    metric_metadata_t statusMeta = {
        .name = "esp32_sensor_ens160_status",
        .type = "gauge",
        .help = "Information about the current state of the module.",
        .metric_getter = &status_metric
    };
    prometheus_register_metric(statusMeta);

    metric_metadata_t qualityMeta = {
        .name = "esp32_sensor_ens160_quality_uba",
        .type = "gauge",
        .help = "The UBA Air Quality Index (by the German Federal Environmental Agency) which goes from 1 (good) to 5 (bad).",
        .metric_getter = &quality_metric
    };
    prometheus_register_metric(qualityMeta);

    metric_metadata_t eco2Meta = {
        .name = "esp32_sensor_ens160_eco2",
        .type = "gauge",
        .help = "The CO2 concentration in ppm (parts per million).",
        .metric_getter = &eco2_metric
    };
    prometheus_register_metric(eco2Meta);
    
    metric_metadata_t tvocMeta = {
        .name = "esp32_sensor_ens160_tvoc",
        .type = "gauge",
        .help = "The TVOC (total volatile organic compounds) concentration in ppb (parts per billion).",
        .metric_getter = &tvoc_metric
    };
    prometheus_register_metric(tvocMeta);

    metric_metadata_t gprMeta = {
        .name = "esp32_sensor_ens160_gpr",
        .type = "gauge",
        .help = "General information about the sensor.",
        .metric_getter = &gpr_metric
    };
    prometheus_register_metric(gprMeta);
}

esp_err_t ens160_init() {
    if (!Wire.begin()) {
        ESP_LOGE(TAG, "Failed to initialize I2C!");
        return ESP_FAIL;
    }

    Ens160.begin(&Wire, ENS160_I2C_ADDRESS);

    while (!Ens160.init()) {
        ESP_LOGI(TAG, "Waiting for initialization...");
        delay(1000);
    }

    uint8_t* ver = Ens160.getFirmwareVersion();
    ESP_LOGI(TAG, "Initialized on %#010x. Firmware ver.: %i.%i.%i", ENS160_I2C_ADDRESS, ver[0], ver[1], ver[2]);

    Ens160.startStandardMeasure();
    ens160_register_metrics();
    ESP_LOGI(TAG, "ENS160 specific metrics registered");
    return ESP_OK;
}

REGISTER_SENSOR("ens160", ens160_init)
#endif