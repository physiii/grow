float ph_bias = 4695; // 4973
float PH_SLOPE = 450; // 256
int COUNT_MOD = 10;
float PH_CAL = 7;
float PH_SET_POINT = 5.8;
float PH_WINDOW = 0.5;
int PH_ADJUST_DELAY = 60 * 1000;
float avg_value = 0;
bool ph_timer_expired = true;
int ph_count = 0;
int cnt = 0;
bool raising_ph = false;
bool lowering_ph = false;
float sum_value = 0;

struct ph_controller
{
  int base_pump_io;
  int acid_pump_io;
  int reading;
  int prev_reading;
  float value;
  float set_value;
  float min_value;
  float max_value;
};

struct ph_controller ph;

#include "drivers/adc.c"

void
start_ph_timer(bool val)
{
  if (val) {
    ph_timer_expired = false;
    ph_count = 0;
  } else {
    ph_timer_expired = true;
  }
}

static void
ph_timer(void *pvParameter)
{
  while (1) {
    if (ph_count >= PH_ADJUST_DELAY && !ph_timer_expired) {
      ph_timer_expired = true;
    } else ph_count++;
    vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
  }
}

void
lower_ph ()
{
  if (!ph_timer_expired) return;
  start_ph_timer(true);
  raising_ph = false;
  lowering_ph = true;
  gpio_set_level(CONFIG_ACID_PUMP_IO, true);
  ESP_LOGI(TAG, "Lowering PH to: %f", ph.set_value);
}

void
raise_ph ()
{
  if (!ph_timer_expired) return;
  start_ph_timer(true);
  raising_ph = false;
  lowering_ph = true;
  gpio_set_level(CONFIG_BASE_PUMP_IO, true);
  ESP_LOGI(TAG, "Raising PH to: %f", ph.set_value);
}

void
stop_ph ()
{
  raising_ph = false;
  lowering_ph = false;
  gpio_set_level(CONFIG_NUTRIENT_PUMP_IO, false);
  gpio_set_level(CONFIG_WATER_PUMP_IO, false);
  ESP_LOGI(TAG, "Set Point Reached: %f", ph.set_value);
}

void
calibrate_ph()
{
  ph_bias = PH_CAL * PH_SLOPE + avg_value;
  printf("[handle_settings] calibrating ph...%f\n", ph_bias);
}

void
check_ph_state()
{
  if (ph.value < ph.min_value) {
    raise_ph();
  }
  if (ph.value  > ph.max_value) {
    lower_ph();
  }
  if (lowering_ph && ph.value < ph.set_value) {
    stop_ph();
  }
  if (raising_ph && ph.value > ph.set_value) {
    stop_ph();
  }

  if (ph.reading!=ph.prev_reading) {
      ph.value = (ph_bias - ph.reading)/PH_SLOPE;
      cJSON *number = cJSON_CreateNumber(ph.value);
      cJSON_ReplaceItemInObjectCaseSensitive(state,"ph",number);
      printf("! --- Reading: %d | pH: %f --- !\n", ph.reading, ph.value);
      sum_value+=ph.reading;
  }  cnt++;
  cnt++;

  if ((cnt % COUNT_MOD)==0) {
    avg_value = sum_value / COUNT_MOD;
    // printf("! --- Average Reading: %f --- !\n",avg_value);
    sum_value = 0;
  }
  ph.prev_reading = ph.reading;
}

static void
ph_task(void *pvParameter)
{
  ph.set_value = PH_SET_POINT;
  ph.min_value = ph.set_value - PH_WINDOW / 2;
  ph.max_value = ph.set_value + PH_WINDOW / 2;
  ph.value = 0;
  xTaskCreate(ph_timer, "ph_timer", 2048, NULL, 10, NULL);
  xTaskCreate(&adc_task, "adc_service_task", 5000, NULL, 5, NULL);

  while (1) {
    //outgoing messages to other services
    check_ph_state();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
