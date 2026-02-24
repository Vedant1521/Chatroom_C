#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 5000

char answer[BUFSIZE] = {0};

void err_check(char* var, void* err) {
    if (err == NULL) {
        fprintf(stdout, "%s\n", answer);
        fprintf(stderr, "%s failed\n", var);
        exit(EXIT_FAILURE);
    }
}

void handle_data(char* buffer) {
    cJSON* json = cJSON_Parse(buffer);
    err_check("json", json);
    cJSON* candidates = cJSON_GetObjectItem(json, "candidates");
    err_check("candidates", candidates);
    cJSON* arr1 = cJSON_GetArrayItem(candidates, 0);
    err_check("arr1", arr1);
    cJSON* content = cJSON_GetObjectItem(arr1, "content");
    err_check("content", content);
    cJSON* parts = cJSON_GetObjectItem(content, "parts");
    err_check("parts", parts);
    cJSON* arr2 = cJSON_GetArrayItem(parts, 0);
    err_check("arr2", arr2);
    cJSON* text = cJSON_GetObjectItem(arr2, "text");
    err_check("text", text);
    strncpy(answer, cJSON_GetStringValue(text), BUFSIZE - 1);
    answer[BUFSIZE - 1] = '\0';
    cJSON_Delete(json);
}

static size_t write_callback(char* contents, size_t itemsize, size_t nitems, void* userp) {
	size_t total_size = itemsize * nitems;
    contents[total_size] = '\0';
    strncpy(answer, contents, BUFSIZE);
    handle_data(contents);
	return total_size;
}

char* response(const char* chat) {
	CURL* curl;
	CURLcode res;
	char* response = calloc(256, sizeof(char));
	const char* api_key = getenv("GEMINI_API_KEY");
	if (api_key == NULL) {
		perror("api key not found\n");
		return NULL;
	}
	const char* url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";
	cJSON* data = cJSON_CreateObject();
	cJSON* contents = cJSON_CreateArray();
	cJSON* content_obj = cJSON_CreateObject();
	cJSON* parts = cJSON_CreateArray();
	cJSON* part_obj = cJSON_CreateObject();
	cJSON_AddStringToObject(part_obj, "text", chat);
	cJSON_AddItemToArray(parts, part_obj);
	cJSON_AddItemToObject(content_obj, "parts", parts);
	cJSON_AddItemToArray(contents, content_obj);
	cJSON_AddItemToObject(data, "contents", contents);
	char* json_data = cJSON_PrintUnformatted(data);
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	char api_header[BUFSIZE];
	snprintf(api_header, sizeof(api_header), "X-goog-api-key: %s", api_key);
	headers = curl_slist_append(headers, api_header);
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "init failed\n");
		return NULL;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
		return NULL;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	cJSON_Delete(data);
	free(json_data);
	return answer;
}
