/*
	MQTT Reciever - http_serv.h
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
#ifndef HTTP_SERV_H
#define HTTP_SERV_H

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "init.h"
#include "wifi.h"

#include <esp_http_server.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/** Starts HTTP server for network setup.
 *
 * Params:
 * Network list - a list of network names from wifi_scan(char ***)
 */
httpd_handle_t start_httpserver(char **);

/** Stops the HTTP server for network setup.
 *
 * Params:
 * HTTPd handle - handle of server returned from start_httpserver(char **)
 */
void stop_httpserver(httpd_handle_t);

/// Handler to render the index.html page
esp_err_t index_get_handler(httpd_req_t *);
/// Handler to render the advanced.html page
esp_err_t advanced_get_handler(httpd_req_t *);
/// Handler to recieve POST data and save the WiFi information
esp_err_t submit_post_handler(httpd_req_t *);
/// Handler to send the favicon file
esp_err_t favicon_get_handler(httpd_req_t *);

/// Delayed restart caused by network setup
void delayed_restart(void *);
/// Wi-Fi connect attempt caused by network setup
void try_network_setup(void *);
/// Decodes URL-encoded strings
char *urldecode(char *);
/// String copy function with forced NULL termination
void cleancpy(char *, char *, int);

// clang-format off
static const httpd_uri_t index_uri = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = index_get_handler,
	.user_ctx = NULL
};

static const httpd_uri_t adv_uri = {
	.uri = "/advanced.html",
	.method = HTTP_GET,
	.handler = advanced_get_handler,
	.user_ctx = NULL
};

static const httpd_uri_t post_uri = {
	.uri = "/submit",
	.method = HTTP_POST,
	.handler = submit_post_handler,
	.user_ctx = NULL
};

static const httpd_uri_t favicon_uri = {
	.uri = "/favicon.ico",
	.method = HTTP_GET,
	.handler = favicon_get_handler,
	.user_ctx = NULL
};

#endif // HTTP_SERV_H
