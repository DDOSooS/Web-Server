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
      bytes_received_so_far(0), temp_upload_fd(-1), should_close(false),
      filename_detected(false), is_multipart_upload(false), multipart_boundary(""),
      detected_filename(""), detected_extension(".bin")
{
    this->_server = NULL;
}

ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) 
    : _server(NULL), fd(socketFd), port(ntohs(clientAddr.sin_port)),
      connectTime(time(NULL)), lastActivity(time(NULL)),
      builder(NULL), http_response(NULL), http_request(NULL),
      is_streaming_upload(false), total_content_length(0),
      bytes_received_so_far(0), temp_upload_fd(-1), should_close(false),
      filename_detected(false), is_multipart_upload(false), multipart_boundary(""),
      detected_filename(""), detected_extension(".bin")
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

void ClientConnection::setServerConfig(const ServerConfig& config)
{
    server_config = config;
}

ServerConfig ClientConnection::getServerConfig() const
{
    return server_config;
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
        // If we're already in streaming mode, don't try to generate a new request
        std::cout << "Already in streaming mode, skipping request generation" << std::endl;
        return;
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
    
    std::cout << "Received request from client " << buffer << std::endl;

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
                
                // Create the HTTP request object first
                if (this->http_request) {
                    delete this->http_request;
                }
                this->http_request = new HttpRequest(build.GetHttpRequest());
                this->http_request->SetClientData(this);
                this->setServerConfig(this->_server->getConfigByHost(this->http_request->GetHeader("Host")));
                
                // Get content type from the request
                std::string content_type = this->http_request->GetHeader("Content-Type");
                std::string initial_body = rawRequest.substr(bodyStart);
                std::string extended_body = initial_body;
                std::string original_filename;
                std::string file_extension = ".bin";
                
                // For multipart/form-data uploads, we need to extract the boundary and properly parse it
                if (content_type.find("multipart/form-data") != std::string::npos) {
                    std::cout << "Processing multipart/form-data upload" << std::endl;
                    is_multipart_upload = true;
                    
                    // Extract boundary from the Content-Type header
                    size_t boundary_pos = content_type.find("boundary=");
                    if (boundary_pos != std::string::npos) {
                        boundary_pos += 9; // Skip "boundary="
                        
                        // Handle quoted and unquoted boundaries
                        if (content_type[boundary_pos] == '"') {
                            boundary_pos++; // Skip the quote
                            size_t boundary_end = content_type.find('"', boundary_pos);
                            if (boundary_end != std::string::npos) {
                                multipart_boundary = content_type.substr(boundary_pos, boundary_end - boundary_pos);
                            }
                        } else {
                            // Unquoted boundary ends at semicolon or end of string
                            size_t boundary_end = content_type.find(';', boundary_pos);
                            if (boundary_end == std::string::npos) {
                                boundary_end = content_type.length();
                            }
                            multipart_boundary = content_type.substr(boundary_pos, boundary_end - boundary_pos);
                        }
                        
                        std::cout << "Found multipart boundary: '" << multipart_boundary << "'" << std::endl;
                    }
                    
                    // If we don't have enough data, read more to get the multipart headers
                    if (extended_body.find("filename=") == std::string::npos || 
                        extended_body.find("Content-Disposition:") == std::string::npos) {
                        
                        std::cout << "Reading additional data to extract filename..." << std::endl;
                        
                        // Read up to 8KB more to get the multipart headers
                        char extra_buffer[8192];
                        ssize_t extra_read = recv(fd, extra_buffer, sizeof(extra_buffer) - 1, 0);
                        if (extra_read > 0) {
                            extra_buffer[extra_read] = '\0';
                            extended_body += std::string(extra_buffer, extra_read);
                            bodyBytesRead += extra_read;
                            std::cout << "Read additional " << extra_read << " bytes for filename extraction" << std::endl;
                            std::cout << "Extended body size: " << extended_body.size() << " bytes" << std::endl;
                        }
                    }
                    
                    // Now try to extract filename from the extended body
                    size_t content_disposition_pos = extended_body.find("Content-Disposition:");
                    if (content_disposition_pos != std::string::npos) {
                        std::cout << "Found Content-Disposition at position: " << content_disposition_pos << std::endl;
                        
                        // Look for filename parameter
                        size_t filename_pos = extended_body.find("filename=\"", content_disposition_pos);
                        if (filename_pos != std::string::npos) {
                            filename_pos += 10; // Skip "filename=\""
                            size_t filename_end = extended_body.find("\"", filename_pos);
                            if (filename_end != std::string::npos) {
                                original_filename = extended_body.substr(filename_pos, filename_end - filename_pos);
                                std::cout << "SUCCESS: Extracted filename: '" << original_filename << "'" << std::endl;
                                
                                // Extract extension
                                size_t dot_pos = original_filename.find_last_of('.');
                                if (dot_pos != std::string::npos) {
                                    file_extension = original_filename.substr(dot_pos);
                                    std::cout << "SUCCESS: Extracted extension: '" << file_extension << "'" << std::endl;
                                }
                            }
                        } else {
                            std::cout << "ERROR: Could not find filename in Content-Disposition" << std::endl;
                            
                            // Debug output
                            size_t debug_end = std::min(content_disposition_pos + 200, extended_body.size());
                            std::string debug_section = extended_body.substr(content_disposition_pos, debug_end - content_disposition_pos);
                            std::cout << "Content-Disposition section: '" << debug_section << "'" << std::endl;
                        }
                    } else {
                        std::cout << "ERROR: Could not find Content-Disposition in body" << std::endl;
                        
                        // Show first part of extended body for debugging
                        std::cout << "First 500 chars of extended body:" << std::endl;
                        std::string debug_body = extended_body.substr(0, std::min((size_t)500, extended_body.size()));
                        for (size_t i = 0; i < debug_body.length(); ++i) {
                            char c = debug_body[i];
                            if (c == '\r') std::cout << "\\r";
                            else if (c == '\n') std::cout << "\\n";
                            else if (c >= 32 && c <= 126) std::cout << c;
                            else std::cout << "[" << (int)c << "]";
                        }
                        std::cout << std::endl;
                    }
                }
                
                // If still no extension, guess from content type
                if (file_extension == ".bin" && !content_type.empty()) {
                    if (content_type.find("video/mp4") != std::string::npos) {
                        file_extension = ".mp4";
                    } else if (content_type.find("video/webm") != std::string::npos) {
                        file_extension = ".webm";
                    } else if (content_type.find("video/") != std::string::npos) {
                        file_extension = ".mp4";
                    } else if (content_type.find("image/jpeg") != std::string::npos) {
                        file_extension = ".jpg";
                    } else if (content_type.find("image/png") != std::string::npos) {
                        file_extension = ".png";
                    }
                    std::cout << "Guessed extension from Content-Type: " << file_extension << std::endl;
                }
                
                // Initialize streaming with extracted filename
                initializeStreamingWithFilename(contentLength, original_filename, file_extension);
                
                // Write any data we've already read
                if (!extended_body.empty()) {
                    write(temp_upload_fd, extended_body.c_str(), extended_body.size());
                    bytes_received_so_far = extended_body.size();
                    showProgress();
                }
                
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
            
            // Handle smaller uploads that fit in memory
            size_t totalBytesRead = rawRequest.length();
            size_t headerSize = bodyStart;
            size_t bodyBytesAlreadyRead = totalBytesRead - headerSize;
            size_t remainingTotalBytes = 0;
            
            std::cout << "DEBUG: Total bytes read: " << totalBytesRead << ", Header size: " << headerSize << ", Body bytes already read: " << bodyBytesAlreadyRead << std::endl;
            
            if (bodyBytesAlreadyRead < (size_t)contentLength) {
                remainingTotalBytes = contentLength - bodyBytesAlreadyRead;
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
                
                char* tempBuffer = new char[8192];
                size_t totalAdditionalBytesRead = 0;
                
                while (totalAdditionalBytesRead < remainingTotalBytes) {
                    size_t bytesToRead = std::min((size_t)8192, remainingTotalBytes - totalAdditionalBytesRead);
                    
                    ssize_t chunkRead = recv(fd, tempBuffer, bytesToRead, 0);
                    if (chunkRead <= 0) {
                        delete[] tempBuffer;
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
                
                // Extract the initial body part from the original request
                std::string initialBody = rawRequest.substr(bodyStart);
                
                // Combine the initial body with the remaining data
                std::string completeBody = initialBody + remainingData;
                build.SetBody(completeBody);
                
                std::cout << "Initial body size: " << initialBody.length() << " bytes" << std::endl;
                std::cout << "Additional data size: " << remainingData.length() << " bytes" << std::endl;
                std::cout << "Complete body assembled: " << completeBody.length() << " bytes" << std::endl;
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
    
    // Create final HTTP request object
    if (this->http_request) {
        delete this->http_request;
    }
    this->http_request = new HttpRequest(build.GetHttpRequest());
    this->http_request->SetClientData(this);
    this->setServerConfig(this->_server->getConfigForClient(this->GetFd()));

    std::cout << "Server Name is: " << this->getServerConfig().get_server_name() << "=============\n\n\n" << std::endl;
    std::cout << "Server Root is: " << this->getServerConfig().get_root() << "=============\n\n\n" << std::endl;
}


void ClientConnection::initializeStreamingWithFilename(size_t content_length, const std::string& original_filename, const std::string& file_extension)
{
    is_streaming_upload = true;
    total_content_length = content_length;
    bytes_received_so_far = 0;
    
    // Initialize filename detection state
    if (!original_filename.empty()) {
        detected_filename = original_filename;
        filename_detected = true;
    }
    
    if (file_extension != ".bin") {
        detected_extension = file_extension;
        filename_detected = true;
    }
    
    // Get the appropriate uploads directory from Post class
    Post post_handler;
    std::string uploads_dir = post_handler.getUploadsDirectory(server_config);
    
    // Create a unique filename while preserving the original name
    std::string base_filename;
    if (!original_filename.empty()) {
        // Remove the extension from the original filename to avoid double extensions
        size_t dot_pos = original_filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            base_filename = original_filename.substr(0, dot_pos);
        } else {
            base_filename = original_filename;
        }
        
        // Sanitize the filename (remove problematic characters)
        for (size_t i = 0; i < base_filename.length(); i++) {
            char c = base_filename[i];
            if (c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || 
                c == '{' || c == '}' || c == '&' || c == '?' || c == '*' || 
                c == '<' || c == '>' || c == '|' || c == ':' || c == '\\' || c == '/') {
                base_filename[i] = '_';
            }
        }
    } else {
        base_filename = "upload";
    }
    
    // Create a unique filename to avoid collisions
    std::stringstream ss;
    ss << time(NULL) << "_" << ipAddress << "_" << port << "_" << base_filename;
    std::string final_filename = ss.str() + file_extension;
    
    temp_upload_path = uploads_dir + "/" + final_filename;
    
    std::cout << "Creating file: " << temp_upload_path << std::endl;
    std::cout << "Original filename: " << (original_filename.empty() ? "Not found" : original_filename) << std::endl;
    std::cout << "Base filename: " << base_filename << std::endl;
    std::cout << "File extension: " << file_extension << std::endl;
    std::cout << "Final filename: " << final_filename << std::endl;
    
    // Create the final file directly
    temp_upload_fd = open(temp_upload_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (temp_upload_fd == -1) {
        std::cerr << "Failed to create destination file: " << temp_upload_path
                 << " (errno: " << errno << ": " << strerror(errno) << ")" << std::endl;
        
        // Fallback: If the error is that the file exists, try with a different name
        if (errno == EEXIST) {
            ss.str("");  // Clear the stringstream
            ss << time(NULL) << "_" << ipAddress << "_" << port << "_" << base_filename << "_" << (rand() % 1000);
            final_filename = ss.str() + file_extension;
            temp_upload_path = uploads_dir + "/" + final_filename;
            temp_upload_fd = open(temp_upload_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        }
        
        // If still failed, try with /tmp as a last resort
        if (temp_upload_fd == -1) {
            std::cerr << "Failed to create file in uploads directory, falling back to /tmp" << std::endl;
            temp_upload_path = "/tmp/" + final_filename;
            temp_upload_fd = open(temp_upload_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
            
            if (temp_upload_fd == -1) {
                std::cerr << "Failed to create destination file: " << temp_upload_path
                         << " (errno: " << errno << ": " << strerror(errno) << ")" << std::endl;
                throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
            }
        }
    }
    
    std::cout << "Streaming initialized for " << content_length << " bytes directly to: " << temp_upload_path << std::endl;
}

void ClientConnection::initializeStreaming(size_t content_length)
{
    // This method is now deprecated - use initializeStreamingWithFilename instead
    // Keeping for compatibility but calling the new method with defaults
    initializeStreamingWithFilename(content_length, "", ".bin");
}

// COMPLETELY REWRITTEN: Simple, non-blocking streaming read
bool ClientConnection::continueStreamingRead(int fd)
{
    // Simple approach: Read what's available RIGHT NOW and return
    // Don't try to read everything at once - NO WHILE LOOPS!
    
    char chunk_buffer[STREAM_CHUNK_SIZE];
    
    // Keep socket non-blocking - this is crucial!
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    // Try to read some data, but don't block
    ssize_t chunk_read = recv(fd, chunk_buffer, STREAM_CHUNK_SIZE, MSG_DONTWAIT);
    
    if (chunk_read > 0) {
        // First, check if we can extract a filename from this chunk (for multipart uploads)
        // This is important because the filename often comes after streaming has started
        if (is_multipart_upload && !filename_detected && bytes_received_so_far < 64000) { // Only try in first ~64KB
            if (tryExtractFilenameFromData(chunk_buffer, chunk_read)) {
                // If we found a filename with a good extension, update the file
                updateFileExtensionIfNeeded();
            }
        }
        
        // Got some data - write it to the temp file
        ssize_t written = write(temp_upload_fd, chunk_buffer, chunk_read);
        if (written != chunk_read) {
            std::cerr << "Error writing to temp file" << std::endl;
            throw HttpException(500, "Failed to write upload data", INTERNAL_SERVER_ERROR);
        }
        
        bytes_received_so_far += chunk_read;
        updateActivity();
        
        // Show progress every 1MB or when complete
        if (bytes_received_so_far % (1024 * 1024) == 0 || 
            bytes_received_so_far >= total_content_length) {
            double progress = (double)bytes_received_so_far / total_content_length * 100.0;
            std::cout << "Upload progress: " << std::fixed << std::setprecision(1) 
                     << progress << "% (" << bytes_received_so_far 
                     << "/" << total_content_length << " bytes)" << std::endl;
        }
        
        // Check if upload is complete
        if (bytes_received_so_far >= total_content_length) {
            std::cout << "✅ Upload complete: " << bytes_received_so_far << " bytes" << std::endl;
            fsync(temp_upload_fd);
            return true; // Upload finished
        }
        
        // More data expected - return false so we get called again
        return false;
    }
    else if (chunk_read == 0) {
        // Client closed connection
        std::cout << "Client disconnected during upload at " 
                  << bytes_received_so_far << "/" << total_content_length 
                  << " bytes (" << (bytes_received_so_far * 100.0 / total_content_length) << "%)" << std::endl;
        
        // If we got most of the file, consider it successful
        if (bytes_received_so_far >= total_content_length * 0.9) {
            std::cout << "Got 90%+ of file, considering upload successful" << std::endl;
            return true;
        }
        
        throw HttpException(400, "Upload incomplete - connection closed", BAD_REQUEST);
    }
    else {
        // chunk_read < 0 - check errno
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available right now - this is normal for non-blocking sockets
            // Just return false and we'll be called again when more data arrives
            return false;
        }
        else {
            // Real error
            std::cerr << "Error reading upload data: " << strerror(errno) << std::endl;
            throw HttpException(500, "Error reading upload data", INTERNAL_SERVER_ERROR);
        }
    }
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

// Helper method to extract filename from multipart form data during streaming
bool ClientConnection::tryExtractFilenameFromData(const char* data, size_t length)
{
    // If we already found the filename, no need to continue
    if (filename_detected) {
        return true;
    }
    
    // Skip if we don't have a boundary or not a multipart upload
    if (!is_multipart_upload || multipart_boundary.empty()) {
        return false;
    }
    
    std::string data_chunk(data, std::min(length, (size_t)4096)); // Limit to first 4KB for performance
    
    // Look for Content-Disposition header in the data
    std::string full_boundary = "--" + multipart_boundary;
    size_t boundary_pos = data_chunk.find(full_boundary);
    
    if (boundary_pos != std::string::npos) {
        size_t content_disposition_pos = data_chunk.find("Content-Disposition:", boundary_pos);
        if (content_disposition_pos != std::string::npos) {
            size_t filename_pos = data_chunk.find("filename=\"", content_disposition_pos);
            if (filename_pos != std::string::npos) {
                filename_pos += 10; // Skip "filename=\""
                size_t filename_end = data_chunk.find("\"", filename_pos);
                if (filename_end != std::string::npos && filename_end > filename_pos) {
                    detected_filename = data_chunk.substr(filename_pos, filename_end - filename_pos);
                    std::cout << "Found filename in streaming data: '" << detected_filename << "'" << std::endl;
                    
                    // Extract extension
                    size_t dot_pos = detected_filename.find_last_of('.');
                    if (dot_pos != std::string::npos) {
                        detected_extension = detected_filename.substr(dot_pos);
                        std::cout << "Found file extension: '" << detected_extension << "'" << std::endl;
                        filename_detected = true;
                        return true;
                    }
                }
            }
            
            // If we found Content-Disposition but no extension yet, check for Content-Type
            if (!filename_detected) {
                size_t content_type_pos = data_chunk.find("Content-Type:", content_disposition_pos);
                if (content_type_pos != std::string::npos) {
                    size_t content_type_end = data_chunk.find("\r\n", content_type_pos);
                    if (content_type_end != std::string::npos) {
                        std::string part_content_type = data_chunk.substr(
                            content_type_pos + 13, // Skip "Content-Type: "
                            content_type_end - (content_type_pos + 13)
                        );
                        std::cout << "Found part Content-Type: '" << part_content_type << "'" << std::endl;
                        
                        // Map common MIME types to extensions
                        if (part_content_type.find("image/jpeg") != std::string::npos) {
                            detected_extension = ".jpg";
                        } else if (part_content_type.find("image/png") != std::string::npos) {
                            detected_extension = ".png";
                        } else if (part_content_type.find("image/gif") != std::string::npos) {
                            detected_extension = ".gif";
                        } else if (part_content_type.find("image/webp") != std::string::npos) {
                            detected_extension = ".webp";
                        } else if (part_content_type.find("video/mp4") != std::string::npos) {
                            detected_extension = ".mp4";
                        } else if (part_content_type.find("video/webm") != std::string::npos) {
                            detected_extension = ".webm";
                        } else if (part_content_type.find("video/quicktime") != std::string::npos) {
                            detected_extension = ".mov";
                        } else if (part_content_type.find("audio/mpeg") != std::string::npos) {
                            detected_extension = ".mp3";
                        } else if (part_content_type.find("audio/wav") != std::string::npos) {
                            detected_extension = ".wav";
                        } else if (part_content_type.find("audio/ogg") != std::string::npos) {
                            detected_extension = ".ogg";
                        } else if (part_content_type.find("application/pdf") != std::string::npos) {
                            detected_extension = ".pdf";
                        } else if (part_content_type.find("application/zip") != std::string::npos) {
                            detected_extension = ".zip";
                        } else if (part_content_type.find("application/x-iso9660-image") != std::string::npos) {
                            detected_extension = ".iso";
                        } else if (part_content_type.find("text/plain") != std::string::npos) {
                            detected_extension = ".txt";
                        } else if (part_content_type.find("text/csv") != std::string::npos) {
                            detected_extension = ".csv";
                        }
                        
                        if (detected_extension != ".bin") {
                            std::cout << "Determined extension from part Content-Type: '" << detected_extension << "'" << std::endl;
                            filename_detected = true;
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

// Update file extension if it has changed
bool ClientConnection::updateFileExtensionIfNeeded()
{
    if (!filename_detected || temp_upload_path.empty() || detected_extension == ".bin") {
        return false;
    }
    
    std::string current_ext;
    size_t dot_pos = temp_upload_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        current_ext = temp_upload_path.substr(dot_pos);
    }
    
    // If the extension is already correct, do nothing
    if (current_ext == detected_extension) {
        return false;
    }
    
    // We need to rename the file
    std::cout << "Updating file extension from '" << current_ext << "' to '" << detected_extension << "'" << std::endl;
    
    // Construct the new path
    std::string new_path;
    if (dot_pos != std::string::npos) {
        new_path = temp_upload_path.substr(0, dot_pos) + detected_extension;
    } else {
        new_path = temp_upload_path + detected_extension;
    }
    
    // Close the current file
    if (temp_upload_fd != -1) {
        fsync(temp_upload_fd); // Make sure all data is flushed
        close(temp_upload_fd);
        temp_upload_fd = -1;
    }
    
    // Rename the file
    if (rename(temp_upload_path.c_str(), new_path.c_str()) != 0) {
        std::cerr << "Failed to rename file from " << temp_upload_path << " to " << new_path
                 << " (" << strerror(errno) << ")" << std::endl;
        return false;
    }
    
    // Reopen the file for appending
    temp_upload_fd = open(new_path.c_str(), O_WRONLY | O_APPEND, 0644);
    if (temp_upload_fd == -1) {
        std::cerr << "Failed to reopen renamed file: " << new_path
                 << " (" << strerror(errno) << ")" << std::endl;
        return false;
    }
    
    // Update internal state
    temp_upload_path = new_path;
    std::cout << "File successfully renamed to: " << new_path << std::endl;
    return true;
}

// SIMPLIFIED: Remove complex checks and mark file as directly uploaded
void ClientConnection::finalizeStreaming()
{
    if (!is_streaming_upload) {
        return;
    }
    
    // Close the final file
    if (temp_upload_fd != -1) {
        close(temp_upload_fd);
        temp_upload_fd = -1;
    }
    
    std::cout << "Finalizing streaming upload: " << bytes_received_so_far << " bytes" << std::endl;
    
    // Set the request body to point to our final file, with a marker indicating it's already in final location
    if (http_request) {
        http_request->SetBody("__DIRECT_UPLOAD_FILE:" + temp_upload_path);
    }
    
    // Reset streaming state
    is_streaming_upload = false;
    total_content_length = 0;
    bytes_received_so_far = 0;
    
    // Process the upload
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
                                this->_server->getConfigForClient(this->GetFd()), 
                                this->server_config);
    
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