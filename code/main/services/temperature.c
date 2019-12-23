#include "drivers/ds18b20.c"

char temperature_service_message[2000];
char temperature_service_message_in[2000];
bool temperature_service_message_ready = false;
float atm_temp = 0;
float humidity = 0;
float previous_atm_temp = 0;
float previous_humidity = 0;

#include "drivers/i2c.c"

static void
temperature_task(void *pvParameter)
{
  int temp = 0;
  xTaskCreate(&ds18b20_main, "ds18b20_main", 5000, NULL, 5, NULL);
  i2c_main();
  float previous_water_temp = 0;
  while (1) {
    //outgoing messages to other services

    if (atm_temp!=previous_atm_temp) {
        cJSON *number = cJSON_CreateNumber(atm_temp);
        cJSON_ReplaceItemInObjectCaseSensitive(state,"atm_temp",number);
        previous_atm_temp = atm_temp;
    }

    if (humidity!=previous_humidity) {
        cJSON *number = cJSON_CreateNumber(humidity);
        cJSON_ReplaceItemInObjectCaseSensitive(state,"humidity",number);
        previous_humidity = humidity;
    }

    if (water_temp!=previous_water_temp) {
        cJSON *number = cJSON_CreateNumber(water_temp);
        cJSON_ReplaceItemInObjectCaseSensitive(state,"water_temp",number);
        previous_water_temp = water_temp;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
