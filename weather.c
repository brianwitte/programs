#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char *data;
    size_t size;
} response_t;

// Callback function to write response data
size_t write_callback(void *contents, size_t size, size_t nmemb, response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->data, response->size + realsize + 1);
    if (!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    
    return realsize;
}

int main() {
    CURL *curl;
    CURLcode res;
    response_t response = {0};
    
    printf("Weather App - Fetching Austin, TX Weather\n");
    printf("==========================================\n\n");
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }
    
    // Allocate initial memory for response
    response.data = malloc(1);
    response.size = 0;
    
    // Set URL - Austin, TX coordinates with current weather
    const char *url = "https://api.open-meteo.com/v1/forecast?"
                     "latitude=30.2672&longitude=-97.7431&"
                     "current_weather=true&"
                     "hourly=temperature_2m,precipitation&"
                     "timezone=America/Chicago";
    
    // Configure curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "weather-app/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Perform the request
    printf("Fetching weather data...\n");
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }
    
    // Get HTTP response code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    printf("HTTP Response Code: %ld\n", response_code);
    printf("Response Size: %zu bytes\n\n", response.size);
    
    if (response_code == 200 && response.data) {
        printf("Weather Data (JSON):\n");
        printf("====================\n");
        printf("%s\n", response.data);
    } else {
        printf("Failed to fetch weather data\n");
    }
    
    // Cleanup
    free(response.data);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    printf("\n==========================================\n");
    printf("Weather fetch complete!\n");
    printf("Compile with: gcc weather.c -o weather -lcurl\n");
    
    return 0;
}
