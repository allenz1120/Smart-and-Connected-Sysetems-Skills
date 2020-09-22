//Allen Zou 9/18/2020

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define LEDPIN0 12
#define LEDPIN1 27
#define LEDPIN2 33
#define LEDPIN3 15

void app_main(void)
{
    /* Configure the IOMUX register for pad LEDPIN0 (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_set_direction(LEDPIN0, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEDPIN3, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LEDPIN0);
    gpio_pad_select_gpio(LEDPIN1);
    gpio_pad_select_gpio(LEDPIN2);
    gpio_pad_select_gpio(LEDPIN3);
    int counter = 0;
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
        counter++;
        if (counter == 16)
        {
            counter = 0;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
