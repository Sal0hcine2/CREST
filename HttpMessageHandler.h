class HttpMessageHandler	{
public:
	// No-args constructor
	HttpMessageHandler();

	void handle(struct ns_connection *nc, struct http_message *hm);
};

