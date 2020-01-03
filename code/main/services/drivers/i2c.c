#include <stdio.h>
#include "driver/i2c.h"
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"

#define DATA_LENGTH          256  /*!<Data buffer length for test buffer*/
#define RW_TEST_LENGTH       129  /*!<Data length for r/w test, any value from 0-DATA_LENGTH*/
#define CLIMATE_DELAY_TIME   1234 /*!< delay time between different test items */

#define I2C_EXAMPLE_SLAVE_NUM I2C_NUM_0    /*!<I2C port number for slave dev */
#define I2C_EXAMPLE_SLAVE_TX_BUF_LEN  (2*DATA_LENGTH) /*!<I2C slave tx buffer size */
#define I2C_EXAMPLE_SLAVE_RX_BUF_LEN  (2*DATA_LENGTH) /*!<I2C slave rx buffer size */

#define I2C_EXAMPLE_MASTER_SCL_IO    21    /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO    19    /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */

#define TSL4531_SENSOR_ADDR  0x29    /*!< slave address for TSL4531 sensor */
#define TSL4531_CMD   0x04    /*!< Command to set measure mode */

#define SI7020_SENSOR_ADDR          0x40    /*!< slave address for SI7020 sensor */
#define SI7020_CMD_MEASURE_TEMP     0xF3    /*!< Command to set measure mode */
#define SI7020_CMD_MEASURE_HUM      0xF5    /*!< Command to set measure mode */

#define TSL4531_SENSOR_ADDR         0x29    /*!< slave address for SI7020 sensor */
#define TSL4531_CMD_MEASURE_LIGHT   0x04    /*!< Command to set measure mode */

#define BH1750_SENSOR_ADDR  0x29    /*!< slave address for BH1750 sensor */
#define BH1750_CMD_START    0x00    /*!< Command to set measure mode */
#define ESP_SLAVE_ADDR 0x29         /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

char temp_str[50];
bool climate_received = false;
uint8_t mac[6];
char climate_rx_data[256];
char climate_command[256];


char temperature_str[100] = "0";
char humidity_str[100] = "0";
char light_level_str[100] = "0";
bool climate_data_ready = false;

char front_climate_str[100];
char i_str[10];
int climate_linked = 0;
char climate_str[250] = "";
char climate_req_str[1024];

float tempature_cf = 429.1;
float tempature_cb = 17736.5;
float humidity_cf =  346.2;
float humidity_cb = 8576.9;

static esp_err_t SI7020_measure_temp(i2c_port_t i2c_num, uint8_t* data_h, uint8_t* data_l)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7020_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SI7020_CMD_MEASURE_TEMP, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ret;
    }

    vTaskDelay(30 / portTICK_RATE_MS);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7020_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_h, ACK_VAL);
    i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t SI7020_measure_hum(i2c_port_t i2c_num, uint8_t* data_h, uint8_t* data_l)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7020_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SI7020_CMD_MEASURE_HUM, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ret;
    }

    vTaskDelay(30 / portTICK_RATE_MS);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7020_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_h, ACK_VAL);
    i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void i2c_example_master_init()
{
    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
}

static void disp_buf(uint8_t* buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if (( i + 1 ) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

static void climate_task(void* arg)
{
    int i = 0;
    int ret;
    uint32_t task_idx = (uint32_t) arg;
    uint8_t* data = (uint8_t*) malloc(DATA_LENGTH);
    uint8_t* data_wr = (uint8_t*) malloc(DATA_LENGTH);
    uint8_t* data_rd = (uint8_t*) malloc(DATA_LENGTH);
    uint8_t sensor_data_h, sensor_data_l;
    while (1) {
        //--------------------------------------------------//
        ret = SI7020_measure_temp( I2C_EXAMPLE_MASTER_NUM, &sensor_data_h, &sensor_data_l);
        if (ret == ESP_OK) {
            //printf("temperature: %d\n", ( sensor_data_h << 8 | sensor_data_l ));
            atm_temp = ( sensor_data_h << 8 | sensor_data_l ) - tempature_cb;
            atm_temp /= tempature_cf;
            // sprintf(temperature_str,"%d", atm_temp);
            // printf("Atmospheric Temperature: %s\n",temperature_str);
        } else {
            printf("SI7020_measure_temp No ack, sensor not connected\n");
        }
        vTaskDelay(( CLIMATE_DELAY_TIME * ( task_idx + 1 ) ) / portTICK_RATE_MS);

        //--------------------------------------------------//
        ret = SI7020_measure_hum( I2C_EXAMPLE_MASTER_NUM, &sensor_data_h, &sensor_data_l);
        if (ret == ESP_OK) {
            //printf("data_h: %02x\n", sensor_data_h);
            //printf("data_l: %02x\n", sensor_data_l);
            //printf("humidity: %d\n", ( sensor_data_h << 8 | sensor_data_l ));
            humidity = ( sensor_data_h << 8 | sensor_data_l ) - humidity_cb;
            humidity /= humidity_cf;
            // sprintf(humidity_str,"%d", humidity);
            // printf("atmironment Humidity: %s\n",humidity_str);
        } else {
            //printf("SI7020_measure_hum No ack, sensor not connected\n");
        }
        vTaskDelay(( CLIMATE_DELAY_TIME * ( task_idx + 1 ) ) / portTICK_RATE_MS);
    }
}

void i2c_main(void)
{
    i2c_example_master_init();
    xTaskCreate(climate_task, "climate_task", 1024 * 2, (void *)0, 10, NULL);
}
