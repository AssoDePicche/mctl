#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include <string.h>

#define MISO_PIN 10
#define MOSI_PIN 23
#define SCLK_PIN 18
#define CS_PIN 5

const char *TAG = "GATEWAY"; 

static spi_device_handle_t spiDevice;

void AccessPointInit(void);

void AccessPointEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

void SpiInit(void);

void app_main(void) {
  esp_err_t nvsInit = nvs_flash_init();

  if (ESP_ERR_NVS_NO_FREE_PAGES == nvsInit || ESP_ERR_NVS_NEW_VERSION_FOUND == nvsInit) {
      ESP_ERROR_CHECK(nvs_flash_erase());

      ESP_ERROR_CHECK(nvs_flash_init());
  }

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &AccessPointEvent, NULL));

  AccessPointInit();

  SpiInit();
}

void AccessPointInit(void) {
    wifi_config_t  configuration =  (wifi_config_t){
        .ap  =  {
            .authmode = WIFI_AUTH_OPEN,
            .channel = 6,
            .max_connection = 1,
        },
    };

    strlcpy((char *)configuration.ap.ssid, TAG, sizeof(configuration.ap.ssid));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &configuration));

    ESP_ERROR_CHECK(esp_wifi_start());

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

void SpiInit(void) {
    spi_bus_config_t busConfiguration = (spi_bus_config_t){
        .miso_io_num = MISO_PIN,
        .mosi_io_num = MOSI_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    spi_device_interface_config_t connectionConfiguration = (spi_device_interface_config_t){
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = CS_PIN,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST,  &busConfiguration, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &connectionConfiguration, &spiDevice));

    ESP_LOGI(TAG, "SPI MASTER INIT FINISHED");
}
