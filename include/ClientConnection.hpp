#pragma once

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include "./request/HttpRequestBuilder.hpp"
#include "./request/HttpException.hpp"
#include "./response/HttpResponse.hpp"
#include "./request/RequestHandler.hpp"
// #include "./request/Get.hpp"

class HttpRequestBuilder;
class HttpRequest;
class HttpResponse;
class HttpHandler;
class WebServer;
class RequestHandler;
// class Get;


#define REQUSET_LINE_BUFFER 8000

class ClientConnection
{
    public:
        WebServer*              _server;                // Pointer to the WebServer instance
        int                     fd;                     // Socket file descriptor
        std::string             ipAddress;              // Client IP address as string
        uint16_t                port;                   // Client port number
        time_t                  connectTime;            // When client connected
        time_t                  lastActivity;           // Last activity timestamp 
        HttpRequestBuilder      *builder;
    
    public:
        ClientConnection(); 
        ClientConnection(int socketFd, const sockaddr_in& clientAddr);
        ~ClientConnection();
        HttpResponse     *http_response;
        HttpRequest      *http_request;
        HttpHandler      *handler_chain;
        
        void            GenerateRequest(int fd);
        void            ProcessRequest(int fd);
        void            RespondToClient(int fd);
        void            parseRequest(char *buff);
        void            updateActivity();
        bool            isStale(time_t timeoutSec) const ;
};
    
