#include "drivers/ds18b20.c"

char temperature_service_message[2000];
char temperature_service_message_in[2000];
bool temperature_service_message_ready = false;

static void
temperature_task(void *pvParameter)
{
  int temp = 0;
  xTaskCreate(&ds18b20_main, "ds18b20_main", 5000, NULL, 5, NULL);
  float previous_water_temp = 0;
  while (1) {
    //outgoing messages to other services
    if (water_temp!=previous_water_temp) {
        cJSON *number = cJSON_CreateNumber(water_temp);
        cJSON_ReplaceItemInObjectCaseSensitive(state,"water_temp",number);
        send_state();
        previous_water_temp = water_temp;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
