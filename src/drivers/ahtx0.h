#ifndef _AHTX0_H
#define _AHTX0_H

#include <esp_err.h>

void ahtx0_register_metrics();
esp_err_t ahtx0_init();

#endif