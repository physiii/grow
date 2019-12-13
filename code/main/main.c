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

#include "services/store.c"
#include "services/websocket.c"
#include "services/station.c"
#include "services/temperature.c"
#include "services/ph.c"

void app_main(void)
{
  printf("Grow Controller - Version 0.1\n");

  snprintf(state_str,sizeof(state_str),""
  "{\"light_level\":51, \"uptime\":0, \"on\":false, \"ph\":2.1, \"atm_temp\":75, \"humidity\":90, \"water_temp\":75, \"ec\":10, \"pco2\":4.1}");

  state = cJSON_Parse(state_str);

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  websocket_main();
  xTaskCreate(&ph_task, "ph_service_task", 5000, NULL, 5, NULL);
  xTaskCreate(&temperature_task, "temperature_service_task", 5000, NULL, 5, NULL);
  station_main();

  int uptime = 0;
  while (1) {
    cJSON *number = cJSON_CreateNumber(uptime);
    cJSON_ReplaceItemInObjectCaseSensitive(state,"uptime",number);
    send_state();
    uptime++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
