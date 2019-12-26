bool raising_ec = false;
bool lowering_ec = false;

float EC_WINDOW = 0.01;
float EC_SET_POINT = 0.1;

int EC_ADJUST_DELAY = 60 * 1000;
bool ec_timer_expired = true;
int ec_count = 0;
float previous_ec_value = 0;

struct ec_controller
{
  int supply_io;
  int adc_io;
  int nutrient_pump_io;
  int water_pump_io;
  float value;
  float set_value;
  float min_value;
  float max_value;
};

struct ec_controller ec;

void
start_ec_timer(bool val)
{
  if (val) {
    ec_timer_expired = false;
    ec_count = 0;
  } else {
    ec_timer_expired = true;
  }
}

static void
ec_timer(void *pvParameter)
{
  while (1) {
    if (ec_count >= EC_ADJUST_DELAY && !ec_timer_expired) {
      ec_timer_expired = true;
    } else ec_count++;
    vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
  }
}


void
lower_ec ()
{
  if (!ec_timer_expired) return;
  start_ec_timer(true);
  raising_ec = false;
  lowering_ec = true;
  gpio_set_level(CONFIG_WATER_PUMP_IO, true);
  ESP_LOGI(TAG, "Lowering EC to: %f", EC_SET_POINT);
}

void
raise_ec ()
{
  if (!ec_timer_expired) return;
  start_ec_timer(true);
  raising_ec = false;
  lowering_ec = true;
  gpio_set_level(CONFIG_NUTRIENT_PUMP_IO, true);
  ESP_LOGI(TAG, "Raising EC to: %f", EC_SET_POINT);
}

void
stop_ec ()
{
  raising_ec = false;
  lowering_ec = false;
  gpio_set_level(CONFIG_NUTRIENT_PUMP_IO, false);
  gpio_set_level(CONFIG_WATER_PUMP_IO, false);
  ESP_LOGI(TAG, "Set Point Reached: %f", EC_SET_POINT);
}

void
check_ec_state()
{
  if (ec.value < ec.min_value) {
    raise_ec();
  }
  if (ec.value  < ec.max_value) {
    lower_ec();
  }
  if (lowering_ec && ec.value < EC_SET_POINT) {
    stop_ec();
  }
  if (raising_ec && ec.value > EC_SET_POINT) {
    stop_ec();
  }

  if (ec.value!=previous_ec_value) {
      cJSON *number = cJSON_CreateNumber(ec.value);
      cJSON_ReplaceItemInObjectCaseSensitive(state,"ec",number);
  }
  previous_ec_value = ec.value;
}

static void
ec_task(void *pvParameter)
{
  ec.adc_io = CONFIG_EC_ADC_IO;
  ec.supply_io = CONFIG_EC_SUPPLY_IO;
  ec.set_value = EC_SET_POINT;
  ec.min_value = ec.set_value - EC_WINDOW / 2;
  ec.max_value = ec.set_value + EC_WINDOW / 2;
  ec.value = 0;
  xTaskCreate(ec_timer, "ec_timer", 2048, NULL, 10, NULL);
  while (1) {
    check_ec_state();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
