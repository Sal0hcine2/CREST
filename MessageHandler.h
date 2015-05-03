#include <string>

class MessageHandler	{

public:
	// No-args constructor
	MessageHandler();

	void handle();

	int getRaceState();
	int getCompletedLaps();

	std::string createRace();
	std::string addParticipants(std::string race_id);
	std::string updateParticipants(std::string race_id);
};

