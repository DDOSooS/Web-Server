#pragma once 

#include <string>
#include "../WebServer.hpp"
#include "../ClientConnection.hpp"
#include "./HttpRequest.hpp"

class WebServer;
class HttpRequest;

class RequestHandler
{
    private:
        RequestHandler	*_nextHandler;
	
	public:
		RequestHandler();
		~RequestHandler();
		virtual void 		HandleRequest(HttpRequest *request);
		virtual	bool 		CanHandle(std::string method)=0;
		virtual void 		ProccessRequest(HttpRequest *request)=0;
		RequestHandler *	SetNext(RequestHandler *);
};

