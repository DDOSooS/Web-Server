#pragma once

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <fstream>

class HttpRequestBuilder;
class HttpRequest;
class HttpResponse;
class HttpHandler;

class ClientConnection
{
    private:

        int         fd;                      // Socket file descriptor
        std::string ipAddress;       // Client IP address as string
        uint16_t    port;               // Client port number
        time_t      connectTime;          // When client connected
        time_t      lastActivity;         // Last activity timestamp
        
        HttpRequestBuilder    *builder;
    public:
        ClientConnection(); 
        ClientConnection(int socketFd, const sockaddr_in& clientAddr);
        
        HttpResponse    *http_response;
        HttpRequest      *http_request;
        HttpHandler     *hanler_chain;
        // void            handleRequest();
        // bool            processIncomingData(const char* buffer, size_t length);
        
        
        void            parseRequest(char *buff);
        void            updateActivity();
        bool            isStale(time_t timeoutSec) const ;
    
    };
    
    
        //Constructors
        //Request data
        // RequestData *   http_request;
            // bool            hasCompleteRequest() const;
            
            // // Response handling
            // bool            hasDataToSend() const;
            // const char*     getDataToSend(size_t& length);
            // void            updateBytesSent(size_t bytes);
            // // Response data
            // std::string     responseHeaders; // Prepared response headers
            // std::string     responseBody;    // Response content
            // size_t          bytesSent;            // How many bytes already sent
            
            // // Buffer management
            // std::string sendBuffer;      // Data waiting to be sent
            // // bool keepAlive;             // Connection: keep-alive
