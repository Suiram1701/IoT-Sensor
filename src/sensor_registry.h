#ifndef _REGISTRY_H
#define _REGISTRY_H

#include <vector>
#include <esp_err.h>

using namespace std;

typedef esp_err_t (*init_fn)(void);

typedef struct {
    const char* name;
    const init_fn init;
} sensor_driver_t;

class SensorRegistry {
public:
    static esp_err_t registerModule(sensor_driver_t* driver);
    static uint8_t loadAllModules();
private:
    static vector<sensor_driver_t*>& modules();
};

#define REGISTER_SENSOR(sensor_name, fn)                                   \
    static sensor_driver_t instance = { .name = sensor_name, .init = fn }; \
    static esp_err_t instance_registered __attribute__((used)) =           \
        (SensorRegistry::registerModule(&instance));

#endif