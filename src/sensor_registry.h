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

class sensor_registry {
public:
    static vector<sensor_driver_t*>& sensors() {
        static vector<sensor_driver_t*> instance;
        return instance;
    }
};

#define REGISTER_SENSOR(sensor_name, fn)                                   \
    static sensor_driver_t instance = { .name = sensor_name, .init = fn }; \
    static bool instance_registered __attribute__((used)) =         \
        (sensor_registry::sensors().push_back(&instance), true);

#endif