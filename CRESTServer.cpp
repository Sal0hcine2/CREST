// Dependencies
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <thread>
#include <chrono>
#include "MessageHandler.h"
#include "SharedMemoryRenderer.h"


// Configuration properties
#define CREST_VERSION "v0.1"
#define POLL_TIME_IN_MILLIS 17
#define ESC_KEY 27
#define CREST_API_URL "/crest/v1/api"

// Constants
#define HTTP_RESPONSE_404 "{\"status\": \"404 Not found, please use the correct URL: " CREST_API_URL "\"}"

// Server variables

// Response generator
static MessageHandler messageHandler = MessageHandler();
static SharedMemoryRenderer sharedMemoryRenderer = SharedMemoryRenderer();
// Server request handler method
//httpMessageHandler.handle(nc, hm);
std::string race_id = "";

int prevRaceState = 0;
int completedLaps = 0;

int main()	{
	// Open Message Handler
	MessageHandler messageHandler = MessageHandler();
	
	// Print some information on the console
	printf("# TrakR %s - Based on CREST - CARS REST API\n", CREST_VERSION);
	printf("# (c) 2015 Lars Rosenquist, Nick Garland\n\n");
	printf("# Press ESC to terminate\n");	
	
	// Keep polling until ESC is hit
	while (true)	{	
		// Check for Race State change
		if (prevRaceState != messageHandler.getRaceState()){
			prevRaceState = messageHandler.getRaceState();
			printf("\n\n Race State Changed: %d", messageHandler.getRaceState());
		}

		// Not started on the grid 
		if (messageHandler.getRaceState() == 1 && race_id.empty()){
			race_id = messageHandler.createRace();

			printf("\n\n Race Created: %s", race_id.data());

			std::string parts = messageHandler.addParticipants(race_id);
			printf("\n\n %s", parts.data());

			printf("\n\n Laps Completed: %d", messageHandler.getCompletedLaps());
		}

		// 2 == RACESTATE_RACING
		if (messageHandler.getRaceState() == 2){
			// Check if we have completed a lap yet
			if (completedLaps < messageHandler.getCompletedLaps()){
				//Update complete laps
				completedLaps = messageHandler.getCompletedLaps();
				printf("\n\n Laps Completed: %d", completedLaps);

				//Now ping that server!
				printf("\n\n Updated Participants: %s", messageHandler.updateParticipants(race_id));
			}
		}

		// Finish up!
		if (messageHandler.getRaceState() == 3 || messageHandler.getRaceState() == 0){
			race_id = "";
		}

		if (_kbhit() && _getch() == ESC_KEY)	{
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// We're done, free up the server and exit
	return 0;
}
