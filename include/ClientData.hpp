#pragma once

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sstream>
#include <fstream>
struct RequestData
{
    std::string                                     method;
    std::string                                     request_path;
    std::string                                     http_version;
    std::unordered_map <std::string , std::string>  headers;
    std::ifstream                                   ressources;
    std::string                                     full_path;
    size_t                                          content_length;
    std::streamsize                                 response_size;
    std::streamsize                                 remaine_bytes;
    bool                                            keep_alive;
    bool                                            crlf_flag;
    bool                                            request_line;
    bool                                            request_status;
	std::string										request_body;
    RequestData() ;
    bool											findHeader(std::string );
	bool											findHeaderValue(std::string , std::string);
	std::string										getHeader(std::string ) const ;
	std::unordered_map <std::string , std::string > getAllHeaders() const; 
    void                                            reset();
};

struct ClientData
{
    int         fd;                      // Socket file descriptor
    std::string ipAddress;       // Client IP address as string
    uint16_t    port;               // Client port number
    time_t      connectTime;          // When client connected
    time_t      lastActivity;         // Last activity timestamp
    
    //Constructors
    ClientData(); 
    ClientData(int socketFd, const sockaddr_in& clientAddr);

    //Request data
    RequestData *   http_request;
    void        parseRequest(char *buff);
    // Response data
    std::string     responseHeaders; // Prepared response headers
    std::string     responseBody;    // Response content
    size_t          bytesSent;            // How many bytes already sent
    
    // Buffer management
    std::string sendBuffer;      // Data waiting to be sent
    // bool keepAlive;             // Connection: keep-alive

    void updateActivity();
    bool isStale(time_t timeoutSec) const ;
};

