// Alex Prior
// 10/16/2020

/*
  Adapted I2C example code to work with the Adafruit ADXL343 accelerometer. Ported and referenced a lot of code from the Adafruit_ADXL343 driver code.
  ----> https://www.adafruit.com/product/4097

  Emily Lam, Aug 2019 for BU EC444
*/
#include <stdio.h>
#include <math.h>
#include "driver/i2c.h"
#include "./ADXL343.h"

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

// ADXL343
#define SLAVE_ADDR 0x62 // Lidar slave address

// Function to initiate i2c -- note the MSB declaration!
static void i2c_master_init()
{
  // Debug
  printf("\n>> i2c Config\n");
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
  if (err == ESP_OK)
  {
    printf("- initialized: yes\n");
  }

  // Data in MSB mode
  i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
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
    // printf("0x%X%s", i, "\n");
    if (testConnection(i, scanTimeout) == ESP_OK)
    {
      printf("- Device found at address: 0x%X%s", i, "\n");
      count++;
    }
  }
  if (count == 0)
  {
    printf("- No I2C devices found!"
           "\n");
  }
}

////////////////////////////////////////////////////////////////////////////////

// ADXL343 Functions ///////////////////////////////////////////////////////////

// Get Device ID
int getDeviceID(uint8_t *data)
{
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, ADXL343_REG_DEVID, ACK_CHECK_EN);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, data, ACK_CHECK_DIS);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

// Write one byte to register
void writeRegister(uint8_t reg, uint8_t data)
{
  // int ret;
  // printf("--Writing %d to reg %d!--\n", data, reg);
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  //start command
  i2c_master_start(cmd);
  //slave address followed by write bit
  i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
  //register pointer sent
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  //data sent
  i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
  //stop command
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  // if (ret == ESP_OK)
  // {
  //   printf("I2C SUCCESSFUL \n");
  // }
  i2c_cmd_link_delete(cmd);
}

// Read register
uint8_t readRegister(uint8_t reg)
{
  uint8_t value;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  //start command
  i2c_master_start(cmd);
  //slave followed by write bit
  i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
  //register pointer sent
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  //stop command
  // i2c_master_stop(cmd);

  //repeated start command
  i2c_master_start(cmd);
  //slave followed by read bit
  i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
  //place data from register into bus
  i2c_master_read_byte(cmd, &value, ACK_CHECK_DIS);
  //stop command
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return value;
}

static void lidarRead()
{
  uint8_t initReg = 0x00;
  uint8_t initData = 0x04;
  uint8_t data = 0x06;
  vTaskDelay(22);
  while (1)
  {
    // Initialize lidar device

    writeRegister(initReg, initData);
    data = readRegister(0x01);
    while ((data & 1) != 0x00)
    {
      data = readRegister(0x01);
      // printf("content of reg 1 LSB is %x \n", (data & 1));
      vTaskDelay(10);
    }
    // printf("content of reg 1 LSB outsie of the while loop is %x \n", (data & 1));
    // if (data == 0)
    // {
    // Read high and low distance bits off device
    uint8_t distHigh = readRegister(0x11);
    uint8_t distLow = readRegister(0x10);

    // Print high and low distance data
    printf("distHigh is: %x \n", distHigh);
    printf("distLow is: %x \n", distLow);

    //Add the two distances into a 16 bit int
    int distance = (distHigh << 8) + distLow;

    printf("--------------------Distance is: %d-------------------- \n", distance);
    // }
    vTaskDelay(100);
  }
}

void app_main()
{

  // Routine
  i2c_master_init();
  i2c_scanner();

  // Create task to poll Lidar
  xTaskCreate(lidarRead, "lidarRead", 4096, NULL, 5, NULL);
}
