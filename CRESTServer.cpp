// Dependencies
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "HttpMessageHandler.h"

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
