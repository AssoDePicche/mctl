#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include <string.h>

#define DNS_PORT 53
#define MIN_DNS_QUERY_LEN 12

#define MISO_PIN 19
#define MOSI_PIN 23
#define SCLK_PIN 18
#define CS_PIN 5

#define KNOB_OFFSET 2

typedef struct {
    uint8_t led_power;
    uint8_t potentiometer_value;
} spi_packet_t;

const char *TAG = "GATEWAY"; 

static spi_device_handle_t spiDevice;

static int led_power = 50;

static int remote_potentiometer_value = 0;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");

extern const uint8_t index_html_end[] asm("_binary_index_html_end");

void dns_server_task(void *pvParameters);

esp_err_t root_get_handler(httpd_req_t *req);

esp_err_t captive_portal_error_handler(httpd_req_t *req, httpd_err_code_t error); 

esp_err_t led_api_handler(httpd_req_t *req);

static const httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };

static const httpd_uri_t led_api_uri = { .uri = "/api/led", .method = HTTP_GET, .handler = led_api_handler };

void AccessPointInit(void);

void AccessPointEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

void DnsServerInit(void);

void HttpServerInit(void);

void SpiInit(void);

void SpiSync(void);

void app_main(void) {
  esp_err_t nvsInit = nvs_flash_init();

  if (ESP_ERR_NVS_NO_FREE_PAGES == nvsInit || ESP_ERR_NVS_NEW_VERSION_FOUND == nvsInit) {
      ESP_ERROR_CHECK(nvs_flash_erase());

      ESP_ERROR_CHECK(nvs_flash_init());
  }

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_ap();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &AccessPointEvent, NULL));

  AccessPointInit();

  SpiInit();

  HttpServerInit();

  DnsServerInit();
}

void AccessPointInit(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

void DnsServerInit(void) {
    xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5, NULL);
}

void HttpServerInit(void) {
    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) != ESP_OK) {
        return;
    }
    
    httpd_register_uri_handler(server, &root_uri);

    httpd_register_uri_handler(server, &led_api_uri);

    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, captive_portal_error_handler);

    ESP_LOGI(TAG, "Server running.");
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

void SpiSync(void) {
   spi_packet_t tx_data = {
        .led_power = (uint8_t)led_power,
        .potentiometer_value = 0,
    };
    spi_packet_t rx_data = {0};

    spi_transaction_t t = {
        .length = sizeof(spi_packet_t) * 8, 
        .tx_buffer = &tx_data,
        .rx_buffer = &rx_data,
    };

    esp_err_t ret = spi_device_transmit(spiDevice, &t);

    if (ret == ESP_OK) {
        int diff = (int)rx_data.potentiometer_value - remote_potentiometer_value;

        if (diff > KNOB_OFFSET || diff < -KNOB_OFFSET) {
            led_power = rx_data.potentiometer_value; 
        }
    
        remote_potentiometer_value = rx_data.potentiometer_value; 
    } else {
        ESP_LOGE(TAG, "SPI transmission failed: %s", esp_err_to_name(ret));
    } 
}

void dns_server_task(void *pvParameters) {
    uint8_t rx_buffer[128];

    struct sockaddr_storage source_addr;

    socklen_t socklen = sizeof(source_addr);

    int listen_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create DNS socket");

        vTaskDelete(NULL);

        return;
    }

    struct sockaddr_in listen_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "DNS Socket unable to bind");

        close(listen_sock);

        vTaskDelete(NULL);

        return;
    }

    ESP_LOGI(TAG, "DNS Server listening on port %d...", DNS_PORT);

    while (1) {
        int len = recvfrom(listen_sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

        if (len > MIN_DNS_QUERY_LEN) { 
            rx_buffer[2] |= 0x80;

            rx_buffer[3] |= 0x80;
            
            // Payload: Name Offset (2B), Type A (2B), Class IN (2B), TTL (4B), Data Length 4 (2B), IP 192.168.4.1 (4B)
            uint8_t reply_addition[] = {
                0xc0, 0x0c,             
                0x00, 0x01,            
                0x00, 0x01,             
                0x00, 0x00, 0x00, 0x3c, 
                0x00, 0x04, 
                192, 168, 4, 1 
            };

            rx_buffer[6] = 0x00;

            rx_buffer[7] = 0x01;

            if (len + sizeof(reply_addition) <= sizeof(rx_buffer)) {
                memcpy(&rx_buffer[len], reply_addition, sizeof(reply_addition));

                sendto(listen_sock, rx_buffer, len + sizeof(reply_addition), 0, (struct sockaddr *)&source_addr, socklen);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

esp_err_t root_get_handler(httpd_req_t *req) {
    const size_t index_html_size = (index_html_end - index_html_start);

    httpd_resp_set_type(req, "text/html");
    
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);

    return ESP_OK;
}

esp_err_t captive_portal_error_handler(httpd_req_t *req, httpd_err_code_t error) {
    if (error == HTTPD_404_NOT_FOUND) {
        ESP_LOGI(TAG, "Redirecting 404 request to captive portal root");

        return root_get_handler(req);
    }

    return ESP_FAIL;
}

esp_err_t led_api_handler(httpd_req_t *req) {
    char buf[64];
    char action[16] = {0};
    
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "action", action, sizeof(action)) == ESP_OK) {
            if (strcmp(action, "increase") == 0 && led_power < 100) led_power += 10;

            if (strcmp(action, "decrease") == 0 && led_power > 0) led_power -= 10;

            ESP_LOGI(TAG, "Power changed to: %d%%", led_power);
        }
    }

    SpiSync();

    char json[64];

    snprintf(json, sizeof(json), "{\"power\": %d, \"potentiometer\": %d}", led_power, remote_potentiometer_value);

    httpd_resp_set_type(req, "application/json");

    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
