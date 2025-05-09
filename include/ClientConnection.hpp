#pragma once

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include "../include/request/HttpRequestBuilder.hpp"

class HttpRequestBuilder;
class HttpRequest;
class HttpResponse;
class HttpHandler;


#define REQUSET_LINE_BUFFER 8000

class ClientConnection
{
    private:
        int                     fd;                     // Socket file descriptor
        std::string             ipAddress;              // Client IP address as string
        uint16_t                port;                   // Client port number
        time_t                  connectTime;            // When client connected
        time_t                  lastActivity;           // Last activity timestamp 
        HttpRequestBuilder      *builder;
    
    public:
        ClientConnection(); 
        ClientConnection(int socketFd, const sockaddr_in& clientAddr);

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
    
