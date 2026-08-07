#ifndef _PROMETHEUS_H
#define _PROMETHEUS_H

#include <string>
#include <vector>
#include <esp_https_server.h>

using namespace std;

typedef struct {
    const vector<pair<string_view, string_view>> labels;
    const string value;
} metric_t;

typedef vector<metric_t> (*get_metric)(const char* name);

typedef struct {
    const char* name;
    const char* type;
    const char* help;
    const get_metric metric_getter;
} metric_metadata_t;

esp_err_t prometheus_register_metric(metric_metadata_t& metric);

esp_err_t prometheus_endpoint_handler(httpd_req_t *req);

const httpd_uri_t prometheus_endpoint = {
    .uri = "/metrics",
    .method = HTTP_GET,
    .handler = prometheus_endpoint_handler
};

#endif