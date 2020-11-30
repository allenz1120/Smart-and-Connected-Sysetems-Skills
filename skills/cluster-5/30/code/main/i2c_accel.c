//John Kircher, 11/16/2020

/* servo motor control example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define DRIVE_MIN_PULSEWIDTH 900     //Minimum pulse width in microsecond
#define DRIVE_MAX_PULSEWIDTH 1900    //Maximum pulse width in microsecond
#define DRIVE_MAX_DEGREE 180         //Maximum angle in degree upto which servo can rotate
#define STEERING_MIN_PULSEWIDTH 700  //Minimum pulse width in microsecond
#define STEERING_MAX_PULSEWIDTH 2100 //Maximum pulse width in microsecond
#define STEERING_MAX_DEGREE 180      //Maximum angle in degree upto which servo can rotate

static void mcpwm_example_gpio_initialize(void)
{
  printf("initializing mcpwm servo control gpio......\n");
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 26); //Set GPIO 26 as PWM0A, to which drive wheels are connected
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, 25); //Set GPIO 25 as PWM0A, to which steering servo is connected
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
static uint32_t drive_per_degree_init(uint32_t degree_of_rotation)
{
  uint32_t cal_pulsewidth = 0;
  cal_pulsewidth = (DRIVE_MIN_PULSEWIDTH + (((DRIVE_MAX_PULSEWIDTH - DRIVE_MIN_PULSEWIDTH) * (degree_of_rotation)) / (DRIVE_MAX_DEGREE)));
  return cal_pulsewidth;
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
static uint32_t steering_per_degree_init(uint32_t degree_of_rotation)
{
  uint32_t cal_pulsewidth = 0;
  cal_pulsewidth = (STEERING_MIN_PULSEWIDTH + (((STEERING_MAX_PULSEWIDTH - STEERING_MIN_PULSEWIDTH) * (degree_of_rotation)) / (STEERING_MAX_DEGREE)));
  return cal_pulsewidth;
}

void pwm_init()
{
  //1. mcpwm gpio initialization
  mcpwm_example_gpio_initialize();

  //2. initial mcpwm configuration
  printf("Configuring Initial Parameters of mcpwm......\n");
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 50; //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
  pwm_config.cmpr_a = 0;     //duty cycle of PWMxA = 0
  pwm_config.cmpr_b = 0;     //duty cycle of PWMxb = 0
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings
}

void calibrateESC()
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);                                // Give yourself time to turn on crawler
  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 2100); // HIGH signal in microseconds - backwards
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 700); // LOW signal in microseconds - forwards
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1400); // NEUTRAL signal in microseconds
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1400); // reset the ESC to neutral (non-moving) value
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

/**
 * @brief Configure MCPWM module
 */
void drive_control(void *arg)
{
  uint32_t angle, count;

  for (count = 1400; count > 1200; count -= 5)
  {
    printf("DRIVING--------------------");
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, count);
    vTaskDelay(100 / portTICK_RATE_MS);
  }

  for (count = 1200; count < 1600; count += 5)
  {
    printf("DRIVING--------------------");
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, count);
    vTaskDelay(100 / portTICK_RATE_MS);
  }

  for (count = 1600; count >= 1400; count -= 5)
  {
    printf("DRIVING--------------------");
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, count);
    vTaskDelay(100 / portTICK_RATE_MS);
  }

  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1400);
  vTaskDelay(100 / portTICK_RATE_MS);

  //}
  //}
  vTaskDelete(NULL);
}

/**
 * @brief Configure MCPWM module
 */
void steering_control(void *arg)
{
  uint32_t right, center, left;

  while (1)
  {

    //count = 90;

    right = steering_per_degree_init(0);
    center = steering_per_degree_init(105);
    left = steering_per_degree_init(180);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, right);
    vTaskDelay(1000 / portTICK_RATE_MS);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, center);
    vTaskDelay(1000 / portTICK_RATE_MS);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, left);
    vTaskDelay(1000 / portTICK_RATE_MS);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, center);
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}

void app_main(void)
{

  pwm_init();
  calibrateESC();

  printf("Testing servo motor.......\n");
  xTaskCreate(drive_control, "drive_control", 4096, NULL, 4, NULL);
  xTaskCreate(steering_control, "steering_control", 4096, NULL, 5, NULL);
}