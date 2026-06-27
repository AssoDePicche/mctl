#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

const char *TAG = "GATEWAY"; 

void AccessPointInit(void);

void AccessPointEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

void app_main(void) {
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID.&AccessPointEvent, NULL, NULL);

  AcessPointInit();
}

void AccessPointInit(void) {
    wifi_config_t  configuration =  (wifi_config_t){
        .ap  =  {
            .ssid  = TAG,
            .channel =  6,
            .max_connection = 1,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);

    esp_wifi_set_config(WIFI+IF_AP, &configuration);

    esp_wifi_start();

    ESP_LOGI(TAG, "ACCESS POINT INIT FINISHED");
}

void AccessPointEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
    if (WIFI_EVENT_AP_STACONNECTED == eventId) {
        ESP_LOGI(TAG, "DEVICE CONNECTED");
    }

    if (WIFI_EVENT_AP_STADISCONNECTED == eventId) {
        ESP_LOGI(TAG, "DEVICE DISCONNECTED");
    }
}
