#pragma once

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "./request/HttpRequestBuilder.hpp"
#include "./request/HttpException.hpp"
#include "./response/HttpResponse.hpp"
#include "./request/RequestHandler.hpp"

class HttpRequestBuilder;
class HttpRequest;
class HttpResponse;
class WebServer;
class RequestHandler;

#define REQUSET_LINE_BUFFER 8192 
#define MAX_MEMORY_UPLOAD 512000  // 512KB threshold
#define STREAM_CHUNK_SIZE 32768    // 32KB chunks

class ClientConnection
{
public:
    WebServer*              _server;
    int                     fd;
    std::string             ipAddress;
    uint16_t                port;
    time_t                  connectTime;
    time_t                  lastActivity;
    HttpRequestBuilder      *builder;
    HttpResponse            *http_response;
    HttpRequest             *http_request;
    ServerConfig            server_config;
    
    static int              redirect_counter;
    bool                    should_close;
    int                     temp_upload_fd;

    // Constructors and destructor
    ClientConnection(); 
    ClientConnection(int socketFd, const sockaddr_in& clientAddr);
    ~ClientConnection();
    
    // Getter methods
    int GetFd() const;
    
    // Main request handling methods
    void GenerateRequest(int fd);
    void ProcessRequest(int fd);
    void RespondToClient(int fd);
    void parseRequest(char *buff);
    
    // Activity and connection management
    void updateActivity();
    bool isStale(time_t timeoutSec) const;

    // Streaming upload methods

    void setServerConfig(const ServerConfig& config);
    ServerConfig getServerConfig() const;
};