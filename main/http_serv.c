/*	Dylan Weber
 *	Doorbell Reciever
 * 	5/21/2019
 */
#include "http_serv.h"

static const char *TAG = "http";

httpd_handle_t start_httpserver() {
	httpd_handle_t server = NULL;
	httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&server, &server_config) == ESP_OK) {
		httpd_register_uri_handler(server, &index_uri);
		httpd_register_uri_handler(server, &post_uri);
	}

	return server;
}

static esp_err_t index_get_handler(httpd_req_t *req) {
	char *buffer;
	size_t buffer_len;

	httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

	FILE *fp = fopen("/spiffs/index.html", "r");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open index file.");
		return ESP_FAIL;
	}

	fseek(fp, 0, SEEK_END);
	buffer_len = ftell(fp);
	buffer = malloc(buffer_len * sizeof(*buffer));

	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, buffer_len, fp);
	fclose(fp);

	httpd_resp_send(req, buffer, buffer_len);
	return ESP_OK;
}

static esp_err_t submit_post_handler(httpd_req_t *req) {
	char *buffer;
	size_t buffer_len;

	httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

	FILE *fp = fopen("/spiffs/saved.html", "r");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open index file.");
		return ESP_FAIL;
	}

	fseek(fp, 0, SEEK_END);
	buffer_len = ftell(fp);
	buffer = malloc(buffer_len * sizeof(*buffer));

	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, buffer_len, fp);
	fclose(fp);

	httpd_resp_send(req, buffer, buffer_len);
	return ESP_OK;
}
