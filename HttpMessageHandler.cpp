// Dependencies
#include <windows.h>
#include "WinNT.h"
#include "memoryapi.h"
#include "MessageHandler.h"
#include "SharedMemoryRenderer.h"
#include "Utils.h"
#include "sharedmemory.h"
#include <sstream>
#include <curl/curl.h>
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
#include <iostream>

// Constants
#define MAP_OBJECT_NAME "$pcars$"
#define HTTP_RESPONSE_503 "{\"status\": \"503 Service unavailable, is Project CARS running and is Shared Memory enabled?\"}"
#define HTTP_RESPONSE_409 "{\"status\": \"409 Conflict, are CREST and Project CARS both at the latest version?\"}"
#define GZIP_THRESHOLD 128

static SharedMemoryRenderer sharedMemoryRenderer = SharedMemoryRenderer();

MessageHandler::MessageHandler(){};

CURL *curl;
CURLcode res;
std::string data;
HANDLE fileHandle;
const SharedMemory* sharedData;


// Outputs an HTTP 200 on the supplied connection for an OPTIONS request
void sendOptions()    {
    // Send HTTP 200
    printf("HTTP/1.1 200 Ok\r\n"
              "Access-Control-Allow-Origin: *\r\n"
              "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
              "Access-Control-Max-Age: 86400\r\n"
              "Content-Length: 0\r\n");
    
}

// Outputs an HTTP 503 on the supplied connection
void sendServiceUnavailable()    {
	// Send HTTP 503
	printf("HTTP/1.1 503 Service unavailable\r\n"
			"Content-Type: application/json\r\n"
			"Cache-Control: no-cache\r\n"
			"Access-Control-Allow-Origin: *\r\n"
			"Content-Length: %d\r\n\r\n%s",
			(int)strlen(HTTP_RESPONSE_503), HTTP_RESPONSE_503);

}


// Returns true if the response to the given HTTP message should
// be gzipped, based on the value of the Accept-Encoding header
// and the size of the uncompressed response
bool shouldGzipResponse(int responseLength)	{
	return false; // Utils::contains(FossaUtils::getHeaderValue("Accept-Encoding", hm), "gzip") && responseLength > GZIP_THRESHOLD;
}

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{ //callback must have this declaration
	//buf is a pointer to the data that curl has for us
	//size*nmemb is the size of the buffer

	for (int c = 0; c<size*nmemb; c++)
	{
		data.push_back(buf[c]);
	}
	return size*nmemb; //tell curl how many bytes we handled
}

std::string curlResponse(std::string url, std::string JSON){

	// Instantiate CURL object
	curl = curl_easy_init();

	if (curl) {
		// Set the URL
		curl_easy_setopt(curl, CURLOPT_URL, url.data());

		// Do some dodgey wrapping of the JSON into a DATA field.
		std::string postField = "data=" + JSON;

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField.data());
		//curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, true);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		}
		/* always cleanup */
		curl_easy_cleanup(curl);

		return data;
	}

	return "";
}

// Retrieves the race_id from the JSON response
std::string getRaceID(std::string data){

	// Parse the JSON into a document
	rapidjson::Document response;
	response.Parse(data.data());

	// Get the value from the document
	rapidjson::Value& raceID_value = response["race_id"];

	// Get the string of the raceID_value
	std::string race_id = raceID_value.GetString();


	return race_id;
}

// Renders the response
void renderResponse(const SharedMemory* sharedData)  {
	std::string response;

	//Register Race Details
	response = sharedMemoryRenderer.render(sharedData, "eventInformation", "");
	//printf(response.data());

	// Post to Server and get the race_id
	//std::string race_id = getRaceID(curlResponse("http://pcars.garland.io/api/5555/race", response));
	std::string race_id = "554632eb585022e92b302698";
	//printf(race_id.data());

	// Now add some participants
	response = sharedMemoryRenderer.render(sharedData, "participants", race_id);
	printf(response.data());
	//std::string a = curlResponse("http://pcars.garland.io/api/5555/participants", response);
	//printf(a.data());
	//printf(response.data());
}

// Processes the shared memory
void processSharedMemoryData(const SharedMemory* sharedData)   {
	// Ensure we're sync'd to the correct data version
	if (sharedData->mVersion != SHARED_MEMORY_VERSION)	{
		// build conflict response
		printf("Data version mismatch, please make sure that your pCARS version matches your CREST version\n");
	}else{
		renderResponse(sharedData);
	}

}

// Processes the memory mapped file
void processFile(HANDLE fileHandle)    {

	sharedData = (SharedMemory*)MapViewOfFile(fileHandle, PAGE_READONLY, 0, 0, sizeof(SharedMemory));

	if (sharedData == NULL)	{
		// File found, but could not be mapped to shared memory data
		sendServiceUnavailable();
	}
	else{
		// Process file
		processSharedMemoryData(sharedData);
		// Unmap file
		UnmapViewOfFile(sharedData);
	}

}

void handleGet()    {
    // Open the memory mapped file
    fileHandle = OpenFileMappingA(PAGE_READONLY, FALSE, MAP_OBJECT_NAME);
    
    if (fileHandle == NULL)	{
        // File is not available, build service unavailable response
        sendServiceUnavailable();
    }
    else{
        // File is available, process the file
        processFile(fileHandle);
        // Close the file
        CloseHandle(fileHandle);
    }
}

void MessageHandler::handle()	{
	handleGet();
}

SharedMemory* getSharedData(){
	HANDLE fHandle = OpenFileMappingA(PAGE_READONLY, FALSE, MAP_OBJECT_NAME);

	return (SharedMemory*)MapViewOfFile(fHandle, PAGE_READONLY, 0, 0, sizeof(SharedMemory));
}

int MessageHandler::getRaceState(){
	SharedMemory* sharedData = getSharedData();
	
	if (sharedData == NULL){
		return -1;
	}
	return getSharedData()->mRaceState;
}

int MessageHandler::getCompletedLaps(){
	SharedMemory* data =  getSharedData();
	
	return getSharedData()->mParticipantInfo->mLapsCompleted;
}


std::string MessageHandler::createRace(){

	std::string response = sharedMemoryRenderer.render(getSharedData(), "eventInformation", "");

	// Post to Server and get the race_id
	return getRaceID(curlResponse("http://pcars.garland.io/api/5555/race", response));

}

std::string MessageHandler::addParticipants(std::string race_id){
	std::string response = sharedMemoryRenderer.render(getSharedData(), "participants", race_id);

	return curlResponse("http://pcars.garland.io/api/5555/participants", response);
}

std::string MessageHandler::updateParticipants(std::string race_id){
	std::string response = sharedMemoryRenderer.render(getSharedData(), "participants", race_id);

	return curlResponse("http://pcars.garland.io/api/5555/participants", response);
}