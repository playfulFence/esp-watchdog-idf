/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "camera_conf.h"
#include "wifi_ap.h"

#include <esp_log.h>

#define GPIO_PIR_PIN GPIO_NUM_12

static QueueHandle_t gpio_evt_queue = NULL;

void app_main(void)
{

    if( ESP_OK != init_camera()) 
    {
        ESP_LOGE(TAG, "Couldn't initialise camera");
        return;
    }


    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

     

     //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");

    wifi_init_softap();

    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << GPIO_PIR_PIN);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    httpd_handle_t server = NULL;

    server = start_webserver();


    unsigned short photoCnt = 0;
    while(1) {
        
        ESP_LOGI(TAG, "Awake again!");
        /* Active waiting for log. 1 (movement detected) on data pin */
        while (gpio_get_level(GPIO_PIR_PIN) == 0) {}

        camera_fb_t *pic = esp_camera_fb_get();

        // use pic->buf to access the image
        ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes\nCreating new file", pic->len);
        photoCnt++;
        char path[20];
        sprintf(path, "/spiffs/image%hu.jpg", photoCnt);

        FILE* fp = fopen(path, "w"); 
        if (fp == NULL) 
        {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }
        fwrite(pic->buf, pic->len, 1, fp);
        fclose(fp);
        ESP_LOGI(TAG, "File %s written", path);
        

        esp_camera_fb_return(pic);
        ESP_LOGI(TAG, "Taking 5 seconds sleep");
        vTaskDelay(5000 / portTICK_RATE_MS);
        
    }
}
