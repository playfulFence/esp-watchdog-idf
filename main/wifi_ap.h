#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


void wifi_event_handler(void*, esp_event_base_t, int32_t, void*);

void wifi_init_softap(void);

httpd_handle_t start_webserver(void);

esp_err_t photos_handler(httpd_req_t* );
