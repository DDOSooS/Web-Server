#include "../include/request/Post.hpp"
#include <iostream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <set>

Post::Post() {}
Post::~Post() {}

bool Post::CanHandle(std::string method) {
    return method == "POST";
}

void Post::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig) {
    std::string body = request->GetBody();
    std::string contentType = request->GetHeader("Content-Type");
    std::string contentLength = request->GetHeader("Content-Length");
    
    // Check body size limit
    if (!contentLength.empty()) {
        size_t bodySize = 0;
        std::stringstream ss(contentLength);
        ss >> bodySize;
        
        if (serverConfig.get_client_max_body_size() > 0 && 
            bodySize > serverConfig.get_client_max_body_size()) {
            setErrorResponse(request, 413, "Request Entity Too Large");
            return;
        }
    }
    
    std::string location = request->GetLocation();
    
    std::cout << "POST request to: " << location << std::endl;
    std::cout << "Content-Type: " << contentType << std::endl;
    std::cout << "Body size: " << body.size() << " bytes" << std::endl;
    
    // Handle streaming upload files
    if (body.find("__STREAMING_UPLOAD_FILE:") != std::string::npos) {
        std::string marker = "__STREAMING_UPLOAD_FILE:";
        size_t marker_pos = body.find(marker);
        std::string file_path = body.substr(marker_pos + marker.length());
        
        // Check if this is a duplicate processing attempt
        static std::set<std::string> processed_files;
        if (processed_files.find(file_path) != processed_files.end()) {
            std::cout << "File already processed, skipping duplicate processing: " << file_path << std::endl;
            std::stringstream ss;
            ss << "File uploaded successfully: " << file_path;
            setSuccessResponse(request, ss.str());
            return;
        }
        
        // Process the upload
        handleStreamingUpload(request, file_path);
        
        // Mark this file as processed to prevent duplicate processing
        processed_files.insert(file_path);
        return;
    }
    
    if (body.empty()) {
        setErrorResponse(request, 400, "Empty request body");
        return;
    }
    
    // Handle different content types
    if (contentType.find("multipart/form-data") != std::string::npos) {
        std::string boundary = extractBoundary(contentType);
        if (!boundary.empty()) {
            handleMultipartForm(request, boundary);
        } else {
            setErrorResponse(request, 400, "Invalid multipart boundary");
        }
    } 
    else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        handleUrlEncodedForm(request);
    }
    else if (contentType.find("text/plain") != std::string::npos) {
        handlePlainText(request);
    }
    else if (contentType.find("application/json") != std::string::npos) {
        handleJsonData(request);
    }
    else {
        // More detailed error message for unsupported content types
        std::cout << "Unsupported content type: " << contentType << std::endl;
        setErrorResponse(request, 415, "Unsupported Media Type. Supported types: multipart/form-data, application/x-www-form-urlencoded, text/plain, application/json");
    }
}

void Post::handleUrlEncodedForm(HttpRequest *request) {
    std::cout << "Handling URL-encoded form data" << std::endl;
    
    std::string body = request->GetBody();
    std::cout << "Form data: " << body << std::endl;
    
    std::map<std::string, std::string> formData = parseUrlEncodedForm(body);
    
    std::stringstream response;
    response << "<!DOCTYPE html><html><head><title>Form Submitted</title>";
    response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
    response << "h1{color:#2c3e50}ul{background:#f8f9fa;padding:20px;border-radius:8px}";
    response << "li{margin:10px 0}strong{color:#3498db}";
    response << "a{display:inline-block;margin-top:20px;background:#3498db;color:white;padding:10px 15px;text-decoration:none;border-radius:4px}</style>";
    response << "</head><body>";
    response << "<h1>✅ Form Data Received</h1><ul>";
    
    for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
         it != formData.end(); ++it) {
        response << "<li><strong>" << htmlEscape(it->first) << ":</strong> " 
                << htmlEscape(it->second) << "</li>";
    }
    
    response << "</ul><p><a href=\"/\">Back to Home</a></p></body></html>";
    setSuccessResponse(request, response.str());
    
    std::cout << "URL-encoded form processed successfully" << std::endl;
}

void Post::handlePlainText(HttpRequest *request) {
    std::cout << "Handling plain text data" << std::endl;
    
    std::string body = request->GetBody();
    std::cout << "Text data: " << body << std::endl;
    
    std::stringstream response;
    response << "<!DOCTYPE html><html><head><title>Text Received</title>";
    response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
    response << "h1{color:#2c3e50}.text-content{background:#f8f9fa;padding:20px;border-radius:8px;white-space:pre-wrap}";
    response << "a{display:inline-block;margin-top:20px;background:#3498db;color:white;padding:10px 15px;text-decoration:none;border-radius:4px}</style>";
    response << "</head><body>";
    response << "<h1>✅ Text Data Received</h1>";
    response << "<div class=\"text-content\">" << htmlEscape(body) << "</div>";
    response << "<p><strong>Content Length:</strong> " << body.size() << " characters</p>";
    response << "<p><a href=\"/\">Back to Home</a></p></body></html>";
    
    setSuccessResponse(request, response.str());
    
    std::cout << "Plain text processed successfully" << std::endl;
}

void Post::handleJsonData(HttpRequest *request) {
    std::cout << "Handling JSON data" << std::endl;
    
    std::string body = request->GetBody();
    std::cout << "JSON data: " << body << std::endl;
    
    std::stringstream response;
    response << "<!DOCTYPE html><html><head><title>JSON Received</title>";
    response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
    response << "h1{color:#2c3e50}.json-content{background:#f8f9fa;padding:20px;border-radius:8px;white-space:pre-wrap;font-family:monospace}";
    response << "a{display:inline-block;margin-top:20px;background:#3498db;color:white;padding:10px 15px;text-decoration:none;border-radius:4px}</style>";
    response << "</head><body>";
    response << "<h1>✅ JSON Data Received</h1>";
    response << "<div class=\"json-content\">" << htmlEscape(body) << "</div>";
    response << "<p><strong>Content Length:</strong> " << body.size() << " bytes</p>";
    response << "<p><a href=\"/\">Back to Home</a></p></body></html>";
    
    setSuccessResponse(request, response.str());
    
    std::cout << "JSON data processed successfully" << std::endl;
}

// Add this debug code to your Post.cpp in handleMultipartForm method:

void Post::handleMultipartForm(HttpRequest *request, const std::string &boundary) {
    std::string body = request->GetBody();
    std::cout << "handleMultipartForm called with boundary: " << boundary << std::endl;
    std::cout << "Body size for parsing: " << body.size() << " bytes" << std::endl;
    
    // DEBUG: Print the first 500 characters of the body to see the structure
    std::cout << "=== BODY PREVIEW (first 500 chars) ===" << std::endl;
    std::string preview = body.substr(0, std::min((size_t)500, body.size()));
    for (size_t i = 0; i < preview.length(); ++i) {
        char c = preview[i];
        if (c == '\r') std::cout << "\\r";
        else if (c == '\n') std::cout << "\\n";
        else if (c >= 32 && c <= 126) std::cout << c;
        else std::cout << "[" << (int)c << "]";
    }
    std::cout << std::endl << "=== END BODY PREVIEW ===" << std::endl;
    
    // DEBUG: Check what the boundary looks like
    std::cout << "Boundary: '" << boundary << "'" << std::endl;
    std::cout << "Looking for: '--" << boundary << "'" << std::endl;
    
    // DEBUG: Check if boundary exists in body
    std::string fullBoundary = "--" + boundary;
    size_t boundaryPos = body.find(fullBoundary);
    if (boundaryPos != std::string::npos) {
        std::cout << "Found boundary at position: " << boundaryPos << std::endl;
    } else {
        std::cout << "ERROR: Boundary not found in body!" << std::endl;
        std::cout << "Searching for variations..." << std::endl;
        
        // Try different boundary formats
        if (body.find(boundary) != std::string::npos) {
            std::cout << "Found boundary without '--' prefix" << std::endl;
        }
        if (body.find("\r\n--" + boundary) != std::string::npos) {
            std::cout << "Found boundary with \\r\\n-- prefix" << std::endl;
        }
        if (body.find("\n--" + boundary) != std::string::npos) {
            std::cout << "Found boundary with \\n-- prefix" << std::endl;
        }
        
        // Print first few boundary-like strings found in body
        std::cout << "Looking for any '--' sequences in body:" << std::endl;
        size_t pos = 0;
        int count = 0;
        while ((pos = body.find("--", pos)) != std::string::npos && count < 3) {
            size_t endPos = std::min(pos + 50, body.length());
            std::string boundaryCandidate = body.substr(pos, endPos - pos);
            std::cout << "Found '--' at position " << pos << ": '";
            for (size_t i = 0; i < boundaryCandidate.length(); ++i) {
                char c = boundaryCandidate[i];
                if (c == '\r') std::cout << "\\r";
                else if (c == '\n') std::cout << "\\n";
                else if (c >= 32 && c <= 126) std::cout << c;
                else break;
            }
            std::cout << "'" << std::endl;
            pos++;
            count++;
        }
    }
    
    std::vector<FormPart> parts = parseMultipartForm(body, boundary);
    
    if (parts.empty()) {
        std::cout << "ERROR: parseMultipartForm returned empty parts!" << std::endl;
        setErrorResponse(request, 400, "No valid parts found in multipart form data");
        return;
    }
    
    // Rest of your existing code...
}

void Post::handleStreamingUpload(HttpRequest *request, const std::string &file_path) {
    std::cout << "Handling streaming upload from: " << file_path << std::endl;
    
    // Check if this upload has already been processed
    std::string alreadyProcessed = request->GetHeader("X-Upload-Processed");
    if (alreadyProcessed == "true") {
        std::cout << "Upload already processed, skipping duplicate processing" << std::endl;
        setSuccessResponse(request, "File uploaded successfully");
        return;
    }
    
    // Check if temp file exists
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == -1) {
        std::cerr << "Upload file not found: " << file_path << " (errno: " << errno << ")" << std::endl;
        setErrorResponse(request, 500, "Upload file not found or inaccessible");
        return;
    }
    
    // Verify file size against Content-Length
    std::string contentLength = request->GetHeader("Content-Length");
    if (!contentLength.empty()) {
        size_t expectedSize = 0;
        std::stringstream ss(contentLength);
        ss >> expectedSize;
        
        std::cout << "Verifying file size: expected " << expectedSize 
                 << " bytes, actual " << file_stat.st_size << " bytes" << std::endl;
        
        // For multipart uploads, the actual file size will be smaller than Content-Length
        // due to headers and boundaries, so we allow some flexibility
        size_t minAcceptableSize = expectedSize * 0.85; // Allow up to 15% overhead for multipart
        
        if (file_stat.st_size < minAcceptableSize) {
            std::cerr << "File size mismatch: expected at least " << minAcceptableSize 
                     << " bytes, got " << file_stat.st_size << " bytes" << std::endl;
            std::cerr << "Upload appears to be incomplete. Waiting for more data..." << std::endl;
            
            // Wait for up to 30 seconds for the file to grow
            int wait_attempts = 30;
            bool file_grew = false;
            
            while (wait_attempts > 0) {
                sleep(1); // Wait 1 second
                struct stat new_stat;
                if (stat(file_path.c_str(), &new_stat) == 0) {
                    if (new_stat.st_size > file_stat.st_size) {
                        std::cout << "File grew to " << new_stat.st_size << " bytes" << std::endl;
                        file_stat = new_stat;
                        file_grew = true;
                        if (file_stat.st_size >= minAcceptableSize) {
                            std::cout << "File reached acceptable size" << std::endl;
                            break; // File is now large enough
                        }
                    } else if (file_grew) {
                        // If file grew before but has stopped growing, wait a bit longer
                        // but reduce the number of remaining attempts
                        wait_attempts -= 3;
                    }
                }
                wait_attempts--;
            }
            
            // Final check after waiting
            if (file_stat.st_size < minAcceptableSize) {
                std::cerr << "Upload incomplete after waiting. Expected at least " << minAcceptableSize 
                         << " bytes, got only " << file_stat.st_size << " bytes (" 
                         << (file_stat.st_size * 100 / expectedSize) << "%)" << std::endl;
                setErrorResponse(request, 400, "Upload file is incomplete or corrupted after waiting");
                return;
            }
        }
    }
    
    // Extract original filename and extension from multipart form data
    std::string content_type = request->GetHeader("Content-Type");
    std::string file_extension = ".bin"; // Default extension
    std::string original_filename;
    
    // First check if we can extract the original filename from the request
    if (content_type.find("multipart/form-data") != std::string::npos) {
        // Try to read the first few KB of the file to extract the filename and locate the actual file content
        int source_fd = open(file_path.c_str(), O_RDONLY);
        if (source_fd >= 0) {
            char buffer[16384]; // Read first 16KB to find Content-Disposition header and content start
            ssize_t bytes_read = read(source_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string header_data(buffer);
                
                // Look for Content-Disposition header with filename
                size_t filename_pos = header_data.find("filename=\"");
                if (filename_pos != std::string::npos) {
                    filename_pos += 10; // Skip "filename=\""
                    size_t filename_end = header_data.find("\"", filename_pos);
                    if (filename_end != std::string::npos) {
                        original_filename = header_data.substr(filename_pos, filename_end - filename_pos);
                        std::cout << "Original filename detected: " << original_filename << std::endl;
                        
                        // Extract extension from original filename
                        size_t ext_pos = original_filename.rfind(".");
                        if (ext_pos != std::string::npos) {
                            file_extension = original_filename.substr(ext_pos);
                            std::cout << "Using file extension: " << file_extension << std::endl;
                        }
                    }
                }
            }
            close(source_fd); // Make sure to close the file descriptor
        }
    }
    
    if (file_extension == ".bin") {
        if (content_type.find("application/pdf") != std::string::npos) {
            file_extension = ".pdf";
        } else if (content_type.find("image/jpeg") != std::string::npos) {
            file_extension = ".jpg";
        } else if (content_type.find("image/png") != std::string::npos) {
            file_extension = ".png";
        } else if (content_type.find("text/plain") != std::string::npos) {
            file_extension = ".txt";
        } else if (content_type.find("application/zip") != std::string::npos) {
            file_extension = ".zip";
        } else if (content_type.find("application/octet-stream") != std::string::npos) {
            if (!original_filename.empty()) {
                size_t ext_pos = original_filename.rfind(".");
                if (ext_pos != std::string::npos) {
                    file_extension = original_filename.substr(ext_pos);
                }
            }
        }
    }
    
    std::string uploads_dir = getUploadsDirectory();
    std::string unique_filename;
    if (!original_filename.empty()) {
        unique_filename = generateUniqueFilename(original_filename);
    } else {
        unique_filename = generateUniqueFilename("upload" + file_extension);
    }
    std::string dest_path = uploads_dir + "/" + unique_filename;
    
    off_t content_offset = 0;
    size_t content_length = file_stat.st_size;
    
    if (content_type.find("multipart/form-data") != std::string::npos) {
        int header_fd = open(file_path.c_str(), O_RDONLY);
        if (header_fd < 0) {
            std::cerr << "Failed to open source file for header analysis: " << strerror(errno) << std::endl;
            setErrorResponse(request, 500, "Failed to process uploaded file");
            return;
        }
        
        char header_buffer[32768]; // 32KB 
        ssize_t header_bytes_read = read(header_fd, header_buffer, sizeof(header_buffer) - 1);
        close(header_fd);
        
        if (header_bytes_read > 0) {
            header_buffer[header_bytes_read] = '\0';
            std::string header_data(header_buffer, header_bytes_read);
            
            // Find the double CRLF that separates headers from content
            size_t content_start = header_data.find("\r\n\r\n");
            if (content_start != std::string::npos) {
                // Skip the double CRLF
                content_offset = content_start + 4;
                
                // Find the boundary string
                size_t boundary_pos = content_type.find("boundary=");
                if (boundary_pos != std::string::npos) {
                    boundary_pos += 9; // Skip "boundary="
                    std::string boundary;
                    if (content_type[boundary_pos] == '"') {
                        // Quoted boundary
                        boundary_pos++;
                        size_t boundary_end = content_type.find('"', boundary_pos);
                        if (boundary_end != std::string::npos) {
                            boundary = content_type.substr(boundary_pos, boundary_end - boundary_pos);
                        }
                    } else {
                        // Unquoted boundary
                        size_t boundary_end = content_type.find(';', boundary_pos);
                        if (boundary_end == std::string::npos) {
                            boundary_end = content_type.length();
                        }
                        boundary = content_type.substr(boundary_pos, boundary_end - boundary_pos);
                    }
                    
                    if (!boundary.empty()) {
                        std::string end_boundary = "\r\n--" + boundary + "--";
                        size_t end_pos = header_data.find(end_boundary);
                        if (end_pos != std::string::npos) {
                            // Adjust content length to exclude the end boundary
                            content_length = end_pos - content_offset;
                        } else {
                            // If we can't find the end boundary in our buffer, we'll need to
                            // subtract an estimate for the end boundary from the total file size
                            content_length = file_stat.st_size - content_offset - (end_boundary.length() + 10);
                        }
                    }
                }
                
                std::cout << "Multipart form data: content starts at offset " << content_offset 
                          << ", estimated content length: " << content_length << std::endl;
            }
        }
    }
    
    int source_fd = open(file_path.c_str(), O_RDONLY);
    if (source_fd < 0) {
        std::cerr << "Failed to open source file: " << strerror(errno) << std::endl;
        setErrorResponse(request, 500, "Failed to process uploaded file");
        return;
    }
    
    if (content_offset > 0) {
        if (lseek(source_fd, content_offset, SEEK_SET) != content_offset) {
            std::cerr << "Failed to seek to content start: " << strerror(errno) << std::endl;
            close(source_fd);
            setErrorResponse(request, 500, "Failed to process uploaded file");
            return;
        }
    }
    
    std::cout << "Copying file from " << file_path << " to " << dest_path 
              << " with extension " << file_extension 
              << " (offset: " << content_offset << ", length: " << content_length << ")" << std::endl;
    
    int dest_fd = open(dest_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        std::cerr << "Failed to create destination file: " << dest_path 
                 << " (errno: " << errno << ")" << std::endl;
        close(source_fd);
        setErrorResponse(request, 500, "Failed to save upload");
        return;
    }
    
    // Copy file contents
    char buffer[8192];
    ssize_t bytes_read, bytes_written;
    size_t total_written = 0;
    size_t remaining_content = content_length;
    bool copy_success = true;
    
    while (remaining_content > 0 && (bytes_read = read(source_fd, buffer, std::min(sizeof(buffer), remaining_content))) > 0) {
        char* ptr = buffer;
        size_t remaining_chunk = bytes_read;
        
        while (remaining_chunk > 0) {
            bytes_written = write(dest_fd, ptr, remaining_chunk);
            if (bytes_written <= 0) {
                std::cerr << "Write error: " << strerror(errno) << std::endl;
                copy_success = false;
                break;
            }
            ptr += bytes_written;
            remaining_chunk -= bytes_written;
            total_written += bytes_written;
            remaining_content -= bytes_written;
        }
        
        if (!copy_success) break;
    }
    
    close(source_fd);
    close(dest_fd);
    
    // Delete the temporary file
    unlink(file_path.c_str());
    
    if (copy_success && bytes_read >= 0) {
        std::cout << "Large file upload successful: " << dest_path 
                  << " (" << total_written << " bytes)" << std::endl;
        
        // Mark this upload as processed to prevent duplicate processing
        request->SetHeader("X-Upload-Processed", "true");
        
        // Create success response
        std::stringstream response;
        response << "<!DOCTYPE html><html><head><title>Upload Success</title>";
        response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
        response << "h1{color:#2c3e50}strong{color:#3498db}.success{color:#27ae60;font-weight:bold}";
        response << "a{display:inline-block;margin-top:20px;background:#3498db;color:white;padding:10px 15px;text-decoration:none;border-radius:4px}</style>";
        response << "</head><body>";
        response << "<h1><span class='success'>✅</span> File Upload Successful!</h1>";
        response << "<p><strong>File Type:</strong> " << content_type << "</p>";
        response << "<p><strong>File Size:</strong> " << formatFileSize(total_written) << "</p>";
        response << "<p><strong>Saved As:</strong> " << unique_filename << "</p>";
        response << "<p><strong>Upload Method:</strong> Memory-efficient streaming</p>";
        response << "<p><a href=\"/\">← Back to Home</a></p>";
        response << "</body></html>";
        
        setSuccessResponse(request, response.str());
    } else {
        std::cerr << "Failed to copy file: " << strerror(errno) << std::endl;
        setErrorResponse(request, 500, "Failed to save upload: " + std::string(strerror(errno)));
    }
}

std::string Post::formatFileSize(size_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int suffixIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && suffixIndex < 4) {
        size /= 1024.0;
        suffixIndex++;
    }
    
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << size << " " << suffixes[suffixIndex];
    if (suffixIndex > 0) {
        ss << " (" << bytes << " bytes)";
    }
    return ss.str();
}

bool Post::saveFileInChunks(const std::string &content, const std::string &filename) {
    std::string uploadsDir = getUploadsDirectory();
    std::string fullPath = uploadsDir + "/" + filename;
    
    std::cout << "Attempting to save file to: " << fullPath << std::endl;
    
    // Use low-level file operations for better control
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << "Failed to open file for writing: " << fullPath 
                 << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    
    size_t totalSize = content.size();
    size_t bytesWritten = 0;
    size_t chunkSize = CHUNK_SIZE;
    
    std::cout << "Writing " << totalSize << " bytes in chunks of " << chunkSize << std::endl;
    
    while (bytesWritten < totalSize) {
        size_t remainingBytes = totalSize - bytesWritten;
        size_t currentChunkSize = (remainingBytes < chunkSize) ? remainingBytes : chunkSize;
        
        ssize_t written = write(fd, content.c_str() + bytesWritten, currentChunkSize);
        
        if (written < 0) {
            std::cerr << "Error writing to file at position " << bytesWritten 
                     << " (errno: " << errno << ")" << std::endl;
            close(fd);
            return false;
        }
        
        bytesWritten += written;
    }
    
    close(fd);
    
    // Verify file was written correctly
    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0) {
        if (static_cast<size_t>(st.st_size) == totalSize) {
            std::cout << "Successfully wrote " << totalSize << " bytes to " << fullPath << std::endl;
            return true;
        } else {
            std::cerr << "File size mismatch: expected " << totalSize 
                     << " bytes, got " << st.st_size << " bytes" << std::endl;
        }
    } else {
        std::cerr << "Failed to stat file after writing: " << fullPath 
                 << " (errno: " << errno << ")" << std::endl;
    }
    
    return false;
}

std::string Post::generateUniqueFilename(const std::string &originalName) {
    std::time_t now = std::time(NULL);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    
    // Extract base filename without path
    std::string baseName = originalName;
    size_t lastSlash = originalName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseName = originalName.substr(lastSlash + 1);
    }
    
    // Remove potentially problematic characters
    std::string sanitizedName = baseName;
    for (size_t i = 0; i < sanitizedName.length(); i++) {
        char c = sanitizedName[i];
        if (c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') {
            sanitizedName[i] = '_';
        }
    }
    
    std::stringstream ss;
    ss << timestamp << "_" << (rand() % 10000);
    
    // If we have a valid filename, add it to make the file more recognizable
    if (!sanitizedName.empty()) {
        ss << "_" << sanitizedName;
    }
    
    return ss.str();
}

std::string Post::getUploadsDirectory() {
    static std::string uploadsDir;
    if (uploadsDir.empty()) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            uploadsDir = std::string(cwd) + "/uploads";
        } else {
            uploadsDir = "uploads";
        }
        
        std::cout << "Uploads directory: " << uploadsDir << std::endl;
        
        // Create directory if it doesn't exist
        struct stat st;
        if (stat(uploadsDir.c_str(), &st) == -1) {
            std::cout << "Creating uploads directory: " << uploadsDir << std::endl;
            if (mkdir(uploadsDir.c_str(), 0755) != 0) {
                std::cerr << "Failed to create uploads directory: " << uploadsDir 
                         << " (errno: " << errno << ")" << std::endl;
            } else {
                std::cout << "Successfully created uploads directory" << std::endl;
            }
        } else {
            std::cout << "Uploads directory already exists" << std::endl;
        }
    }
    return uploadsDir;
}

std::string Post::extractBoundary(const std::string &contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(boundaryPos + 9);
    
    // Handle quoted boundary
    if (!boundary.empty() && boundary[0] == '"') {
        size_t endQuote = boundary.find('"', 1);
        if (endQuote != std::string::npos) {
            boundary = boundary.substr(1, endQuote - 1);
        }
    } else {
        // Handle unquoted boundary (might contain semicolons or other delimiters)
        size_t delimPos = boundary.find_first_of(" ;,");
        if (delimPos != std::string::npos) {
            boundary = boundary.substr(0, delimPos);
        }
    }
    
    return boundary;
}

std::map<std::string, std::string> Post::parseUrlEncodedForm(const std::string &body) {
    std::map<std::string, std::string> result;
    std::istringstream stream(body);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            result[key] = value;
        }
    }
    return result;
}

std::string Post::urlDecode(const std::string &encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            std::stringstream ss;
            ss << std::hex << hex;
            int c;
            ss >> c;
            result += static_cast<char>(c);
            i += 2;
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

void Post::setSuccessResponse(HttpRequest *request, const std::string &message) {
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(message);
}

void Post::setErrorResponse(HttpRequest *request, int statusCode, const std::string &message) {
    std::stringstream response;
    response << "<!DOCTYPE html><html><head><title>Error " << statusCode << "</title></head><body>";
    response << "<h1>Error " << statusCode << "</h1>";
    response << "<p>" << htmlEscape(message) << "</p>";
    response << "<p><a href=\"/\">Back to Home</a></p>";
    response << "</body></html>";
    
    request->GetClientDatat()->http_response->setStatusCode(statusCode);
    request->GetClientDatat()->http_response->setStatusMessage(message);
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(response.str());
}

std::string Post::htmlEscape(const std::string &input) {
    std::string result;
    for (std::string::const_iterator it = input.begin(); it != input.end(); ++it) {
        switch (*it) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += *it; break;
        }
    }
    return result;
}