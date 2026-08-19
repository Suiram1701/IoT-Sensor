#include <string>
#include <vector>
#include <esp_log.h>
#include <Adafruit_BME280.h>
#include "../sensor_registry.h"
#include "../prometheus.h"
#include "config.h"

#ifdef USE_BME280
#ifndef BME280_I2C_ADDRESS
#define BME280_I2C_ADDRESS 0x77
#endif

using namespace std;

static const char* TAG = "bme280";

static Adafruit_BME280 Bme280;

static vector<metric_t> temperature_metric(const char* name) {
    if (!Bme280.takeForcedMeasurement()) {     // Called first because this metric is registered first
        ESP_LOGW(TAG, "Failed to take measurement!");
    }

    metric_t temperature = { .value = to_string(Bme280.readTemperature()) };
    return { temperature };
}

static vector<metric_t> humidity_metric(const char* name) {
    metric_t humidity = { .value = to_string(Bme280.readHumidity()) };
    return { humidity };
}

static vector<metric_t> pressure_metric(const char* name) {
    metric_t pressure = { .value = to_string(Bme280.readPressure()) };
    return { pressure };
}

static vector<metric_t> altitude_metric(const char* name) {
    metric_t altitude = { .value = to_string(Bme280.readAltitude(SENSORS_PRESSURE_SEALEVELHPA)) };
    return { altitude };
}

void bme280_register_metrics() {
    metric_metadata_t tempMeta = {
        .name = "esp32_sensor_bme280_temperature",
        .type = "gauge",
        .help = "The read temperature from the device in degree celcius.",
        .metric_getter = &temperature_metric
    };
    prometheus_register_metric(tempMeta);

    metric_metadata_t humidMeta = {
        .name = "esp32_sensor_bme280_humidity",
        .type = "gauge",
        .help = "The read relative humidity in percent.",
        .metric_getter = &humidity_metric
    };
    prometheus_register_metric(humidMeta);

    metric_metadata_t pressureMeta = {
        .name = "esp32_sensor_bme280_pressure",
        .type = "gauge",
        .help = "The read pressure in pascal from the device.",
        .metric_getter = &pressure_metric
    };
    prometheus_register_metric(pressureMeta);

    metric_metadata_t altMeta = {
        .name = "esp32_sensor_bme280_altitude",
        .type = "gauge",
        .help = "The calculates altitude based on the pressure and the sealevel.",
        .metric_getter = &altitude_metric
    };
    prometheus_register_metric(altMeta);
}

esp_err_t bme280_init() {
    if (!Bme280.begin(BME280_I2C_ADDRESS, &Wire)) {
        ESP_LOGE(TAG, "Failed to initialize bme280!");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sensor initialized on %#010x", BME280_I2C_ADDRESS);
    Bme280.setSampling(
        Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X1,     // temperature
        Adafruit_BME280::SAMPLING_X1,     // pressure
        Adafruit_BME280::SAMPLING_X1,     // humidity
        Adafruit_BME280::FILTER_OFF);
    bme280_register_metrics();
    return ESP_OK;
}

REGISTER_SENSOR("bme280", bme280_init)
#endif