/***************************************************************************************************
                                    ExploreEmbedded Copyright Notice    
****************************************************************************************************
 * File:   AWS_IOT.cpp
 * Version: 1.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: ESP32  Arduino library for AWS IOT.
 
This code has been developed and tested on ExploreEmbedded boards.  
We strongly believe that the library works on any of development boards for respective controllers. 
Check this link http://www.exploreembedded.com/wiki for awesome tutorials on 8051,PIC,AVR,ARM,Robotics,RTOS,IOT.
ExploreEmbedded invests substantial time and effort developing open source HW and SW tools, to support consider buying the ExploreEmbedded boards.
 
The ExploreEmbedded libraries and examples are licensed under the terms of the new-bsd license(two-clause bsd license).
See also: http://www.opensource.org/licenses/bsd-license.php

EXPLOREEMBEDDED DISCLAIMS ANY KIND OF HARDWARE FAILURE RESULTING OUT OF USAGE OF LIBRARIES, DIRECTLY OR
INDIRECTLY. FILES MAY BE SUBJECT TO CHANGE WITHOUT PRIOR NOTICE. THE REVISION HISTORY CONTAINS THE INFORMATION 
RELATED TO UPDATES.
 

Permission to use, copy, modify, and distribute this software and its documentation for any purpose
and without fee is hereby granted, provided that this copyright notices appear in all copies 
and that both those copyright notices and this permission notice appear in supporting documentation.
**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "AWS_IOT.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"

#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"


 
static const char *TAG = "AWS_IOT";
char AWS_IOT_HOST_ADDRESS[128];

//char cPayload[512];

//IoT_Publish_Message_Params paramsQOS0;
IoT_Publish_Message_Params paramsQOS1;
pSubCallBackHandler_t subApplCallBackHandler = 0;



/* CA Root certificate, device ("Thing") certificate and device
 * ("Thing") key.

   "Embedded Certs" are stored in the file aws_iot_certificates.c as arrays
*/


void aws_iot_task(void *param);



void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
        IoT_Publish_Message_Params *params, void *pData) 
{    
    if(subApplCallBackHandler != 0){ //User call back if configured
		subApplCallBackHandler(topicName, topicNameLen, (char *)params->payload, params->payloadLen,pData);
	}
}



void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) 
    {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } 
    else
    {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
      //  rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } 
        else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}


int AWS_IOT::connect(const char *hostAddress, const char *clientID, const char* will_topic, const uint16_t will_topic_len)
{
    const size_t stack_size = 36*1024;
    
    strcpy(AWS_IOT_HOST_ADDRESS,hostAddress);
    IoT_Error_t rc = FAILURE;


    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
    

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = AWS_IOT_HOST_ADDRESS;
    mqttInitParams.port = CONFIG_AWS_IOT_MQTT_PORT;


    mqttInitParams.pRootCALocation = aws_root_ca_pem.c_str();
    mqttInitParams.pDeviceCertLocation = certificate_pem_crt.c_str();
    mqttInitParams.pDevicePrivateKeyLocation = private_pem_key.c_str();
	
    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;


    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
   
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        return rc; //abort();
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = clientID;
    connectParams.clientIDLen = (uint16_t) strlen(clientID);
    connectParams.isWillMsgPresent = true;
	IoT_MQTT_Will_Options will_msg;
	will_msg.struct_id[0] = 'M';
	will_msg.struct_id[1] = 'Q';
	will_msg.struct_id[2] = 'T';
	will_msg.struct_id[3] = 'W';
	will_msg.pTopicName = const_cast<char*>(will_topic);
	will_msg.topicNameLen = will_topic_len;
	will_msg.pMessage = "{\"is_alive\":false}";
	will_msg.msgLen = 18;
	will_msg.isRetained = false;
	will_msg.qos = QOS0;
	connectParams.will = will_msg;

    ESP_LOGI(TAG, "Connecting to AWS...");
    
	rc = aws_iot_mqtt_connect(&client, &connectParams);
	
	if(SUCCESS != rc) {
		ESP_LOGE(TAG, "Error(%d) connecting to %s:%d.", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		
	}
        
 

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    //rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    //if(SUCCESS != rc) {
        //ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        //abort();
    //} 
    
    //if(rc == SUCCESS){
		//xTaskCreate(&aws_iot_task, "aws_iot_task", stack_size, NULL, 5, NULL);
		//xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", stack_size, NULL, 5, NULL, 0);
	//}

    return rc;
}

int AWS_IOT::disconnect()
{
	IoT_Error_t rc;
	aws_iot_mqtt_disconnect(&client);
	return rc;
}

int AWS_IOT::publish(const char *pubtopic,const char *pubPayLoad, const bool isRetained)
{
    IoT_Error_t rc;

    // paramsQOS0.qos = QOS0;
    // paramsQOS0.payload = (void *) pubPayLoad;
    // paramsQOS0.isRetained = isRetained;
    // paramsQOS0.payloadLen = strlen(pubPayLoad);
	
	paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *) pubPayLoad;
    paramsQOS1.isRetained = isRetained;
    paramsQOS1.payloadLen = strlen(pubPayLoad);
	
    rc = aws_iot_mqtt_publish(&client, pubtopic, strlen(pubtopic), &paramsQOS1);
    
    return rc;  
}



int AWS_IOT::subscribe(const char *subTopic, pSubCallBackHandler_t pSubCallBackHandler, void* user_data)
{
    IoT_Error_t rc;
    
    subApplCallBackHandler = pSubCallBackHandler;

    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, subTopic, strlen(subTopic), QOS1, iot_subscribe_callback_handler, user_data);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        return rc;
    }
    ESP_LOGI(TAG, "Subscribing... Successful");
    
    return rc;
}

bool AWS_IOT::handle(){
	IoT_Error_t rc = aws_iot_mqtt_yield(&client, 200);
	return SUCCESS == rc;	
}


ClientState AWS_IOT::state(){
	return aws_iot_mqtt_get_client_state(&client);
}

bool AWS_IOT::reconnect(){
	const auto err = aws_iot_mqtt_attempt_reconnect(&client);
	ESP_LOGE(TAG, "Reconnect Error: %d", err);
	return err == SUCCESS || err == NETWORK_RECONNECTED;
}

// void aws_iot_task(void *param) {
	// IoT_Error_t rc = SUCCESS;
    // while(1)
    // {
        // //Max time the yield function will wait for read messages
        // rc = aws_iot_mqtt_yield(&client, 200);
        
        // if(NETWORK_ATTEMPTING_RECONNECT == rc)
        // {
            // // If the client is attempting to reconnect we will skip the rest of the loop.
            // continue;
        // }        
        // vTaskDelay(1000 / portTICK_RATE_MS);
    // }
// }
