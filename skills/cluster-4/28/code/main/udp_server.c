//Allen Zou
//11/9/2020

/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include <ctype.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

//From Timer Example
#define TIMER_DIVIDER 16                             //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // to seconds
#define TIMER_INTERVAL_SEC (1)                       // Sample test interval for the first timer
#define TEST_WITH_RELOAD 1                           // Testing will be done with auto reload
//Timer Variables
#define ELECTION_TIMEOUT 15
#define LEADER_TIMEOUT 30
#define HEARTBEAT 1
#define UDP_TIMER 3

int timeout = ELECTION_TIMEOUT;
int udpTimer = UDP_TIMER;

#define PORT 9002

static const char *TAG = "example";

static const char *payload = "";

#define MAX 100

typedef enum
{
    ELECTION_STATE,
    LEADER_STATE,
    FOLLOWER_STATE
} state_e;

char status[MAX] = "No_Leader";
char myID[MAX] = "2";
char deviceAge[MAX] = "New";
char data[MAX];
char leaderHeartbeat[MAX] = "Dead";
state_e deviceState = ELECTION_STATE;
char transmitting[MAX] = "Yes";

// LED Output pins definitions
#define BLUEPIN 14
#define GREENPIN 32
#define REDPIN 15
#define ONBOARD 13

// TIMER CODE -------------------------------------------------
// A simple structure to pass "events" to main task
typedef struct
{
    int flag; // flag for enabling stuff in main code
} timer_event_t;

// Initialize queue handler for timer-based events
xQueueHandle timer_queue;

// ISR handler
void IRAM_ATTR timer_group0_isr(void *para)
{
    // Prepare basic event data, aka set flag
    timer_event_t evt;
    evt.flag = 1;

    // Clear the interrupt, Timer 0 in group 0
    TIMERG0.int_clr_timers.t0 = 1;

    // After the alarm triggers, we need to re-enable it to trigger it next time
    TIMERG0.hw_timer[TIMER_0].config.alarm_en = TIMER_ALARM_EN;

    // Send the event data back to the main program task
    xQueueSendFromISR(timer_queue, &evt, NULL);
}

// Initialize timer 0 in group 0 for 1 sec alarm interval and auto reload
static void alarm_init()
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TEST_WITH_RELOAD;
    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    // Timer's counter will initially start from value below
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

    // Configure the alarm value and the interrupt on alarm
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_INTERVAL_SEC * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_group0_isr,
                       (void *)TIMER_0, ESP_INTR_FLAG_IRAM, NULL);

    // Start timer
    timer_start(TIMER_GROUP_0, TIMER_0);
}

// The main task of this example program
static void timer_evt_task(void *arg)
{
    while (1)
    {
        // Create dummy structure to store structure from queue
        timer_event_t evt;

        // Transfer from queue
        xQueueReceive(timer_queue, &evt, portMAX_DELAY);

        // Do something if triggered!
        if (evt.flag == 1)
        {
            // printf("Action!\n");
            timeout--;
            udpTimer--;
            if (timeout < 0 && deviceState == ELECTION_STATE)
            {
                // printf("GOING TO LEADER STATE\n");
                deviceState = LEADER_STATE; // Change to leader state (Last remaining device in election state)
            }

            if (timeout < 0 && deviceState == FOLLOWER_STATE)
            {
                deviceState = ELECTION_STATE; // Change to election state
                timeout = ELECTION_TIMEOUT;   // Change timeout variable to election timeout constant
            }

            if (deviceState == LEADER_STATE)
            {
                if (udpTimer <= 0)
                {
                    udpTimer = HEARTBEAT;
                }
                printf("I AM THE LEADER!!!\n");
                strcpy(leaderHeartbeat, "Alive"); // Change leaderHeartbeat parameter in payload to "Alive" upon being elected leader
                strcpy(status, "Leader");         // Change status to "Leader"
            }
            // if(udpTimer == 0){
            //     udpTimer = 3;
            // }
            // if(timeout == 0){
            //     //if state is leader
            //         //go send heartbeat

            //     //if state is election

            // }

            // printf("timeout variable is: %d, udpTimer varibale is: %d", timeout, udpTimer);
        }
    }
}

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1)
    {
        if (addr_family == AF_INET)
        {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }
        else if (addr_family == AF_INET6)
        {
            bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
            dest_addr.sin6_family = AF_INET6;
            dest_addr.sin6_port = htons(PORT);
            ip_protocol = IPPROTO_IPV6;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        if (addr_family == AF_INET6)
        {
            // Note that by default IPV6 binds to both protocols, it is must be disabled
            // if both protocols used at the same time (used in CI)
            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        }
#endif

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        while (1)
        {
            char recv_status[MAX];
            char recv_ID[MAX];
            char recv_deviceAge[MAX];
            char recv_leaderHeartbeat[MAX];

            ESP_LOGI(TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (source_addr.sin6_family == PF_INET6)
                {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...

                // Returns first token
                char *token = strtok(rx_buffer, ",");

                // Keep printing tokens while one of the
                // delimiters present in str[].
                for (int i = 0; i < 4; i++)
                {
                    // printf("%s\n", token);
                    if (i == 0)
                    {
                        // printf("token at i=0: %s", token);
                        // printf("\n");
                        strcpy(recv_status, token);
                    }
                    else if (i == 1)
                    {
                        strcpy(recv_ID, token);
                        // printf("token at i=1: %s", token);
                        // printf("\n");
                    }
                    else if (i == 2)
                    {
                        strcpy(recv_deviceAge, token);
                        // printf("token at i=2: %s", token);
                        // printf("\n");
                    }
                    else if (i == 3)
                    {
                        strcpy(recv_leaderHeartbeat, token);
                        // printf("token at i=3: %s", token);
                        // printf("\n");
                    }

                    token = strtok(NULL, ",");
                }

                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                printf("recv_status is %s, recv_ID is %s, reccv_deviceAge is %s, recv_leaderHeartbeat is %s \n", recv_status, recv_ID, recv_deviceAge, recv_leaderHeartbeat);
                ESP_LOGI(TAG, "%s", rx_buffer);

                // Check device state and handle incoming data accordingly
                int myID_num = atoi(myID);
                int recv_ID_num = atoi(recv_ID);
                if (deviceState == ELECTION_STATE)
                {
                    printf("ELECTION STATE with timeout %d\n", timeout);
                    strcpy(status, "No_Leader"); // Status is "No_Leader"
                    strcpy(transmitting, "Yes"); // Continue transmitting
                    if (myID_num < recv_ID_num)
                    {
                        deviceState = ELECTION_STATE; // Stay in election state
                        strcpy(status, "No_Leader");  // Status is "No_Leader"
                        strcpy(transmitting, "Yes");  // Continue transmitting
                        printf("My id was lower than recv and i am resetting timeout \n");
                        timeout = ELECTION_TIMEOUT; // Reset election timeout
                    }
                    else if (myID_num > recv_ID_num)
                    {
                        deviceState = FOLLOWER_STATE; // Change to follower state
                        printf("My id was higher than recv and i am going to follower \n");
                        timeout = LEADER_TIMEOUT; // Change timeout variable to leader timeout constant
                    }
                }
                else if (deviceState == FOLLOWER_STATE)
                {
                    printf("FOLLOWER STATE with timeout of %d\n", timeout);
                    udpTimer = UDP_TIMER * 2;
                    strcpy(transmitting, "No"); // Stop transmitting
                    if (strcmp(recv_deviceAge, "New") == 0)
                    {
                        deviceState = ELECTION_STATE; // Change to election state
                    }
                    else if (strcmp(recv_leaderHeartbeat, "Alive") == 0)
                    {
                        timeout = LEADER_TIMEOUT; // Reset leader timeout upon receiving leader heartbeat
                        strcpy(status, "Leader"); // Update status to leader upon receiving leader heartbeat
                    }
                }
                else if (deviceState == LEADER_STATE)
                {
                    if (strcmp(recv_deviceAge, "New") == 0)
                    {
                        deviceState = ELECTION_STATE; // Change state to election state
                        timeout = ELECTION_TIMEOUT;   // Change timeout variable to election timeout constant
                    }
                }

                memset(recv_deviceAge, 0, sizeof(data));
                memset(recv_ID, 0, sizeof(data));
                memset(recv_status, 0, sizeof(data));
                memset(recv_leaderHeartbeat, 0, sizeof(data));

                //ERROR CHECKING
                int err = sendto(sock, payload, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

/* BSD Socket API Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/event_groups.h"
// #include "addr_from_stdin.h"

// #if defined(CONFIG_EXAMPLE_IPV4)
// #define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
// #elif defined(CONFIG_EXAMPLE_IPV6)
// #define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
// #else
#define HOST_IP_ADDR "192.168.1.171"
// #endif

#define PORT2 9003

// static const char *TAG = "example";

static void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1)
    {

#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT2);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = {0};
        inet6_aton(HOST_IP_ADDR, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT2);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_in6 dest_addr = {0};
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT2, SOCK_DGRAM, &ip_protocol, &addr_family, &dest_addr));
#endif

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT2);

        while (1)
        {
            if (udpTimer == 0)
            {
                memset(data, 0, sizeof(data));
                strcat(data, status);
                strcat(data, ",");
                strcat(data, myID);
                strcat(data, ",");
                strcat(data, deviceAge);
                strcat(data, ",");
                strcat(data, leaderHeartbeat);

                payload = data;
                printf("payload is: %s", payload);
                printf(" and we are in %d with a timeout of %d\n", deviceState, timeout);
                // printf("\n");

                int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                strcpy(deviceAge, "Old");
                // printf(deviceAge);
                printf("\n");
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
                ESP_LOGI(TAG, "Message sent");

                struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
                socklen_t socklen = sizeof(source_addr);

                udpTimer = UDP_TIMER;
            }
            // int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            // if (len < 0) {
            //     ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            //     break;
            // }
            // Data received
            // else {
            //     rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            //     ESP_LOGI(TAG, "Client received %d bytes from %s:", len, host_ip);
            //     printf(rx_buffer);
            //     printf("\n");
            //     ESP_LOGI(TAG, "%s", rx_buffer);
            //     if (strncmp(rx_buffer, "Ok!", 3) == 0) {
            //         printf("EXECUTING---------------------");
            //         ESP_LOGI(TAG, "Received expected message, reconnecting");
            //         break;
            //     }
            // }

            // vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

// GPIO init for LEDs
static void led_init()
{
    gpio_pad_select_gpio(BLUEPIN);
    gpio_pad_select_gpio(GREENPIN);
    gpio_pad_select_gpio(REDPIN);
    gpio_pad_select_gpio(ONBOARD);
    gpio_set_direction(BLUEPIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREENPIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(REDPIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ONBOARD, GPIO_MODE_OUTPUT);
}

// LED task to light LED based on traffic state
void led_task()
{
    while (1)
    {
        switch (deviceState)
        {
        case FOLLOWER_STATE: // Red
            gpio_set_level(GREENPIN, 0);
            gpio_set_level(REDPIN, 1);
            gpio_set_level(BLUEPIN, 0);
            // printf("Current state: %c\n",status);
            break;
        case LEADER_STATE: // Blue
            gpio_set_level(GREENPIN, 0);
            gpio_set_level(REDPIN, 0);
            gpio_set_level(BLUEPIN, 1);
            // printf("Current state: %c\n",status);
            break;
        case ELECTION_STATE: // Green
            gpio_set_level(GREENPIN, 1);
            gpio_set_level(REDPIN, 0);
            gpio_set_level(BLUEPIN, 0);
            // printf("Current state: %c\n",status);
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(udp_server_task, "udp_server", 4096, (void *)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(udp_server_task, "udp_server", 4096, (void *)AF_INET6, 5, NULL);
#endif

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
        * Read "Establishing Wi-Fi or Ethernet Connection" section in
        * examples/protocols/README.md for more information about this function.
        */
    // ESP_ERROR_CHECK(example_connect());

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);

    // Create a FIFO queue for timer-based
    timer_queue = xQueueCreate(10, sizeof(timer_event_t));

    // Create task to handle timer-based events
    xTaskCreate(timer_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);

    // Initiate alarm using timer API
    alarm_init();
    // Initiate LED init
    led_init();

    xTaskCreate(led_task, "set_traffic_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
}
