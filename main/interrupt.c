/*
	MQTT Reciever - interrupt.c
	Copyright 2019 Dylan Weber

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
 */
#include "interrupt.h"

static const char *TAG = "int";

void IRAM_ATTR gpio_isr_handler(void *params) {
	uint32_t gpio_num = (uint32_t)params;
	xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

void gpio_event_task(void *params) {
	uint32_t gpio_num;
	while (true) {
		if (xQueueReceive(gpio_event_queue, &gpio_num, portMAX_DELAY)) {
			if (gpio_num == BUTTON_NUM) {
				ESP_LOGI(TAG, "Button pressed.");
			}
		} else {
			ESP_LOGI(TAG, "No interrupt.");
		}
	}
}
