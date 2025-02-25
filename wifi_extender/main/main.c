#include <stdio.h>

// tmp NVS
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"

#include "esp_log.h"

#include "net_tools.c"

void init_nvs()
{
    //  Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

int app_main(int argc, char **argv)
{
    // vTaskDelay(2000);
    vTaskDelay(1000);

    init_nvs();

    esp_err_t crel_err = esp_event_loop_create_default();
    if(crel_err != ESP_OK)
    {
        printf("failed to create event loop");
    }

    printf("start wifi :\n");
    // esp_log_level_set("wifi", ESP_LOG_DEBUG);
    // esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_VERBOSE);
    esp_log_level_set("lwip", ESP_LOG_DEBUG);
    esp_log_level_set(0x00U, ESP_LOG_DEBUG);
    // esp_log_level_set(IP_DEBUG, ESP_LOG_VERBOSE);

    // Be a AP_STA with ip forwarding
    init_AP_STA();

    return 0;
}