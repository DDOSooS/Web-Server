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
#define STREAM_CHUNK_SIZE 32768    

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
    int                     temp_upload_fd;
    bool                    should_close;


    ClientConnection(); 
    ClientConnection(int socketFd, const sockaddr_in& clientAddr);
    ~ClientConnection();
    int GetFd() const;
    void GenerateRequest(int fd);
    void ProcessRequest(int fd);
    void RespondToClient(int fd);
    void parseRequest(char *buff);
    void updateActivity();
    bool isStale(time_t timeoutSec) const;
    void setServerConfig(const ServerConfig& config);
    ServerConfig getServerConfig() const;
};