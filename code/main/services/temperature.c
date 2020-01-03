char temperature_service_message[2000];
char temperature_service_message_in[2000];
bool temperature_service_message_ready = false;
bool raising_temp = false;
bool lowering_temp = false;
float atm_temp = 0;
float humidity = 0;
float previous_atm_temp = 0;
float previous_water_temp = 0;
float previous_humidity = 0;

bool THERM_INVERT_IO = CONFIG_THERM_INVERT_IO;

float TEMP_SET_POINT = CONFIG_TEMP_SET_POINT;
float TEMP_WINDOW = 1.0;

struct thermostat
{
  int control_pos_io;
  int control_neg_io;
  bool invert_io;
  float temp;
  float set_temp;
  float min_temp;
  float max_temp;
};

struct thermostat ts;

#include "drivers/i2c.c"
#include "drivers/ds18b20.c"

void
lower_temp ()
{
  if (lowering_temp) return;
  raising_temp = false;
  lowering_temp = true;
  gpio_set_level(ts.control_neg_io, true);
  gpio_set_level(ts.control_pos_io, false);
  ESP_LOGI(TAG, "Lowering temperature to: %f\t", TEMP_SET_POINT);
}

void
raise_temp ()
{
  if (raising_temp) return;
  raising_temp = true;
  lowering_temp = false;
  gpio_set_level(ts.control_neg_io, false);
  gpio_set_level(ts.control_pos_io, true);
  ESP_LOGI(TAG, "(min: %f) Raising temperature %f to %f", ts.min_temp, ts.temp, TEMP_SET_POINT);
}

void
stop_temp ()
{
  if (!raising_temp && !lowering_temp) return;
  raising_temp = false;
  lowering_temp = false;
  gpio_set_level(ts.control_neg_io, false);
  gpio_set_level(ts.control_pos_io, false);
  ESP_LOGI(TAG, "Set Point Reached: %f", TEMP_SET_POINT);
}

void
check_thermostat()
{
  if (ts.temp == 0) return;

  if (ts.temp < ts.min_temp) {
    raise_temp();
  }
  if (ts.temp > ts.max_temp) {
    lower_temp();
  }
  if (lowering_temp && ts.temp < TEMP_SET_POINT) {
    stop_temp();
  }
  if (raising_temp && ts.temp > TEMP_SET_POINT) {
    stop_temp();
  }
}

void
check_temp_state()
{
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

  if (ts.temp!=previous_water_temp) {
      check_thermostat();
      cJSON *number = cJSON_CreateNumber(ts.temp);
      cJSON_ReplaceItemInObjectCaseSensitive(state,"water_temp",number);
      previous_water_temp = ts.temp;
  }
}

static void
temperature_task(void *pvParameter)
{
  int temp = 0;
  xTaskCreate(&ds18b20_main, "ds18b20_main", 5000, NULL, 5, NULL);
  i2c_main();
  float previous_water_temp = 0;

  if (THERM_INVERT_IO) {
    ts.control_pos_io = CONTROL_NEG_IO;
    ts.control_neg_io = CONTROL_POS_IO;
  } else {
    ts.control_neg_io = CONTROL_NEG_IO;
    ts.control_pos_io = CONTROL_POS_IO;
  }
  ts.set_temp = TEMP_SET_POINT;
  ts.min_temp = ts.set_temp - TEMP_WINDOW / 2;
  ts.max_temp = ts.set_temp + TEMP_WINDOW / 2;

  while (1) {
    check_temp_state();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
