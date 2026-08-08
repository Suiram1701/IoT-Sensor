#include "sensor_registry.h"

#include <vector>
#include <Arduino.h>
#include <esp_log.h>
#include "config.h" 

using namespace std;

static const char* TAG = "registry";

esp_err_t SensorRegistry::registerModule(sensor_driver_t* driver) {
    for (auto& module : modules()) {
        if (strcmp(driver->name, module->name) == 0) {
            ESP_LOGW(TAG, "Unable to register a second driver named \"%s\"", driver->name);
            return ESP_FAIL;
        }
    }

    modules().push_back(driver);
    return ESP_OK;
}

uint8_t SensorRegistry::loadAllModules() {
    uint8_t loaded;
    for (auto& entry : modules()) {
        digitalWrite(STATUS_LED, HIGH);
        esp_err_t result = entry->init();
        if (result == ESP_OK) {
            loaded++;
            ESP_LOGD(TAG, "Loaded sensor driver/module %s", entry->name);
        }
        else {
            ESP_LOGE(TAG, "An error occurred while loading driver/module (skipped) %s: %s (%i)", entry->name, esp_err_to_name(result), result);
        }
        digitalWrite(STATUS_LED, LOW);
    }

    ESP_LOGI(TAG, "Successfully loaded %i of %i modules", loaded, modules().size());
    return loaded;
}

vector<sensor_driver_t*>& SensorRegistry::modules() {
    static vector<sensor_driver_t*> instance;
    return instance;
}