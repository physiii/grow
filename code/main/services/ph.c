#include "drivers/adc.c"

char ph_service_message[2000];
char ph_service_message_in[2000];
bool ph_service_message_ready = false;

float ph_bias = 1801; // 4973
float PH_SLOPE = 89; // 256
int COUNT_MOD = 10;
float PH_CAL = 7;
float avg_value = 0;

void calibrate_ph() {
  ph_bias = PH_CAL * PH_SLOPE + avg_value;
  printf("[handle_settings] calibrating ph...%f\n", ph_bias);
}

static void
ph_task(void *pvParameter)
{
  float temp = 0;
  xTaskCreate(&adc_task, "adc_service_task", 5000, NULL, 5, NULL);

  uint32_t previous_ph_reading = 0;
  float sum_value = 0;
  int cnt = 0;
  while (1) {
    //outgoing messages to other services
    if (ph_reading!=previous_ph_reading) {
        float ph_value = (ph_bias - ph_reading)/PH_SLOPE;
        cJSON *number = cJSON_CreateNumber(ph_value);
        cJSON_ReplaceItemInObjectCaseSensitive(state,"ph",number);
        // send_state();
        previous_ph_reading = ph_reading;
        sum_value+=ph_reading;
        printf("Reading: %u\tValue: %f\n",ph_reading,ph_value);
        cnt++;
    }

    if ((cnt % COUNT_MOD)==0) {
      avg_value = sum_value / COUNT_MOD;
      printf("! --- Average Reading: %f --- !\n",avg_value);
      sum_value = 0;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
