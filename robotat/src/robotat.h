/**
 * @file robotat.h
 * @author Camilo Perafan Montoya (per17092@uvg.edu.gt)
 * @brief Header file containing the function prototypes for the communications network of the Robotat ecosystem.
 * 
 * This file presents the prototypes of the functions used for communications in the
 * robotic ecosystem. There are 4 functions, which are used for: initializing the 
 * communications of the Robotat ecosystem (WiFi and MQTT), for publishing a message to a specified
 * topic via MQTT, for retrieving the data recieved via MQTT to use it elsewhere in the code and for 
 * retrieving the status on which the Robotat network is currently. It should also be noted that 
 * it saves the chosen Robotat ID during a routine in the flash memory, so it is impervious to 
 * resets or removals of power.
 * 
 * @version 0.4
 * @date 2021-10-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ROBOTAT_H
#define ROBOTAT_H

//-------------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"

//-------------------------------------------------------------------------------------------------------
// Macros Definition
//-------------------------------------------------------------------------------------------------------
#define WIFI_SSID                     "WiFi_Robotat"
#define WIFI_PASS                     "R0bot4tUVG"
#define WIFI_MAX_RETRY                10
#define MQTT_HOST                     "192.168.0.2"
#define MQTT_PORT                     1883
#define MQTT_IDENTIFIER               "ESP32_Drone"
#define CONNECTED_LED                 2
#define WIFI_CONNECTED_BIT            BIT0
#define WIFI_FAIL_BIT                 BIT1
#define MAX_STRING_LENGTH_MQTT        500
#define MAX_STRING_LENGTH_OK_MESSAGE  50
#define MAX_DATA_LENGTH_MQTT          55

//-------------------------------------------------------------------------------------------------------
// Data Type Definitions
//-------------------------------------------------------------------------------------------------------
/**
 * @brief Definition of the data type robotat_state_t which describes the status of the robot in regards to the Robotat network.
 *
 * The different states on which the connection can be are described below:
 * 
 * ROBOTAT_CONNECTING_MQTT: The device is connecting to the Robotat network (especifically to the MQTT broker).
 * 
 * ROBOTAT_CONNECTION_ERROR_WIFI: The device could not connect to the Robotat network (especifically to the WiFi network).
 * 
 * ROBOTAT_CONNECTION_ERROR_MQTT: The device could not connect to the Robotat network (especifically to the MQTT broker).
 * 
 * ROBOTAT_CONNECTION_SUCCESS_WIFI: The device connected successfully to the Robotat network (especifically to the WiFi network).
 * 
 * ROBOTAT_CONNECTION_SUCCESS_MQTT: The device connected successfully to the Robotat network (especifically to the MQTT broker).
 * 
 * ROBOTAT_DISCONNECTED_MQTT: The device disconnected from the Robotat network (especifically from the MQTT broker).
 * 
 * ROBOTAT_SUBSCRIBED_TO_TOPIC: The device subscribed to a topic in the Robotat network.
 * 
 * ROBOTAT_MESSAGE_PUBLISHED: The device published a message to a topic in the Robotat network.
 * 
 * ROBOTAT_DATA_RECEIVED: The device recieved a message sent from a topic in the Robotat network.
 * 
 */
typedef enum 
{
    ROBOTAT_CONNECTING_MQTT,
    ROBOTAT_CONNECTION_ERROR_WIFI,
    ROBOTAT_CONNECTION_ERROR_MQTT,
    ROBOTAT_CONNECTION_SUCCESS_WIFI,
    ROBOTAT_CONNECTION_SUCCESS_MQTT,
    ROBOTAT_DISCONNECTED_MQTT,
    ROBOTAT_SUBSCRIBED_TO_TOPIC,
    ROBOTAT_MESSAGE_PUBLISHED,
    ROBOTAT_DATA_RECEIVED,

} robotat_state_t;

//-------------------------------------------------------------------------------------------------------
// Definition of Function Prototypes
//-------------------------------------------------------------------------------------------------------
/**
 * @brief Configures and initializes the WiFi and MQTT connections with the parameters specified in the macros.
 * 
 */
void 
robotat_connect(void);

/**
 * @brief Publishes a message via MQTT to the speceified topic.
 * 
 * @param[in] topic Topic to publish to.
 * @param[in] data Message to be sent.
 */
void 
robotat_publish(const char *topic, const char *data);

/**
 * @brief Obtains the data array received via MQTT to be used elsewhere.
 * 
 * @return Pointer to the (read-only) received data array.
 */
const float*
robotat_get_data(void);

/**
 * @brief Obtains the status of the device in regards to the Robotat network.
 * 
 * @return Status on which the device is currently in regards to the Robotat network.
 */
robotat_state_t
robotat_get_status(void);

#endif //ROBOTAT_H