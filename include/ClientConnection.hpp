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
class HttpHandler;
class WebServer;
class RequestHandler;

#define REQUSET_LINE_BUFFER 8000
#define MAX_MEMORY_UPLOAD 1048576  // 1MB threshold
#define STREAM_CHUNK_SIZE 32768    // 32KB chunks

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
    HttpResponse            *http_response;
    HttpRequest             *http_request;
    HttpHandler             *handler_chain;
    
    // Streaming upload members (AFTER handler_chain to match initialization order)
    bool                    is_streaming_upload;
    size_t                  total_content_length;
    size_t                  bytes_received_so_far;
    std::string             accumulated_body;
    int                     temp_upload_fd;
    std::string             temp_upload_path;
    static int                     redirect_counter;
    bool                    should_close;

    // Constructors and destructor
    ClientConnection(); 
    ClientConnection(int socketFd, const sockaddr_in& clientAddr);
    ~ClientConnection();
    
    // Main methods
    void            GenerateRequest(int fd);
    void            ProcessRequest(int fd);
    void            RespondToClient(int fd);
    void            parseRequest(char *buff);
    void            updateActivity();
    bool            isStale(time_t timeoutSec) const;

    // Streaming upload methods
    bool            isStreamingUpload() const { return is_streaming_upload; }
    void            initializeStreaming(size_t content_length);
    bool            continueStreamingRead(int fd);
    void            finalizeStreaming();
};
