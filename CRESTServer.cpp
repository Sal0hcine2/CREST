// Dependencies
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "HttpMessageHandler.h"
#include <curl/curl.h>

CURL *curl;
CURLcode res;

// Configuration properties
#define CREST_VERSION "v1.0.1"
#define POLL_TIME_IN_MILLIS 17
#define ESC_KEY 27
#define CREST_API_URL "/crest/v1/api"

// Constants
#define HTTP_RESPONSE_404 "{\"status\": \"404 Not found, please use the correct URL: " CREST_API_URL "\"}"

// Server variables

// Response generator
static HttpMessageHandler httpMessageHandler = HttpMessageHandler();

// Server request handler method
//httpMessageHandler.handle(nc, hm);


int main()	{

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	// Setup the server
	
	// Print some information on the console
	printf("# CREST - CARS REST API %s\n", CREST_VERSION);
	printf("# (c) 2015 Lars Rosenquist\n\n");
	printf("# Press ESC to terminate\n");
    
	// Keep polling until ESC is hit
	while (true)	{

		if (_kbhit() && _getch() == ESC_KEY)	{
			break;
		}
	}

	// We're done, free up the server and exit
	return 0;
}
