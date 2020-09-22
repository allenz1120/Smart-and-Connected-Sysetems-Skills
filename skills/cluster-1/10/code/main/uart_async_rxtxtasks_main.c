//Allen Zou 9/20/2020

/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_vfs_dev.h"

static const int RX_BUF_SIZE = 1024;

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define __ESP_INTR_ALLOC_H__
#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)
#define LEDPIN0 12
#define LEDPIN1 27
#define LEDPIN2 33
#define LEDPIN3 15
#define GPIOFLAG 26

// 14-Segment Display
#define SLAVE_ADDR 0x70              // alphanumeric address
#define OSC 0x21                     // oscillator cmd
#define HT16K33_BLINK_DISPLAYON 0x01 // Display on cmd
#define HT16K33_BLINK_OFF 0          // Blink off cmd
#define HT16K33_BLINK_CMD 0x80       // Blink cmd
#define HT16K33_CMD_BRIGHTNESS 0xE0  // Brightness cmd

// Master I2C
#define I2C_EXAMPLE_MASTER_SCL_IO 22        // gpio number for i2c clk
#define I2C_EXAMPLE_MASTER_SDA_IO 23        // gpio number for i2c data
#define I2C_EXAMPLE_MASTER_NUM I2C_NUM_0    // i2c port
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE 0 // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE 0 // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_FREQ_HZ 100000   // i2c master clock freq
#define WRITE_BIT I2C_MASTER_WRITE          // i2c master write
#define READ_BIT I2C_MASTER_READ            // i2c master read
#define ACK_CHECK_EN true                   // i2c master will check ack
#define ACK_CHECK_DIS false                 // i2c master will not check ack
#define ACK_VAL 0x00                        // i2c ack value
#define NACK_VAL 0xFF                       // i2c nack value

//0 ascending 1 descending
int flag = 0;

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    gpio_set_direction(LEDPIN0, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIOFLAG, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(LEDPIN0);
    gpio_pad_select_gpio(LEDPIN1);
    gpio_pad_select_gpio(LEDPIN2);
    gpio_pad_select_gpio(LEDPIN3);

    // Function to initiate i2c -- note the MSB declaration!
    // Debug
    // printf("\n>> i2c Config\n");
    int err;

    // Port configuration
    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;

    /// Define I2C configurations
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;                        // Master mode
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;        // Default SDA pin
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;            // Internal pullup
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;        // Default SCL pin
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;            // Internal pullup
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ; // CLK frequency
    err = i2c_param_config(i2c_master_port, &conf);     // Configure
    if (err == ESP_OK)
    {
        printf("- parameters: ok\n");
    }

    // Install I2C driver
    err = i2c_driver_install(i2c_master_port, conf.mode,
                             I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                             I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
    // i2c_set_data_mode(i2c_master_port,I2C_DATA_MODE_LSB_FIRST,I2C_DATA_MODE_LSB_FIRST);
    if (err == ESP_OK)
    {
        printf("- initialized: yes\n\n");
    }

    // Dat in MSB mode
    i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
}

int sendData(const char *logName, const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{              // Interrupt handler for your GPIO
    flag ^= 1; // Toggle state of flag
}

// Utility  Functions //////////////////////////////////////////////////////////

// Utility function to test for I2C device address -- not used in deploy
int testConnection(uint8_t devAddr, int32_t timeout)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int err = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

// Utility function to scan for i2c device
static void i2c_scanner()
{
    int32_t scanTimeout = 1000;
    printf("\n>> I2C scanning ..."
           "\n");
    uint8_t count = 0;
    for (uint8_t i = 1; i < 127; i++)
    {
        // printf("0x%X%s",i,"\n");
        if (testConnection(i, scanTimeout) == ESP_OK)
        {
            printf("- Device found at address: 0x%X%s", i, "\n");
            count++;
        }
    }
    if (count == 0)
        printf("- No I2C devices found!"
               "\n");
    printf("\n");
}

////////////////////////////////////////////////////////////////////////////////

// Alphanumeric Functions //////////////////////////////////////////////////////

// Turn on oscillator for alpha display
int alpha_oscillator()
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, OSC, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    vTaskDelay(200 / portTICK_RATE_MS);
    return ret;
}

// Set blink rate to off
int no_blink()
{
    int ret;
    i2c_cmd_handle_t cmd2 = i2c_cmd_link_create();
    i2c_master_start(cmd2);
    i2c_master_write_byte(cmd2, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd2, HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (HT16K33_BLINK_OFF << 1), ACK_CHECK_EN);
    i2c_master_stop(cmd2);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd2, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd2);
    vTaskDelay(200 / portTICK_RATE_MS);
    return ret;
}

// Set Brightness
int set_brightness_max(uint8_t val)
{
    int ret;
    i2c_cmd_handle_t cmd3 = i2c_cmd_link_create();
    i2c_master_start(cmd3);
    i2c_master_write_byte(cmd3, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd3, HT16K33_CMD_BRIGHTNESS | val, ACK_CHECK_EN);
    i2c_master_stop(cmd3);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd3, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd3);
    vTaskDelay(200 / portTICK_RATE_MS);
    return ret;
}

static void binaryLed(void *pvParameters)
{
    // static const char *TX_TASK_TAG = "TX_TASK";
    // esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    int counter = 0;
    if (flag == 0)
    {
        counter = 0;
    }
    else if (flag == 1)
    {
        counter = 16;
    }
    int temp;

    while (1)
    {

        temp = counter;
        if (temp / 8 == 1)
        {
            gpio_set_level(LEDPIN0, 1);
            temp = temp - 8;
        }
        else
        {
            gpio_set_level(LEDPIN0, 0);
        }
        if (temp / 4 == 1)
        {
            gpio_set_level(LEDPIN1, 1);
            temp = temp - 4;
        }
        else
        {
            gpio_set_level(LEDPIN1, 0);
        }
        if (temp / 2 == 1)
        {
            gpio_set_level(LEDPIN2, 1);
            temp = temp - 2;
        }
        else
        {
            gpio_set_level(LEDPIN2, 0);
        }
        if (temp / 1 == 1)
        {
            gpio_set_level(LEDPIN3, 1);
            temp = temp - 1;
        }
        else
        {
            gpio_set_level(LEDPIN3, 0);
        }
        if (flag == 0)
        {
            counter++;
            if (counter >= 16)
            {
                counter = 0;
            }
        }
        else if (flag == 1)
        {
            counter--;
            if (counter <= 0)
            {
                counter = 16;
            }
        }

        printf("blinking %d and flag is %d\n", counter, flag);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void test_alpha_display()
{
    // Debug
    int ret;
    printf(">> Test Alphanumeric Display: \n");

    // Set up routines
    // Turn on alpha oscillator
    ret = alpha_oscillator();
    if (ret == ESP_OK)
    {
        printf("- oscillator: ok \n");
    }
    // Set display blink off
    ret = no_blink();
    if (ret == ESP_OK)
    {
        printf("- blink: off \n");
    }
    ret = set_brightness_max(0xF);
    if (ret == ESP_OK)
    {
        printf("- brightness: max \n");
    }
    // Write to characters to buffer
    uint16_t displaybuffer[8];
    char displayStr[4];

    // Continually writes the same command
    while (1)
    {
        if (flag == 0)
        {
            displaybuffer[0] = 0b0000000000111110; // U.
            displaybuffer[1] = 0b0000000011110011; // P.
            displaybuffer[2] = 0b0000000000000000; // .
            displaybuffer[3] = 0b0000000000000000; // .
        }
        else if (flag == 1)
        {
            displaybuffer[0] = 0b0001001000001111; // T.
            displaybuffer[1] = 0b0000000000111111; // D.
            displaybuffer[2] = 0b0010100000110110; // C.
            displaybuffer[3] = 0b0010000100110110; // L.
        }

        // vTaskDelay(100);
        // counter++;
        // Send commands characters to display over I2C
        i2c_cmd_handle_t cmd4 = i2c_cmd_link_create();
        i2c_master_start(cmd4);
        i2c_master_write_byte(cmd4, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);
        for (uint8_t i = 0; i < 8; i++)
        {
            i2c_master_write_byte(cmd4, displaybuffer[i] & 0xFF, ACK_CHECK_EN);
            i2c_master_write_byte(cmd4, displaybuffer[i] >> 8, ACK_CHECK_EN);
        }
        i2c_master_stop(cmd4);
        ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd4, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd4);
    }
}

static void buttonInterrupt()
{
    gpio_intr_enable(GPIOFLAG);
    gpio_set_intr_type(GPIOFLAG, 1);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(GPIOFLAG, gpio_isr_handler, (void *)GPIOFLAG);

    while (1)
    {
        if (flag == 1)
        {
            gpio_set_level(GPIOFLAG, 1);
            vTaskDelay(50);
        }
        else if (flag == 0)
        {
            gpio_set_level(GPIOFLAG, 0);
            vTaskDelay(50);
        }
    }
}

void app_main(void)
{
    init();
    xTaskCreate(buttonInterrupt, "buttonInterrupt_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(binaryLed, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0,
                                        256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(UART_NUM_0);
    i2c_scanner();

    xTaskCreate(test_alpha_display, "test_alpha_display", 4096, NULL, 5, NULL);
    // xTaskCreate(ledBoard, "binaryLed_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}
