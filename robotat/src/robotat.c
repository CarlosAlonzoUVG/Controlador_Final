/**
 * @file robotat.c
 * @author Camilo Perafan Montoya (per17092@uvg.edu.gt)
 * @brief Source file containing the functions for the communications network of the Robotat ecosystem.
 * 
 * This file contains the functions presented in the robotat.h file. There are 4 functions, which are 
 * used for: initializing the communications of the Robotat ecosystem (WiFi and MQTT), for publishing 
 * a message to a specified topic via MQTT, for retrieving the data recieved via MQTT to use it elsewhere 
 * in the code and for retrieving the status on which the Robotat network is currently. It should also 
 * be noted that it saves the chosen Robotat ID during a routine in the flash memory, so it is 
 * impervious to resets or removals of power.
 * 
 * @version 0.4
 * @date 2021-10-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

//-------------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------------
#include "robotat.h"

//-------------------------------------------------------------------------------------------------------
// Variable Definition
//-------------------------------------------------------------------------------------------------------
static uint8_t str_len             = 0;
static uint8_t robotat_id_use      = 0;
static uint8_t robotat_id          = 0;
static uint8_t chosen_robotat_id   = 0;
static uint8_t robotat_id_use_flag = 0;
static uint8_t i                   = 0;
static uint8_t s_retry_num         = 0;

static char  *data_ptr;

static char  data_copy [MAX_STRING_LENGTH_MQTT];
static float data_mqtt [MAX_DATA_LENGTH_MQTT];
static char  mes_ok_id [MAX_STRING_LENGTH_OK_MESSAGE];

static robotat_state_t device_status;

static EventGroupHandle_t s_wifi_event_group;
static esp_mqtt_client_handle_t client;

static esp_err_t storage = 0;
static nvs_handle_t flash_save_handle;

//-------------------------------------------------------------------------------------------------------
// Definition of Function Prototypes (Internal)
//-------------------------------------------------------------------------------------------------------
/**
 * @brief Parses the data recieved via MQTT and stores it inside the float array "data_mqtt" (function only to be used inside "mqtt_event_handler").
 * 
 * @param mes_data Message to parse.
 * @param mes_len Length of the message to parse.
 * @param mes_top Topic from which the message was recieved.
 * @param top_len Length of the topic from which the message was recieved.
 */
static void 
data_parsing_mqtt(char *mes_data, int mes_len, char *mes_top, int top_len);

/**
 * @brief Handles the events generated by the MQTT connection (function only to be used inside "robotat_connect").
 * 
 * @param handler_args Arguments of the handler.
 * @param base Subsystem that exposes the event described.
 * @param event_id Describes what case occurs during the event described.
 * @param event_data Contains the data obtained in a case.
 */
static void 
mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 * @brief Handles the events generated by the WiFi connection (function only to be used inside "robotat_connect").
 * 
 * @param arg Arguments of the handler.
 * @param event_base Subsystem that exposes the event described.
 * @param event_id Describes what case occurs during the event described.
 * @param event_data Contains the data obtained in a case.
 */
static void 
wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//-------------------------------------------------------------------------------------------------------
// Definition of Functions
//-------------------------------------------------------------------------------------------------------
static void 
data_parsing_mqtt(char *mes_data, int mes_len, char *mes_top, int top_len)
{
        strncpy(data_copy, mes_data, mes_len);

        str_len = (int) strtof(data_copy, &data_ptr);
        robotat_id_use = (int) strtof(data_ptr, &data_ptr);
        robotat_id = (int) strtof(data_ptr, &data_ptr);
        sprintf(mes_ok_id, "%d", robotat_id);

        storage = nvs_open("storage", NVS_READWRITE, &flash_save_handle);
        storage = nvs_get_u8(flash_save_handle, "id_use_flag", &robotat_id_use_flag);
        storage = nvs_get_u8(flash_save_handle, "chosen_mqtt_id", &chosen_robotat_id);
        nvs_close(flash_save_handle);

        if ((robotat_id_use == 0) & (robotat_id_use_flag == 0))
        {
            robotat_id_use_flag = 1;
            strcat(mes_ok_id, " ");
            strcat(mes_ok_id, MQTT_IDENTIFIER);
            chosen_robotat_id = robotat_id;
            data_mqtt[0] = (float) chosen_robotat_id;
            esp_mqtt_client_publish(client, "Used_IDs", mes_ok_id, 0, 2, 0);

            storage = nvs_open("storage", NVS_READWRITE, &flash_save_handle);
            storage = nvs_set_u8(flash_save_handle, "id_use_flag", robotat_id_use_flag);
            storage = nvs_set_u8(flash_save_handle, "chosen_mqtt_id", chosen_robotat_id);
            storage = nvs_commit(flash_save_handle);
            nvs_close(flash_save_handle);
        }

        if ((robotat_id == chosen_robotat_id) & (robotat_id_use_flag == 1))
        {
            /*printf("Topic: %.*s\n", top_len, mes_top);
            printf("Data Length: %d\n", mes_len);
            printf("Array Length: %d\n", (str_len-2));
            printf("Data:\n");
            printf("%.4F\n", data_mqtt[0]);*/

            for (i = 1; i < (str_len-2); ++i)
            {
                data_mqtt[i] = strtof(data_ptr, &data_ptr);
                //printf("%.4F\n", data_mqtt[i]);
            }
        }
}


static void 
mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t) event_id) {
    case MQTT_EVENT_CONNECTED:
        printf("-- MQTT -- Connected\n");
        /* By default it is connected to the "Robotat" topic.
           If it is desired to subscribe to another topic,
           change the second parameter of the function
           "esp_mqtt_client_subscribe" to the desired topic.
           If it is desired to connect to more than one topic,
           it is necessary to use the function below once per topic.
           Example of use for more than one topic:
           esp_mqtt_client_subscribe(client, "Topic1", 2);
           esp_mqtt_client_subscribe(client, "Topic2", 2);
        */
        //gpio_set_level(CONNECTED_LED, 1);
        device_status = ROBOTAT_CONNECTION_SUCCESS_MQTT;
        esp_mqtt_client_subscribe(client, "Robotat", 2);
        break;

    case MQTT_EVENT_DISCONNECTED:
        printf("-- MQTT -- Disconnected\n");
        device_status = ROBOTAT_DISCONNECTED_MQTT;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        printf("-- MQTT -- Subscribed to Topic\n");
        device_status = ROBOTAT_SUBSCRIBED_TO_TOPIC;
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        printf("-- MQTT -- Unsubscribed to Topic\n");
        break;

    case MQTT_EVENT_PUBLISHED:
        printf("-- MQTT -- Message Published\n");
        device_status = ROBOTAT_MESSAGE_PUBLISHED;
        break;

    case MQTT_EVENT_DATA:
        //printf("-- MQTT -- Data Received\n");
        device_status = ROBOTAT_DATA_RECEIVED;
        data_parsing_mqtt(event->data, event->data_len, event->topic, event->topic_len);      
        break;

    case MQTT_EVENT_ERROR:
        printf("-- MQTT -- Error\n");
        device_status = ROBOTAT_CONNECTION_ERROR_MQTT;
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            if ((event->error_handle->esp_tls_last_esp_err) != 0) {
                printf("-- MQTT -- Last error Reported from ESP-TLS: 0x%x\n", (event->error_handle->esp_tls_last_esp_err));
            }
            if ((event->error_handle->esp_tls_stack_err) != 0) {
                printf("-- MQTT -- Last error Reported from TLS Stack: 0x%x\n", (event->error_handle->esp_tls_stack_err));
            }
            if ((event->error_handle->esp_transport_sock_errno) != 0) {
                printf("-- MQTT -- Last error Captured as Transport's Socket Errno: 0x%x\n", (event->error_handle->esp_transport_sock_errno));
            }
            printf("-- MQTT -- Last Errno String (%s)\n", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        printf("-- MQTT -- Other Event ID (Default): %d\n", event->event_id);
        device_status = ROBOTAT_CONNECTING_MQTT;
        break;
    }
}


static void 
wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            printf("-- WIFI -- Retrying to Connect to the AP\n");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        printf("-- WIFI -- Failed to Connect to the AP\n");
        device_status = ROBOTAT_CONNECTION_ERROR_WIFI;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("-- WIFI -- IP: %d.%d.%d.%d\n", IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        device_status = ROBOTAT_CONNECTION_SUCCESS_WIFI;
    }
}


void 
robotat_connect(void)
{   
    storage = nvs_flash_init();
    if (storage == ESP_ERR_NVS_NO_FREE_PAGES || storage == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      storage = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(storage);
    
   // gpio_pad_select_gpio(CONNECTED_LED);
   // gpio_set_direction(CONNECTED_LED, GPIO_MODE_OUTPUT);

    esp_mqtt_client_config_t mqtt_cfg = {
        .host = MQTT_HOST,
        .port = MQTT_PORT,
        .client_id = MQTT_IDENTIFIER,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);   

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("-- WIFI -- WiFi Start Finished\n");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        printf("-- WIFI -- Connected to AP -> SSID: %s Password: %s\n", WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        printf("-- WIFI -- Failed to Connect to AP -> SSID: %s Password: %s\n", WIFI_SSID, WIFI_PASS);
    } else {
        printf("-- WIFI -- Unexpected Event\n");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}


void 
robotat_publish(const char *topic, const char *data)
{
    esp_mqtt_client_publish(client, topic, data, 0, 2, 0);
}


const float* 
robotat_get_data(void)
{
    return data_mqtt;
}

robotat_state_t
robotat_get_status(void)
{
    return device_status;
}