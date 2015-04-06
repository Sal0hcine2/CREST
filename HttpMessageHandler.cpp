// Dependencies
#include <windows.h>
#include "WinNT.h"
#include "memoryapi.h"
#include "HttpMessageHandler.h"
#include "SharedMemoryRenderer.h"
#include "Utils.h"
#include "sharedmemory.h"
#include <sstream>


// Constants
#define MAP_OBJECT_NAME "$pcars$"
#define HTTP_RESPONSE_503 "{\"status\": \"503 Service unavailable, is Project CARS running and is Shared Memory enabled?\"}"
#define HTTP_RESPONSE_409 "{\"status\": \"409 Conflict, are CREST and Project CARS both at the latest version?\"}"
#define GZIP_THRESHOLD 128

static SharedMemoryRenderer sharedMemoryRenderer = SharedMemoryRenderer();

HttpMessageHandler::HttpMessageHandler(){};

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
bool shouldGzipResponse(struct http_message *hm, int responseLength)	{
	return false; // Utils::contains(FossaUtils::getHeaderValue("Accept-Encoding", hm), "gzip") && responseLength > GZIP_THRESHOLD;
}

// Renders the response
void renderResponse(const SharedMemory* sharedData, struct http_message *hm)  {

	std::string responseJson = sharedMemoryRenderer.render(sharedData, "participants");
	std::string response;

	bool gzipResponse = shouldGzipResponse(hm, responseJson.size());

	if (gzipResponse)	{
		response = Utils::gzipString(responseJson);
	}else{
		response = responseJson;
	}

	// build HTTP OK response with JSON response body
	printf("HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Cache-Control: no-cache\r\n"
        "Access-Control-Allow-Origin: *\r\n");
	if (gzipResponse)	{
		printf("Content-Encoding: gzip\r\n");
	}
	printf("Content-Length: %d\r\n\r\n",
		(int)response.size());
	response.data(); response.size();

}

// Processes the shared memory
void processSharedMemoryData(const SharedMemory* sharedData, struct http_message *hm)   {
	// Ensure we're sync'd to the correct data version
	if (sharedData->mVersion != SHARED_MEMORY_VERSION)	{
		// build conflict response
		printf("Data version mismatch, please make sure that your pCARS version matches your CREST version\n");
	}else{
		renderResponse(sharedData, hm);
	}

}

// Processes the memory mapped file
void processFile(HANDLE fileHandle, struct http_message *hm)    {

	const SharedMemory* sharedData = (SharedMemory*)MapViewOfFile(fileHandle, PAGE_READONLY, 0, 0, sizeof(SharedMemory));

	if (sharedData == NULL)	{
		// File found, but could not be mapped to shared memory data
		sendServiceUnavailable();
	}
	else{
		// Process file
		processSharedMemoryData(sharedData, hm);
		// Unmap file
		UnmapViewOfFile(sharedData);
	}

}

void handleGet(struct http_message *hm)    {
    // Open the memory mapped file
    HANDLE fileHandle = OpenFileMappingA(PAGE_READONLY, FALSE, MAP_OBJECT_NAME);
    
    if (fileHandle == NULL)	{
        // File is not available, build service unavailable response
        sendServiceUnavailable();
    }
    else{
        // File is available, process the file
        processFile(fileHandle, hm);
        // Close the file
        CloseHandle(fileHandle);
    }
}