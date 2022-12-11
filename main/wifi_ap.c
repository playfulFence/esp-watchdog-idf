#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_ap.h"

/* The examples use WiFi configuration that you can set via project configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define WIFI_SSID      "esp32-cam"
#define WIFI_PASS      "esptesty"
#define WIFI_CHANNEL   1
#define MAX_STA_CONN   4

#define TAG "wifi_softAP"

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t * p_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);

    esp_netif_ip_info_t if_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif, &if_info));
    ESP_LOGI(TAG, "ESP32 IP:" IPSTR, IP2STR(&if_info.ip));
}


typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index) {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
        return 0;
    }
    j->len += len;
    return len;
}


esp_err_t photos_handler(httpd_req_t *req){
    esp_err_t res = ESP_OK;

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    // char ts[32];
    // camera_fb_t* frame = esp_camera_fb_get();
    // snprintf(ts, 32, "%ld.%06ld", frame->timestamp.tv_sec, frame->timestamp.tv_usec);
    // httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);

    FILE* fp = fopen("/spiffs/image1.jpg", "rb");

    fseek(fp, 0, SEEK_END);

    int size = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    char* buffer = malloc(sizeof(char) * size);
    if(!buffer)
    {
        ESP_LOGE(TAG, "Couldn't allocate memory for jpeg transfer");
        exit(1);
    }

    fread(buffer, size, sizeof(unsigned char), fp);
    fclose(fp);

    //if (frame->format == PIXFORMAT_JPEG) {
    res = httpd_resp_send(req, (const char *)buffer, size);
    // } else {
    //     jpg_chunking_t jchunk = {req, 0};
    //     res = frame2jpg_cb(frame, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
    //     httpd_resp_send_chunk(req, NULL, 0);
    // }
    // ESP_LOGI(TAG, "Parsed snapshot, clear frame");
    //esp_camera_fb_return(frame);

    return res;
}

httpd_uri_t photos_uri = {
    .uri = "/photos",
    .method = HTTP_GET,
    .handler = photos_handler
};

httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Start the httpd server
  ESP_LOGI("TEST", "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK)
  {
    // Set URI handlers
    ESP_LOGI("TEST", "Registering URI handlers");
    if(server == NULL){
        ESP_LOGE(TAG, "Server is NULL value");
    }
    httpd_register_uri_handler(server, &photos_uri);
    return server;
  }

  ESP_LOGI("TEST", "Error starting server!");
  return NULL;
}

