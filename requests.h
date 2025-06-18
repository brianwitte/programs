#ifndef REQUESTS_H
#define REQUESTS_H

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *data;
    size_t size;
} requests_response_t;

typedef struct {
    char **headers;
    size_t count;
} requests_headers_t;

// Response callback for writing data
static size_t requests_write_callback(void *contents, size_t size, size_t nmemb, requests_response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->data, response->size + realsize + 1);
    if (!ptr) return 0;
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    
    return realsize;
}

// Initialize requests library
static int requests_init(void) {
    return curl_global_init(CURL_GLOBAL_DEFAULT);
}

// Cleanup requests library
static void requests_cleanup(void) {
    curl_global_cleanup();
}

// Create response object
static requests_response_t* requests_response_create(void) {
    requests_response_t *response = malloc(sizeof(requests_response_t));
    if (!response) return NULL;
    response->data = malloc(1);
    response->size = 0;
    return response;
}

// Free response object
static void requests_response_free(requests_response_t *response) {
    if (response) {
        free(response->data);
        free(response);
    }
}

// Create headers object
static requests_headers_t* requests_headers_create(void) {
    requests_headers_t *headers = malloc(sizeof(requests_headers_t));
    if (!headers) return NULL;
    headers->headers = NULL;
    headers->count = 0;
    return headers;
}

// Add header
static void requests_headers_add(requests_headers_t *headers, const char *header) {
    if (!headers) return;
    headers->headers = realloc(headers->headers, (headers->count + 1) * sizeof(char*));
    headers->headers[headers->count] = strdup(header);
    headers->count++;
}

// Free headers object
static void requests_headers_free(requests_headers_t *headers) {
    if (headers) {
        for (size_t i = 0; i < headers->count; i++) {
            free(headers->headers[i]);
        }
        free(headers->headers);
        free(headers);
    }
}

// Generic request function
static requests_response_t* requests_request(const char *method, const char *url, 
                                           const char *data, requests_headers_t *headers) {
    CURL *curl;
    CURLcode res;
    requests_response_t *response = requests_response_create();
    if (!response) return NULL;
    
    curl = curl_easy_init();
    if (!curl) {
        requests_response_free(response);
        return NULL;
    }
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, requests_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    
    // Set method
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (data) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (data) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "PATCH") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (data) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    }
    
    // Set headers
    struct curl_slist *curl_headers = NULL;
    if (headers) {
        for (size_t i = 0; i < headers->count; i++) {
            curl_headers = curl_slist_append(curl_headers, headers->headers[i]);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    }
    
    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Perform request
    res = curl_easy_perform(curl);
    
    // Cleanup
    if (curl_headers) curl_slist_free_all(curl_headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        requests_response_free(response);
        return NULL;
    }
    
    return response;
}

// Convenience functions
static requests_response_t* requests_get(const char *url) {
    return requests_request("GET", url, NULL, NULL);
}

static requests_response_t* requests_get_headers(const char *url, requests_headers_t *headers) {
    return requests_request("GET", url, NULL, headers);
}

static requests_response_t* requests_post(const char *url, const char *data) {
    return requests_request("POST", url, data, NULL);
}

static requests_response_t* requests_post_headers(const char *url, const char *data, requests_headers_t *headers) {
    return requests_request("POST", url, data, headers);
}

static requests_response_t* requests_put(const char *url, const char *data) {
    return requests_request("PUT", url, data, NULL);
}

static requests_response_t* requests_delete(const char *url) {
    return requests_request("DELETE", url, NULL, NULL);
}

static requests_response_t* requests_patch(const char *url, const char *data) {
    return requests_request("PATCH", url, data, NULL);
}

// JSON convenience function
static requests_response_t* requests_post_json(const char *url, const char *json_data) {
    requests_headers_t *headers = requests_headers_create();
    requests_headers_add(headers, "Content-Type: application/json");
    requests_response_t *response = requests_post_headers(url, json_data, headers);
    requests_headers_free(headers);
    return response;
}

#ifdef __cplusplus
}
#endif

#endif // REQUESTS_H

/*
Usage example:

#include "requests.h"

int main() {
    requests_init();
    
    // Simple GET request
    requests_response_t *response = requests_get("https://httpbin.org/get");
    if (response) {
        printf("Response: %s\n", response->data);
        requests_response_free(response);
    }
    
    // POST with JSON
    response = requests_post_json("https://httpbin.org/post", "{\"key\":\"value\"}");
    if (response) {
        printf("Response: %s\n", response->data);
        requests_response_free(response);
    }
    
    // Custom headers
    requests_headers_t *headers = requests_headers_create();
    requests_headers_add(headers, "Authorization: Bearer token123");
    requests_headers_add(headers, "User-Agent: MyApp/1.0");
    
    response = requests_get_headers("https://httpbin.org/headers", headers);
    if (response) {
        printf("Response: %s\n", response->data);
        requests_response_free(response);
    }
    requests_headers_free(headers);
    
    requests_cleanup();
    return 0;
}

Compile with: gcc -o example example.c -lcurl
*/
