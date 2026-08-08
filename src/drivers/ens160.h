#ifndef _ENS160_H
#define _ENS160_H

#include <esp_err.h>

void ens160_register_metrics();
esp_err_t ens160_init();

#endif