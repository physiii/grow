#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "cJSON.h"
#include <string.h>
#include "driver/adc.h"
#if CONFIG_IDF_TARGET_ESP32
#include "esp_adc_cal.h"
#endif

static const char *TAG = "Controller";

cJSON *state = NULL;
char state_str[2000];
bool connected_to_server = false;

void send_state(void);
#include "services/switch.c"
#include "services/store.c"
#include "services/websocket.c"
#include "services/station.c"
#include "services/temperature.c"
#include "services/ph.c"

void app_main(void)
{
  printf("Grow Controller - Version 0.1\n");

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  snprintf(state_str,sizeof(state_str),""
  "{\"id\":\"pod_1\", \"light_level\":51, \"uptime\":0, \"on\":false, \"ph\":2.1, \"atm_temp\":75, \"humidity\":90, \"water_temp\":75, \"ec\":10, \"pco2\":4.1}");
  state = cJSON_Parse(state_str);

  ESP_ERROR_CHECK(nvs_flash_init());
  strcpy(device_id,get_char("device_id"));
  if (strcmp(device_id,"")==0) {
    xTaskCreate(&websocket_utilities_task, "websocket_utilities_task", 10000, NULL, 5, NULL);
  } else {
    printf("pulled device_id from storage: %s\n", token);
  }

  strcpy(token,get_char("token"));
  if (strcmp(token,"")==0) {
    strcpy(token,device_id);
    printf("no token found, setting token as device id: %s\n", token);
  } else {
    printf("pulled token from storage: %s\n", token);
  }

  // xTaskCreate(&switch_task, "switch_task", 5000, NULL, 5, NULL);
  station_main();
  // xTaskCreate(&websocket_task, "websocket_task", 5000, NULL, 5, NULL);
  // websocket_main();
  xTaskCreate(&websocket_relay_task, "websocket_relay_task", 10000, NULL, 5, NULL);
  xTaskCreate(&temperature_task, "temperature_service_task", 5000, NULL, 5, NULL);
  xTaskCreate(&ph_task, "ph_service_task", 5000, NULL, 5, NULL);

  int uptime = 0;
  while (1) {
    uptime++;
    cJSON *number = cJSON_CreateNumber(uptime);
    cJSON_ReplaceItemInObjectCaseSensitive(state,"uptime",number);
    send_state();
    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
