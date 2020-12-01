// Allen Zou
// 11/23/2020

/* ADC1 Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF 1100 //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  //Multisampling

#define RED 15
#define GREEN 32
#define BLUE 14

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6; //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

// PID -----------------------------------------

// Flag for dt
int dt_complete = 0;

// // Define timer interrupt handler
// void IRAM_ATTR timer_isr()
// {
//     // Clear interrupt
//     TIMERG0.int_clr_timers.t0 = 1;
//     // Indicate timer has fired
//     dt_complete = 1;
// }

// // Set up periodic timer for dt = 100ms
// static void periodic_timer_init()
// {
//     // Basic parameters of the timer
//     timer_config_t config;

//     // register timer interrupt
//     timer_init();

//     // Timer's counter will initially start from value below
//     timer_set_counter_value();

//     // Configure the alarm value and the interrupt on alarm.
//     timer_set_alarm_value();
//     timer_enable_intr();
//     timer_isr_register();

//     // Start timer
//     timer_start();
// }

int dt = 100;
int setpoint = 50;

float previous_error = 0.00; // Set up PID loop
float integral = 0.00;
float derivative;
float output;
int error;

int Kp;
int Kd;
int Ki;

int range;

// GPIO init for LEDs
static void led_init()
{
    gpio_pad_select_gpio(BLUE);
    gpio_pad_select_gpio(GREEN);
    gpio_pad_select_gpio(RED);
    gpio_set_direction(BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RED, GPIO_MODE_OUTPUT);
}

void PID()
{
    integral = 0.00;
    previous_error = 0.00;
    while (1)
    {
        error = setpoint - range;

        if (error > 0)
        {
            printf("BLUE \n");
            gpio_set_level(RED, 0);
            gpio_set_level(GREEN, 0);
            gpio_set_level(BLUE, 1);
        }
        else if (error < 0)
        {
            printf("RED \n");
            gpio_set_level(RED, 1);
            gpio_set_level(GREEN, 0);
            gpio_set_level(BLUE, 0);
        }
        else if (error == 0)
        {
            printf("GREEN \n");
            gpio_set_level(RED, 0);
            gpio_set_level(GREEN, 1);
            gpio_set_level(BLUE, 0);
        }

        integral = integral + error * dt;
        derivative = (error - previous_error) / dt;
        output = Kp * error + Ki * integral + Kd * derivative;
        previous_error = error;
        vTaskDelay(dt);
    }
}

// While(1){
// 	if dt_complete == 1 {
//        PID();
//        dt_complete = 0;
//        // Re-enable alarm
//        TIMERG0.hw_timer[TIMER_0].config.alarm_en = TIMER_ALARM_EN;
// 	}
// }

static void check_efuse(void)
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        printf("eFuse Two Point: Supported\n");
    }
    else
    {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        printf("eFuse Vref: Supported\n");
    }
    else
    {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse Vref\n");
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}

void app_main(void)
{
    // LED Init
    led_init();

    // PID Task
    xTaskCreate(PID, "PID", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);

    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    //Continuously sample ADC1
    while (1)
    {
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++)
        {
            if (unit == ADC_UNIT_1)
            {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            }
            else
            {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        uint32_t vcm = 3.222;  //conversion to get volts per centimeter. This is found by 3.3V / 1024
        range = voltage / vcm; //calculation to get range in centimeters.
        printf("Raw: %d\tCentimeters: %dcm\n", adc_reading, range);
        vTaskDelay(pdMS_TO_TICKS(1000)); //2 second delay
    }
}
