/***************************************************************************************************
                                    ExploreEmbedded Copyright Notice    
****************************************************************************************************
 * File:   AWS_IOT.h
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
 
 
#ifndef _HORNBILL_AWS_IOT_LIB_H_
#define _HORNBILL_AWS_IOT_LIB_H_

#include <stdint.h>

#include "aws_iot_mqtt_client.h"

typedef void (*pSubCallBackHandler_t)(const char *topicName,const int topicNameLen, const char *payLoad, const int payloadLen, void* user_data);

class AWS_IOT{    
    private:
		AWS_IoT_Client client;
    public:
		int connect(const char *hostAddress, const char *clientID,const char* will_topic, const uint16_t will_topic_len);
		int disconnect();
		int publish(const char *pubtopic, const char *pubPayLoad, const bool isRetained = false);
		int subscribe(const char *subTopic, pSubCallBackHandler_t pSubCallBackHandler, void* user_data);
		bool handle();
};


#endif

