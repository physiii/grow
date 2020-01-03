#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#if CONFIG_IDF_TARGET_ESP32
#include "esp_adc_cal.h"
#endif

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
  adc_channel_t channel;
  adc_atten_t atten;
  adc_unit_t unit;
  esp_adc_cal_characteristics_t *adc_chars;
  int nutrient_pump_io;
  int water_pump_io;
  float value;
  float set_value;
  float min_value;
  float max_value;
  float bias;
  float slope;
};

struct ec_controller ec;

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   1000        //Multisampling
uint32_t ec_reading = 0;

// static esp_adc_cal_characteristics_t *adc_chars;
// static const adc_channel_t channel = ADC_CHANNEL_5;     //GPIO34 if ADC1, GPIO14 if ADC2
// static const adc_atten_t atten = ADC_ATTEN_DB_11;
// static const adc_unit_t unit = ADC_UNIT_1;

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
  if (ec.value  > ec.max_value) {
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

  ec.channel = CONFIG_EC_ADC_CHANNEL;
  ec.supply_io = CONFIG_EC_SUPPLY_IO;
  ec.set_value = EC_SET_POINT;
  ec.min_value = ec.set_value - EC_WINDOW / 2;
  ec.max_value = ec.set_value + EC_WINDOW / 2;
  ec.value = 0;
  ec.bias = 0.1137;
  ec.slope = 0.00207;
  xTaskCreate(ec_timer, "ec_timer", 2048, NULL, 10, NULL);

  //Configure ADC
  if (ec.unit == ADC_UNIT_1) {
      adc1_config_width(ADC_WIDTH_BIT_12);
      adc1_config_channel_atten(ec.channel, ec.atten);
  } else {
      adc2_config_channel_atten((adc2_channel_t)ec.channel, ec.atten);
  }

  while (1) {
    uint32_t adc_reading = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      adc_reading += adc1_get_raw((adc1_channel_t)ec.channel);
    }
    adc_reading /= NO_OF_SAMPLES;

    ec.value = adc_reading * 0.002072;
    vTaskDelay(pdMS_TO_TICKS(1000));
    check_ec_state();
  }
}
