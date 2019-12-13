#include "drivers/adc.c"

char ph_service_message[2000];
char ph_service_message_in[2000];
bool ph_service_message_ready = false;

static void
ph_task(void *pvParameter)
{
  float temp = 0;
  xTaskCreate(&adc_task, "adc_service_task", 5000, NULL, 5, NULL);

  uint32_t previous_ph_reading = 0;
  while (1) {
    //outgoing messages to other services
    if (1) {
      if (ph_reading!=previous_ph_reading) {
          cJSON *number = cJSON_CreateNumber(ph_reading);
          cJSON_ReplaceItemInObjectCaseSensitive(state,"ph",number);
          send_state();
          previous_ph_reading = ph_reading;
      }
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
