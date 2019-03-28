#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Arduino.h"
#include "esp_task_wdt.h"

#if CONFIG_AUTOSTART_ARDUINO

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

void loopTask(void *pvParameters)
{
    setup();
    for(;;) {
        loop();
		esp_task_wdt_reset();
    }
}

extern "C" void app_main()
{	
    initArduino();
	esp_task_wdt_init(60, true);
	TaskHandle_t task_handler = nullptr;
    xTaskCreatePinnedToCore(loopTask, "loopTask", 32*1024, NULL, 1, &task_handler, ARDUINO_RUNNING_CORE);
	esp_task_wdt_add(task_handler);
}

#endif
