#include "drivers/io.c"

#define SWITCH_IO CONFIG_SWITCH_IO

void
setSwitch (int val)
{
  printf("Setting switch: %d\n", val);
  cJSON *number = cJSON_CreateNumber(val);
  cJSON_ReplaceItemInObjectCaseSensitive(state,"light_level",number);
  // send_state();
  gpio_set_level(SWITCH_IO, val);
}

static void
switch_task(void *pvParameter)
{
  xTaskCreate(&io_task, "io_task", 5000, NULL, 5, NULL);
  while (1) {
    //outgoing messages to other services

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
