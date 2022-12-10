idf_component_register(SRCS "main.c" "camera_conf.c" "wifi_ap.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp32-camera esp_http_server esp_wifi spiffs)
