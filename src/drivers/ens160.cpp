#include "ens160.h"

#include <esp_log.h>
#include "../sensor_registry.h"
#include "config.h"

static const char* TAG = "ens160";

esp_err_t ens160_init() {
    ESP_LOGI(TAG, "Initialized");
    return ESP_OK;
}

#ifdef USE_ENS160
REGISTER_SENSOR("ens160", ens160_init)
#endif