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
		virtual ~RequestHandler();
		virtual void 		HandleRequest(HttpRequest *request, const ServerConfig &serverConfig);
		virtual	bool 		CanHandle(std::string method)=0;
		virtual void 		ProccessRequest(HttpRequest *request,const ServerConfig &serverConfig)=0;
		RequestHandler *	SetNext(RequestHandler *);
		RequestHandler *	GetNext();

};

