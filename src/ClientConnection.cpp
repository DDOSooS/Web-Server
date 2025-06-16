#include "../include/ClientConnection.hpp"
#include "../include/WebServer.hpp"
#include "../include/request/Get.hpp"
#include "../include/request/Post.hpp"
#include "../include/request/CgiHandler.hpp"
#include "../include/request/Delete.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include <errno.h>
#include <set>

int ClientConnection::redirect_counter = 0;

ClientConnection::ClientConnection(): fd(-1), ipAddress(""), port(0), connectTime(0), lastActivity(0)
            , builder(NULL), http_response(NULL), http_request(NULL), handler_chain(NULL)
            , is_streaming_upload(false), total_content_length(0), bytes_received_so_far(0), temp_upload_fd(-1), should_close(false)
{
    this->_server = NULL;
}

// Constructor with socket and client address
ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) :
    _server(NULL),
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(NULL)),
    lastActivity(time(NULL)),
    builder(NULL),
    http_response(NULL),
    http_request(NULL),
    handler_chain(NULL),
    is_streaming_upload(false),
    total_content_length(0),
    bytes_received_so_far(0),
    temp_upload_fd(-1),
    should_close(false)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

void ClientConnection::GenerateRequest(int fd)
{
    // If we're in the middle of a streaming upload, continue reading
    if (is_streaming_upload) {
        if (continueStreamingRead(fd)) {
            // Upload complete, process normally
            finalizeStreaming();
            return;
        } else {
            // Still reading, will continue on next poll event
            return;
        }
    }

    char buffer[REQUSET_LINE_BUFFER];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        std::cerr << "Error receiving data: "
                  << (bytesRead == 0 ? "Connection closed" : strerror(errno))
                  << "\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    // Null-terminate the buffer
    buffer[bytesRead] = '\0';

    std::string rawRequest(buffer);
    std::cout << "==================== (Buffer:) =====================\n " << buffer <<
        "\n==================================================================" << std::endl;

    // Parse the raw request string
    HttpRequestBuilder build = HttpRequestBuilder();
    build.ParseRequest(rawRequest, this->_server->getConfigForClient(this->GetFd()));

    // Check if we need to read more data (for POST requests with Content-Length)
    std::string contentLengthStr = build.GetHttpRequest().GetHeader("Content-Length");
    if (!contentLengthStr.empty()) {
        long contentLength = atol(contentLengthStr.c_str());
        std::cout << "Content-Length header found: " << contentLength << " bytes" << std::endl;
        
        // Find the body start position (after \r\n\r\n)
        size_t bodyStart = rawRequest.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 4; // Move past \r\n\r\n
            size_t bodyBytesRead = rawRequest.length() - bodyStart;
            std::cout << "Already read " << bodyBytesRead << " bytes of body" << std::endl;
            
            // If it's a large upload, switch to streaming mode
            if (contentLength > MAX_MEMORY_UPLOAD) {
                std::cout << "Large upload detected (" << contentLength 
                         << " bytes), switching to streaming mode" << std::endl;
                
                std::string initial_body = rawRequest.substr(bodyStart);
                
                // Initialize streaming
                initializeStreaming(contentLength);
                
                // Save initial body data to file
                if (!initial_body.empty()) {
                    write(temp_upload_fd, initial_body.c_str(), initial_body.size());
                    bytes_received_so_far = initial_body.size();
                    std::cout << "Saved initial " << initial_body.size() << " bytes to file" << std::endl;
                }
                
                // Create request object with streaming marker IMMEDIATELY
                if (this->http_request) {
                    delete this->http_request;
                }
                this->http_request = new HttpRequest(build.GetHttpRequest());
                this->http_request->SetClientData(this);
                
                // CRITICAL FIX: Set the streaming marker immediately
                this->http_request->SetBody("__STREAMING_UPLOAD_FILE:" + temp_upload_path);
                std::cout << "Set streaming upload marker in request body: __STREAMING_UPLOAD_FILE:" << temp_upload_path << std::endl;
                
                // Check if upload is already complete
                if (bytes_received_so_far >= total_content_length) {
                    std::cout << "Upload already complete with initial read" << std::endl;
                    finalizeStreaming();
                } else {
                    std::cout << "Streaming upload in progress, waiting for more data..." << std::endl;
                    // CRITICAL FIX: Actually try to read more data now!
                    if (continueStreamingRead(fd)) {
                        // Upload completed immediately
                        finalizeStreaming();
                    }
                    // If not complete, will continue on next poll event
                }
                return;
            }
            
            // Handle SMALL uploads (< 1MB) - your original logic
            if (bodyBytesRead < (size_t)contentLength) {
                size_t remainingBytes = contentLength - bodyBytesRead;
                std::cout << "Need to read " << remainingBytes << " more bytes" << std::endl;
                
                // Read the rest of the body in chunks
                char* bodyBuffer = new char[remainingBytes + 1];
                size_t totalBodyBytesRead = 0;
                
                while (totalBodyBytesRead < remainingBytes) {
                    ssize_t chunkRead = recv(fd, bodyBuffer + totalBodyBytesRead, 
                        remainingBytes - totalBodyBytesRead, 0);
                    if (chunkRead <= 0) {
                        delete[] bodyBuffer;
                        std::cerr << "Error reading request body: " 
                                 << (chunkRead == 0 ? "Connection closed" : strerror(errno))
                                 << std::endl;
                        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
                    }
                    totalBodyBytesRead += chunkRead;
                    std::cout << "Read " << chunkRead << " bytes, total: " 
                             << totalBodyBytesRead << "/" << remainingBytes << std::endl;
                }
                
                // Null-terminate the body buffer
                bodyBuffer[totalBodyBytesRead] = '\0';
                
                // Append the body to the raw request
                std::string fullBody(bodyBuffer, totalBodyBytesRead);
                delete[] bodyBuffer;
                
                // Set the full body in the request
                build.SetBody(rawRequest.substr(bodyStart) + fullBody);
                std::cout << "Full body size: " << build.GetHttpRequest().GetBody().size() << " bytes" << std::endl;
            } else {
                // All body data was received in the initial read
                std::string bodyData = rawRequest.substr(bodyStart);
                build.SetBody(bodyData);
                std::cout << "Complete body received in initial read: " << bodyData.size() << " bytes" << std::endl;
            }
        } else {
            std::cout << "WARNING: Could not find body start marker (\\r\\n\\r\\n)" << std::endl;
        }
    }
    
    // Clean up any existing request object
    // int redirectCounter = this->http_request->GetRedirectCounter();
    if (this->http_request)
    {
        delete this->http_request;
    }
    this->http_request = new HttpRequest(build.GetHttpRequest());
    this->http_request->SetClientData(this);
    // if (redirectCounter)
    //     this->http_request->SetRedirectCounter(redirectCounter);
    
    // For small uploads, process immediately
    // For large uploads, this won't be reached until streaming is complete
    ProcessRequest(fd);
}

void ClientConnection::initializeStreaming(size_t content_length)
{
    is_streaming_upload = true;
    total_content_length = content_length;
    bytes_received_so_far = 0;
    
    // Create temporary file for streaming upload
    char temp_template[] = "/tmp/webserver_upload_XXXXXX";
    temp_upload_fd = mkstemp(temp_template);
    if (temp_upload_fd == -1) {
        std::cerr << "Failed to create temporary file: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    temp_upload_path = temp_template;
    
    std::cout << "Streaming upload initialized: " << content_length 
              << " bytes -> " << temp_upload_path << std::endl;
}

bool ClientConnection::continueStreamingRead(int fd)
{
    char chunk_buffer[STREAM_CHUNK_SIZE];
    
    // Set socket to blocking mode for reliable data reception
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    // Remove O_NONBLOCK flag to make socket blocking
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        std::cerr << "Failed to set socket to blocking mode: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 30;  // 30 seconds timeout
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) {
        std::cerr << "Failed to set socket timeout: " << strerror(errno) << std::endl;
        // Not fatal, continue anyway
    }
    
    // Main upload loop
    while (bytes_received_so_far < total_content_length) {
        size_t bytes_to_read = std::min((size_t)STREAM_CHUNK_SIZE, 
                                       total_content_length - bytes_received_so_far);
        
        // Use blocking read with timeout
        ssize_t chunk_read = recv(fd, chunk_buffer, bytes_to_read, 0);
        
        if (chunk_read <= 0) {
            if (chunk_read == 0) {
                std::cerr << "Connection closed during upload" << std::endl;
                // Restore non-blocking mode before throwing
                fcntl(fd, F_SETFL, flags);
                throw HttpException(400, "Bad Request", BAD_REQUEST);
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    // Timeout or interrupted
                    std::cout << "Socket timeout or interrupted, progress so far: " 
                             << bytes_received_so_far << "/" << total_content_length 
                             << " bytes (" << (bytes_received_so_far * 100 / total_content_length) 
                             << "%)" << std::endl;
                    
                    // Restore non-blocking mode
                    fcntl(fd, F_SETFL, flags);
                    return false; // Will continue on next poll event
                } else {
                    std::cerr << "Error during streaming read: " << strerror(errno) << std::endl;
                    // Restore non-blocking mode before throwing
                    fcntl(fd, F_SETFL, flags);
                    throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
                }
            }
        }
        
        // Write chunk directly to file
        ssize_t written = write(temp_upload_fd, chunk_buffer, chunk_read);
        if (written != chunk_read) {
            std::cerr << "Failed to write chunk to file: " << strerror(errno) << std::endl;
            // Restore non-blocking mode before throwing
            fcntl(fd, F_SETFL, flags);
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        
        bytes_received_so_far += chunk_read;
        updateActivity();
        
        // Progress reporting
        if (bytes_received_so_far % (1024 * 1024) == 0 || // Every 1MB
            bytes_received_so_far == total_content_length) { // Or at completion
            std::cout << "Upload progress: " << bytes_received_so_far 
                     << "/" << total_content_length << " bytes ("
                     << (bytes_received_so_far * 100 / total_content_length) << "%)" << std::endl;
        }
        
        // Sync file to disk periodically
        if (bytes_received_so_far % (10 * 1024 * 1024) == 0) { // Every 10MB
            fsync(temp_upload_fd);
        }
    }
    
    // Final sync to ensure all data is written to disk
    fsync(temp_upload_fd);
    
    // Restore non-blocking mode
    fcntl(fd, F_SETFL, flags);
    
    std::cout << "Streaming upload data reception complete!" << std::endl;
    return true; // Upload complete
}

void ClientConnection::finalizeStreaming()
{
    if (!is_streaming_upload) {
        return; // Not in streaming mode
    }
    
    // Check if we've already finalized this streaming upload
    static std::set<std::string> finalized_uploads;
    if (!temp_upload_path.empty() && finalized_uploads.find(temp_upload_path) != finalized_uploads.end()) {
        std::cout << "Upload already finalized, skipping duplicate processing: " << temp_upload_path << std::endl;
        return;
    }
    
    // Close the temporary file
    if (temp_upload_fd != -1) {
        close(temp_upload_fd);
        temp_upload_fd = -1;
    }
    
    std::cout << "Streaming upload completed: " << bytes_received_so_far 
              << " bytes saved to " << temp_upload_path << std::endl;
    
    // Verify the streaming upload marker is in the request body
    if (http_request && http_request->GetBody().find("__STREAMING_UPLOAD_FILE:") != std::string::npos) {
        std::cout << "Streaming upload marker confirmed in request body" << std::endl;
    } else {
        std::cerr << "Warning: Streaming upload marker not found in request body" << std::endl;
        if (http_request) {
            http_request->SetBody("__STREAMING_UPLOAD_FILE:" + temp_upload_path);
        }
    }
    
    // Mark this upload as finalized to prevent duplicate processing
    if (!temp_upload_path.empty()) {
        finalized_uploads.insert(temp_upload_path);
    }
    
    // Reset streaming state
    is_streaming_upload = false;
    total_content_length = 0;
    bytes_received_so_far = 0;
    
    std::cout << "Processing streaming upload request..." << std::endl;
    ProcessRequest(fd);
}

//@todo: Consider logical work flow before !!!!! refactoring this function to handle errors more gracefully
void ClientConnection::ProcessRequest(int fd)
{
    RequestHandler *chain_handler;
    
    chain_handler = new CgiHandler(this);
    (chain_handler->SetNext(new Get()))->SetNext(new Post())->SetNext(new Delete());
    std::cout << "Available handlers: GET, POST, DELETE" << std::endl;

    if (http_request == NULL) {
        std::cerr << "Error: No request to process" << std::endl;
        return;
    }

    // Ensure http_response is properly initialized before proceeding
    if (this->http_response == NULL) {
        std::map<std::string, std::string> emptyHeaders;
        this->http_response = new HttpResponse(200, emptyHeaders, "text/plain", false, false);
    }
    
    chain_handler->HandleRequest(this->http_request, this->_server->getConfigForClient(this->GetFd()));
    std::cout << "END OF PROCESSING THE REQUEST \n";
    if (this->_server != NULL) {
        this->_server->updatePollEvents(fd, POLLOUT);
    } else {
        std::cerr << "Error: Server pointer is NULL" << std::endl;
    }
    delete chain_handler;
}

ClientConnection::~ClientConnection()
{
    // Clean up streaming upload resources first
    if (temp_upload_fd != -1) {
        close(temp_upload_fd);
        temp_upload_fd = -1;
        if (!temp_upload_path.empty()) {
            unlink(temp_upload_path.c_str()); // Clean up temp file
        }
    }
    
    // Clean up allocated resources in a safe way
    if (http_request != NULL) {
        delete http_request;
        http_request = NULL;
    }
    
    if (http_response != NULL) {
        delete http_response;
        http_response = NULL;
    }
    
    if (builder != NULL) {
        delete builder;
        builder = NULL;
    }
    
    // Don't delete handler_chain here, it could be a dangling pointer!!!!
    // or managed elsewhere
}

// Update activity timestamp
void ClientConnection::updateActivity()
{
    lastActivity = time(NULL);
}

// Check if connection is stale (timeout)
bool ClientConnection::isStale(time_t timeoutSec) const
{
    return (time(NULL) - lastActivity) > timeoutSec;
}

// Placeholder implementations for methods declared in header
void ClientConnection::RespondToClient(int fd)
{
    (void)fd; // Silence unused parameter warning
    // This method is handled by your WebServer class
}

void ClientConnection::parseRequest(char *buff)
{
    (void)buff; // Silence unused parameter warning
    // This functionality is handled by GenerateRequest()
}



int ClientConnection::GetFd() const {
    return (fd);
}
