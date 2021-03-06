/*
	MQTT Reciever - init.c
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
#include "init.h"

static const char *TAG = "init";

esp_err_t app_init() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	esp_vfs_spiffs_conf_t config = {.base_path = "/spiffs",
									.partition_label = "storage",
									.max_files = 5,
									.format_if_mount_failed = true};

	ret = esp_vfs_spiffs_register(&config);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem.");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS patition.");
		} else {
			ESP_LOGE(TAG, "Other SPIFFS error: %s", esp_err_to_name(ret));
		}
		return ESP_FAIL;
	}

	FILE *fp = fopen("/spiffs/example.txt", "r");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open example file.");
		return ESP_FAIL;
	}

	char buf[64];
	memset(buf, 0, sizeof(buf));
	fread(buf, 1, sizeof(buf) - 1, fp);
	fclose(fp);

	ESP_LOGI(TAG, "Read from example.txt: %s", buf);

	tcpip_adapter_init();
	wifi_new_config = false;
	gpio_event_queue = NULL;
	button_semaphore = xSemaphoreCreateBinary();

	xTaskCreate(setup_status_led, "status_led", 4096, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(debounce_and_retrieve, "debounce_and_retrieve", 4096, NULL, tskIDLE_PRIORITY, NULL);
	return ESP_OK;
}

void configure_clear_interrupt(esp_mqtt_client_handle_t *mqtt_client) {
	ESP_LOGI(TAG, "useful number: %p", mqtt_client);

	gpio_config_t io_config;
	io_config.intr_type = GPIO_INTR_POSEDGE;
	io_config.mode = GPIO_MODE_INPUT;
	io_config.pull_up_en = GPIO_PULLUP_DISABLE;
	io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_config.pin_bit_mask = BUTTON_MSK;

	gpio_config(&io_config);

	if (gpio_event_queue == NULL) {
		gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
		xTaskCreate(gpio_event_task, "gpio_event_task", 4096, NULL, 1, NULL);

		gpio_install_isr_service(ALLOC_FLAGS);
	}

	struct interrupt_info *button_int_info = malloc(sizeof(*button_int_info));
	button_int_info->button = BUTTON_NUM;
	button_int_info->mqtt_handle = mqtt_client;

	gpio_isr_handler_add(BUTTON_NUM, gpio_isr_handler, (void *)button_int_info);
}

void configure_ext_interrupt(esp_mqtt_client_handle_t *mqtt_client) {
	gpio_config_t io_config;
	io_config.intr_type = GPIO_INTR_ANYEDGE;
	io_config.mode = GPIO_MODE_INPUT;
	io_config.pull_up_en = GPIO_PULLUP_ENABLE;
	io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_config.pin_bit_mask = EXT_MSK;

	gpio_config(&io_config);

	if (gpio_event_queue == NULL) {
		gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
		xTaskCreate(gpio_event_task, "gpio_event_task", 4096, NULL, 1, NULL);

		gpio_install_isr_service(ALLOC_FLAGS);
	}

	struct interrupt_info *ext_int_info = malloc(sizeof(*ext_int_info));
	ext_int_info->button = EXT_NUM;
	ext_int_info->mqtt_handle = mqtt_client;

	gpio_isr_handler_add(EXT_NUM, gpio_isr_handler, (void *)ext_int_info);
}

void setup_status_led() {
	gpio_pad_select_gpio(STATUS_NUM);
	gpio_set_direction(STATUS_NUM, GPIO_MODE_OUTPUT);
	gpio_set_level(STATUS_NUM, true);
	while (true) {
		while (mqtt_connected == false) {
			gpio_set_level(STATUS_NUM, false);
			vTaskDelay(300 / portTICK_PERIOD_MS);
			gpio_set_level(STATUS_NUM, true);
			vTaskDelay(300 / portTICK_PERIOD_MS);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void mqtt_routine(esp_mqtt_client_handle_t *mqtt_client) {
	ESP_LOGI(TAG, "useful number: %p", mqtt_client);
	char mqtt_hostname[255] = {'\0'};
	uint16_t port = 0;
	init_mdns(mqtt_hostname, &port);
	if (mqtt_hostname != NULL && port != 0) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
		start_mqtt(mqtt_hostname, port, mqtt_client);
	} else {
		ESP_LOGI(TAG, "Failed discovery.");
	}
}

void setup_routine() {
	char **network_list;
	esp_err_t ret = wifi_scan(&network_list);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to scan Wi-Fi.");
		return;
	}
	ret = wifi_startap();
	if (ret == ESP_OK) {
		start_httpserver(network_list);
	}
}

void save_recovery() {
	FILE *fp = fopen("/spiffs/mqtt_roll.dat", "w");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open mqtt file.");
		return;
	}

	fwrite(&mqtt_rolling_code, sizeof(mqtt_rolling_code), 1, fp);
	fclose(fp);
}

void attempt_recovery() {
	FILE *fp = fopen("/spiffs/mqtt_roll.dat", "r");
	if (fp == NULL) {
		ESP_LOGI(TAG, "Data mqtt file not found.");
		return;
	}

	fread(&mqtt_rolling_code, sizeof(mqtt_rolling_code), 1, fp);
	fclose(fp);
	remove("/spiffs/mqtt_roll.dat");
	ESP_LOGI(TAG, "Recovered rolling code: %d", mqtt_rolling_code);
}
