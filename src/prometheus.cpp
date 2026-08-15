#include "prometheus.h"

#include <string>
#include <vector>
#include <Arduino.h>
#include <esp_log.h>
#include <esp_https_server.h>
#include <lwip/sockets.h>
#include "config.h"

using namespace std;

static const char* TAG = "prometheus_exporter";

static vector<metric_metadata_t> RegisterdMetrics;

esp_err_t prometheus_register_metric(metric_metadata_t& metric) {
    for (auto& registerd : RegisterdMetrics) {
        if (strcmp(registerd.name, metric.name) == 0) {     // Already registerd
            ESP_LOGW(TAG, "Metric %s already registered!", metric.name);
            return ESP_FAIL;
        }
    }

    RegisterdMetrics.push_back(metric);
    return ESP_OK;
}

string build_metric_header(const char* name, const char* type, const char* help) {
    return "# HELP " + string(name) + " " + string(help)
       + "\n# TYPE " + string(name) + " " + string(type) + "\n";
}

string build_metric(const char* name, const vector<pair<const char*, string>> labels, const char* value) {
    string metric = name;
    if (labels.size() > 0) {
        metric += '{';
        bool first = true;
        for (auto& [k, v] : labels) {
            if (!first && labels.size() > 1)
                metric += ',';
            metric += k;
            metric += "=\"";
            metric += v;
            metric += '"'; 
            first = false;
        }
        metric += '}';
    }

    return metric + " " + value + "\n";
}

esp_err_t get_client_ip(httpd_req_t *req, struct sockaddr_in6 *addr)
{
    int s = httpd_req_to_sockfd(req);
    socklen_t addrlen = sizeof(*addr);
    if (lwip_getpeername(s, (struct sockaddr *)addr, &addrlen) != -1) {
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error getting peer's IP/port");
        return ESP_FAIL;
    }
}

esp_err_t prometheus_endpoint_handler(httpd_req_t *req) {
    digitalWrite(STATUS_LED, HIGH);

    sockaddr_in6 client_addr;
    get_client_ip(req, &client_addr);
    ESP_LOGI(TAG, "Serving prometheus reqeust from %s:%i", inet_ntoa(client_addr.sin6_addr.un.u32_addr[3]), client_addr.sin6_port);

    string response;
    for (int i = 0; i < RegisterdMetrics.size(); i++) {
        auto& metadata = RegisterdMetrics[i];
        response += build_metric_header(metadata.name, metadata.type, metadata.help);

        for (auto& metric : metadata.metric_getter(metadata.name)) {
            string metricStr = build_metric(metadata.name, metric.labels, metric.value.c_str());
            ESP_LOGD(TAG, "Read metric: %s", metricStr.c_str());
            response += metricStr;
        }
    }
    response += "# EOF";

    httpd_resp_set_type(req, "application/openmetrics-text; version=1.0.0; charset=utf-8");
    esp_err_t send_result = httpd_resp_send(req, response.c_str(), response.length());
    digitalWrite(STATUS_LED, LOW);

    if (send_result != ESP_OK) {
        ESP_LOGE(TAG, "An error occurred while serving request: %s (%i)", esp_err_to_name(send_result), send_result);
    }
    return send_result;
}