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
#include <iomanip>
#include <chrono>

int ClientConnection::redirect_counter = 0;

ClientConnection::ClientConnection() 
    : fd(-1), ipAddress(""), port(0), connectTime(0), lastActivity(0),
      builder(NULL), http_response(NULL), http_request(NULL),
      is_streaming_upload(false), total_content_length(0), 
      bytes_received_so_far(0), temp_upload_fd(-1), should_close(false)
{
    this->_server = NULL;
}

ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) 
    : _server(NULL), fd(socketFd), port(ntohs(clientAddr.sin_port)),
      connectTime(time(NULL)), lastActivity(time(NULL)),
      builder(NULL), http_response(NULL), http_request(NULL),
      is_streaming_upload(false), total_content_length(0),
      bytes_received_so_far(0), temp_upload_fd(-1), should_close(false)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

ClientConnection::~ClientConnection()
{
    if (temp_upload_fd != -1) {
        close(temp_upload_fd);
        temp_upload_fd = -1;
        if (!temp_upload_path.empty()) {
            unlink(temp_upload_path.c_str());
        }
    }
    
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
}

void ClientConnection::GenerateRequest(int fd)
{
    if (is_streaming_upload) {
        if (continueStreamingRead(fd)) {
            finalizeStreaming();
            return;
        } else {
            return;
        }
    }

    char buffer[REQUSET_LINE_BUFFER];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        std::cerr << "Error receiving data: "
                  << (bytesRead == 0 ? "Connection closed" : strerror(errno))
                  << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    buffer[bytesRead] = '\0';
    std::string rawRequest(buffer);
    
    std::cout << "Received request from client " << fd << std::endl;

    HttpRequestBuilder build = HttpRequestBuilder();
    build.ParseRequest(rawRequest, this->_server->getConfigForClient(this->GetFd()));

    std::string contentLengthStr = build.GetHttpRequest().GetHeader("Content-Length");
    if (!contentLengthStr.empty()) {
        long contentLength = atol(contentLengthStr.c_str());
        std::cout << "Content-Length: " << contentLength << " bytes" << std::endl;
        
        size_t bodyStart = rawRequest.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 4;
            size_t bodyBytesRead = rawRequest.length() - bodyStart;
            
            std::cout << "Initial buffer size: " << rawRequest.length() << " bytes" << std::endl;
            std::cout << "Body starts at position: " << bodyStart << std::endl;
            std::cout << "Body bytes in initial read: " << bodyBytesRead << " bytes" << std::endl;
            std::cout << "Total Content-Length: " << contentLength << " bytes" << std::endl;
            
            if (contentLength > MAX_MEMORY_UPLOAD) {
                std::cout << "Large upload detected, enabling streaming mode" << std::endl;
                
                std::string initial_body = rawRequest.substr(bodyStart);
                initializeStreaming(contentLength);
                
                if (!initial_body.empty()) {
                    write(temp_upload_fd, initial_body.c_str(), initial_body.size());
                    bytes_received_so_far = initial_body.size();
                    showProgress();
                }
                
                if (this->http_request) {
                    delete this->http_request;
                }
                this->http_request = new HttpRequest(build.GetHttpRequest());
                this->http_request->SetClientData(this);
                this->http_request->SetBody("__STREAMING_UPLOAD_FILE:" + temp_upload_path);
                
                if (bytes_received_so_far >= total_content_length) {
                    std::cout << "Upload complete" << std::endl;
                    finalizeStreaming();
                } else {
                    if (continueStreamingRead(fd)) {
                        std::cout << "Upload complete" << std::endl;
                        finalizeStreaming();
                    }
                }
                return;
            }
            
            // CRITICAL FIX: Calculate remaining bytes correctly
            // We need to read (Content-Length - body_bytes_already_read)
            size_t totalBytesRead = rawRequest.length();
            size_t remainingTotalBytes = 0;
            
            if (totalBytesRead < (size_t)contentLength) {
                remainingTotalBytes = contentLength - totalBytesRead;
                std::cout << "Need to read " << remainingTotalBytes << " more bytes to complete the request" << std::endl;
                
                // Set socket to blocking mode for reliable body reading
                int flags = fcntl(fd, F_GETFL, 0);
                if (flags != -1) {
                    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
                }
                
                // Set receive timeout
                struct timeval tv;
                tv.tv_sec = 30;
                tv.tv_usec = 0;
                setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                
                // Read the remaining data
                std::string remainingData;
                remainingData.reserve(remainingTotalBytes);
                
                char* tempBuffer = new char[8192]; // Use smaller chunks
                size_t totalAdditionalBytesRead = 0;
                
                while (totalAdditionalBytesRead < remainingTotalBytes) {
                    size_t bytesToRead = std::min((size_t)8192, remainingTotalBytes - totalAdditionalBytesRead);
                    
                    ssize_t chunkRead = recv(fd, tempBuffer, bytesToRead, 0);
                    if (chunkRead <= 0) {
                        delete[] tempBuffer;
                        // Restore socket flags
                        if (flags != -1) fcntl(fd, F_SETFL, flags);
                        std::cerr << "Error reading remaining request data: " 
                                << (chunkRead == 0 ? "Connection closed" : strerror(errno)) << std::endl;
                        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
                    }
                    
                    remainingData.append(tempBuffer, chunkRead);
                    totalAdditionalBytesRead += chunkRead;
                    
                    std::cout << "Read " << chunkRead << " bytes, total additional: " 
                             << totalAdditionalBytesRead << "/" << remainingTotalBytes << std::endl;
                }
                
                delete[] tempBuffer;
                
                // Restore socket flags
                if (flags != -1) fcntl(fd, F_SETFL, flags);
                
                // Combine the initial request with the remaining data
                std::string completeRequest = rawRequest + remainingData;
                
                // Now extract the body from the complete request
                size_t completeBodyStart = completeRequest.find("\r\n\r\n");
                if (completeBodyStart != std::string::npos) {
                    completeBodyStart += 4;
                    std::string completeBody = completeRequest.substr(completeBodyStart);
                    build.SetBody(completeBody);
                    
                    std::cout << "Complete request assembled: " << completeRequest.length() << " bytes" << std::endl;
                    std::cout << "Complete body extracted: " << completeBody.length() << " bytes" << std::endl;
                } else {
                    std::cerr << "Could not find body separator in complete request" << std::endl;
                    throw HttpException(400, "Bad Request", BAD_REQUEST);
                }
            } else {
                // All data was received in the initial read
                std::string bodyData = rawRequest.substr(bodyStart);
                build.SetBody(bodyData);
                std::cout << "Complete request received in initial read" << std::endl;
                std::cout << "Body size: " << bodyData.size() << " bytes" << std::endl;
            }
        } else {
            std::cout << "Could not find body separator (\\r\\n\\r\\n)" << std::endl;
        }
    }
    
    if (this->http_request) {
        delete this->http_request;
    }
    this->http_request = new HttpRequest(build.GetHttpRequest());
    this->http_request->SetClientData(this);
    
    ProcessRequest(fd);
}

void ClientConnection::initializeStreaming(size_t content_length)
{
    is_streaming_upload = true;
    total_content_length = content_length;
    bytes_received_so_far = 0;
    
    char temp_template[] = "/tmp/webserver_upload_XXXXXX";
    temp_upload_fd = mkstemp(temp_template);
    if (temp_upload_fd == -1) {
        std::cerr << "Failed to create temporary file" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    temp_upload_path = temp_template;
    
    std::cout << "Streaming initialized for " << content_length << " bytes" << std::endl;
}

bool ClientConnection::continueStreamingRead(int fd)
{
    char chunk_buffer[STREAM_CHUNK_SIZE];
    
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    size_t last_progress_bytes = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (bytes_received_so_far < total_content_length) {
        size_t bytes_to_read = std::min((size_t)STREAM_CHUNK_SIZE, 
                                       total_content_length - bytes_received_so_far);
        
        ssize_t chunk_read = recv(fd, chunk_buffer, bytes_to_read, 0);
        
        if (chunk_read <= 0) {
            if (chunk_read == 0) {
                fcntl(fd, F_SETFL, flags);
                throw HttpException(400, "Bad Request", BAD_REQUEST);
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    fcntl(fd, F_SETFL, flags);
                    return false;
                } else {
                    fcntl(fd, F_SETFL, flags);
                    throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
                }
            }
        }
        
        ssize_t written = write(temp_upload_fd, chunk_buffer, chunk_read);
        if (written != chunk_read) {
            fcntl(fd, F_SETFL, flags);
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        
        bytes_received_so_far += chunk_read;
        updateActivity();
        
        // Update progress display
        if ((bytes_received_so_far - last_progress_bytes) >= (512 * 1024) || 
            bytes_received_so_far == total_content_length) {
            
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - start_time).count();
            
            double speed_mbps = 0.0;
            if (elapsed > 0) {
                speed_mbps = (double)bytes_received_so_far / (1024.0 * 1024.0) / (elapsed / 1000.0);
            }
            
            showProgressBar(speed_mbps);
            last_progress_bytes = bytes_received_so_far;
        }
        
        if (bytes_received_so_far % (10 * 1024 * 1024) == 0) {
            fsync(temp_upload_fd);
        }
    }
    
    fsync(temp_upload_fd);
    fcntl(fd, F_SETFL, flags);
    
    std::cout << std::endl << "Upload complete!" << std::endl;
    return true;
}

void ClientConnection::showProgress()
{
    double progress = (double)bytes_received_so_far / total_content_length * 100.0;
    std::cout << "Progress: " << std::fixed << std::setprecision(1) 
             << progress << "% (" << bytes_received_so_far 
             << "/" << total_content_length << " bytes)" << std::endl;
}

void ClientConnection::showProgressBar(double speed_mbps)
{
    double progress = (double)bytes_received_so_far / total_content_length * 100.0;
    int bar_width = 40;
    int filled_width = (int)(progress / 100.0 * bar_width);
    
    if (progress >= 100.0) {
        std::cout << "\rProgress: 100.0% [";
        for (int i = 0; i < bar_width; ++i) std::cout << "=";
        std::cout << "] " << std::fixed << std::setprecision(1) 
                 << speed_mbps << " MB/s - Complete!" << std::endl;
    } else {
        std::cout << "\rProgress: " << std::fixed << std::setprecision(1) 
                 << progress << "% [";
        
        for (int i = 0; i < bar_width; ++i) {
            if (i < filled_width) std::cout << "=";
            else if (i == filled_width) std::cout << ">";
            else std::cout << " ";
        }
        
        std::cout << "] " << std::fixed << std::setprecision(1) 
                 << speed_mbps << " MB/s";
        std::cout.flush();
    }
}

void ClientConnection::finalizeStreaming()
{
    if (!is_streaming_upload) {
        return;
    }
    
    static std::set<std::string> finalized_uploads;
    if (!temp_upload_path.empty() && 
        finalized_uploads.find(temp_upload_path) != finalized_uploads.end()) {
        return;
    }
    
    if (temp_upload_fd != -1) {
        close(temp_upload_fd);
        temp_upload_fd = -1;
    }
    
    std::cout << "Streaming complete: " << bytes_received_so_far 
              << " bytes saved" << std::endl;
    
    if (http_request && 
        http_request->GetBody().find("__STREAMING_UPLOAD_FILE:") == std::string::npos) {
        http_request->SetBody("__STREAMING_UPLOAD_FILE:" + temp_upload_path);
    }
    
    if (!temp_upload_path.empty()) {
        finalized_uploads.insert(temp_upload_path);
    }
    
    is_streaming_upload = false;
    total_content_length = 0;
    bytes_received_so_far = 0;
    
    ProcessRequest(fd);
}

void ClientConnection::ProcessRequest(int fd)
{
    RequestHandler *chain_handler = new CgiHandler(this);
    (chain_handler->SetNext(new Get()))->SetNext(new Post())->SetNext(new Delete());
    
    if (http_request == NULL) {
        std::cerr << "No request to process" << std::endl;
        return;
    }
    
    if (this->http_response == NULL) {
        std::map<std::string, std::string> emptyHeaders;
        this->http_response = new HttpResponse(200, emptyHeaders, "text/plain", false, false);
    }
    
    chain_handler->HandleRequest(this->http_request, 
                                this->_server->getConfigForClient(this->GetFd()));
    
    if (this->_server != NULL) {
        this->_server->updatePollEvents(fd, POLLOUT);
    } else {
        std::cerr << "Server pointer is NULL" << std::endl;
    }
    
    // Clean up handler chain
    while (chain_handler->GetNext() != NULL) {
        RequestHandler *next_handler = chain_handler->GetNext();
        chain_handler->SetNext(NULL);
        delete chain_handler; 
        chain_handler = next_handler; 
    }
    if (chain_handler != NULL) {
        delete chain_handler;
    }
}

void ClientConnection::updateActivity()
{
    lastActivity = time(NULL);
}

bool ClientConnection::isStale(time_t timeoutSec) const
{
    return (time(NULL) - lastActivity) > timeoutSec;
}

void ClientConnection::RespondToClient(int fd)
{
    (void)fd;
}

void ClientConnection::parseRequest(char *buff)
{
    (void)buff;
}

int ClientConnection::GetFd() const 
{
    return fd;
}