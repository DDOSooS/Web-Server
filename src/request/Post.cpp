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

void Post::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) {
    std::string body = request->GetBody();
    std::string contentType = request->GetHeader("Content-Type");
    std::string contentLength = request->GetHeader("Content-Length");
    
    // Check body size limit
    if (!contentLength.empty()) {
        size_t bodySize = 0;
        std::stringstream ss(contentLength);
        ss >> bodySize;
        
        if (clientConfig.get_client_max_body_size() > 0 && 
            bodySize > clientConfig.get_client_max_body_size()) {
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
    
    // Flag to track if we've already processed the upload
    bool fileProcessed = false;
    
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
    
    // Special handling for small files
    if (body.size() < 1024 * 10) { // Less than 10KB
        std::cout << "Small file detected, using enhanced parsing for small files" << std::endl;
        
        // Check if this is a simple form with just one file
        size_t contentDispositionPos = body.find("Content-Disposition:");
        size_t filenamePos = body.find("filename=\"");
        
        if (contentDispositionPos != std::string::npos && filenamePos != std::string::npos) {
            filenamePos += 10; // Skip "filename=""
            size_t filenameEnd = body.find("\"", filenamePos);
            
            if (filenameEnd != std::string::npos) {
                std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);
                std::cout << "Detected filename in small file: " << filename << std::endl;
                
                // Find the start of the file content (after the double CRLF)
                size_t contentStart = body.find("\r\n\r\n", filenameEnd);
                if (contentStart == std::string::npos) {
                    contentStart = body.find("\n\n", filenameEnd);
                    if (contentStart != std::string::npos) {
                        contentStart += 2; // Skip \n\n
                    }
                } else {
                    contentStart += 4; // Skip \r\n\r\n
                }
                
                if (contentStart != std::string::npos) {
                    // Find the end boundary
                    std::string endBoundary = "--" + boundary + "--";
                    size_t contentEnd = body.find(endBoundary, contentStart);
                    
                    if (contentEnd == std::string::npos) {
                        // Try without the trailing --
                        endBoundary = "--" + boundary;
                        contentEnd = body.find(endBoundary, contentStart);
                    }
                    
                    if (contentEnd != std::string::npos) {
                        // Extract the file content
                        std::string fileContent = body.substr(contentStart, contentEnd - contentStart);
                        
                        // Create a form part manually
                        FormPart part;
                        part.name = "upload_file"; // Default name
                        part.filename = filename;
                        part.isFile = true;
                        part.body = fileContent;
                        
                        // Find content type
                        size_t contentTypePos = body.find("Content-Type:", contentDispositionPos);
                        if (contentTypePos != std::string::npos && contentTypePos < contentStart) {
                            contentTypePos += 13; // Skip "Content-Type:"
                            size_t contentTypeEnd = body.find("\r\n", contentTypePos);
                            if (contentTypeEnd == std::string::npos) {
                                contentTypeEnd = body.find("\n", contentTypePos);
                            }
                            
                            if (contentTypeEnd != std::string::npos) {
                                part.contentType = body.substr(contentTypePos, contentTypeEnd - contentTypePos);
                                // Trim whitespace
                                part.contentType.erase(0, part.contentType.find_first_not_of(" \t"));
                                part.contentType.erase(part.contentType.find_last_not_of(" \t") + 1);
                            }
                        }
                        
                        std::vector<FormPart> manualParts;
                        manualParts.push_back(part);
                        
                        std::cout << "Manually created form part for small file:" << std::endl;
                        std::cout << "  Name: " << part.name << std::endl;
                        std::cout << "  Filename: " << part.filename << std::endl;
                        std::cout << "  Content-Type: " << part.contentType << std::endl;
                        std::cout << "  Content size: " << part.body.size() << " bytes" << std::endl;
                        
                        // Process the manually created parts
                        processFormParts(request, manualParts);
                        fileProcessed = true;
                    }
                }
            }
        }
    }
    
    // Only proceed with regular parsing if we haven't already processed the file
    if (!fileProcessed) {
        std::vector<FormPart> parts = parseMultipartForm(body, boundary);
        
        if (parts.empty()) {
            std::cout << "ERROR: parseMultipartForm returned empty parts!" << std::endl;
            setErrorResponse(request, 400, "No valid parts found in multipart form data");
            return;
        }
        
        processFormParts(request, parts);
    }
}

// Process form parts and handle file uploads
void Post::processFormParts(HttpRequest *request, const std::vector<FormPart> &parts) {
    std::stringstream response;
    response << "<!DOCTYPE html><html><head><title>Upload Results</title>";
    response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
    response << "table{width:100%;border-collapse:collapse;margin:20px 0}";
    response << "th,td{border:1px solid #ddd;padding:8px;text-align:left}";
    response << "th{background-color:#f2f2f2}";
    response << "tr:nth-child(even){background-color:#f9f9f9}";
    response << "</style></head><body>";
    response << "<h1>Upload Results</h1>";
    
    if (parts.empty()) {
        response << "<p>No files were uploaded.</p>";
    } else {
        response << "<table><tr><th>Field Name</th><th>File Name</th><th>Size</th><th>Status</th></tr>";
        
        std::string uploadsDir = getUploadsDirectory();
        
        for (size_t i = 0; i < parts.size(); ++i) {
            const FormPart &part = parts[i];
            
            response << "<tr>";
            response << "<td>" << htmlEscape(part.name) << "</td>";
            
            if (part.isFile && !part.filename.empty()) {
                // Generate a unique filename
                std::string uniqueFilename = generateUniqueFilename(part.filename);
                
                // Save the file - pass only the filename to saveFileInChunks which already adds the uploads directory
                bool saveSuccess = saveFileInChunks(part.body, uniqueFilename);
                
                if (saveSuccess) {
                    response << "<td>" << htmlEscape(part.filename) << "</td>";
                    response << "<td>" << formatFileSize(part.body.size()) << "</td>";
                    response << "<td>Uploaded successfully as " << htmlEscape(uniqueFilename) << "</td>";
                } else {
                    response << "<td>" << htmlEscape(part.filename) << "</td>";
                    response << "<td>" << formatFileSize(part.body.size()) << "</td>";
                    response << "<td>Failed to save file</td>";
                }
            } else {
                // Regular form field
                response << "<td>Not a file</td>";
                response << "<td>" << part.body.size() << " bytes</td>";
                response << "<td>Form field value: " << htmlEscape(part.body) << "</td>";
            }
            
            response << "</tr>";
        }
        
        response << "</table>";
    }
    
    response << "<p><a href=\"/\">Back to Home</a></p>";
    response << "</body></html>";
    
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(response.str());
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
    
    std::cout << "Temp file found with size: " << file_stat.st_size << " bytes" << std::endl;
    
    // Verify file size against Content-Length (but don't be too strict)
    std::string contentLength = request->GetHeader("Content-Length");
    if (!contentLength.empty()) {
        size_t expectedSize = 0;
        std::stringstream ss(contentLength);
        ss >> expectedSize;
        
        std::cout << "Verifying file size: expected " << expectedSize 
                 << " bytes, actual " << file_stat.st_size << " bytes" << std::endl;
        
        // FIXED: More lenient size checking
        if (file_stat.st_size > 0) {
            double percent_complete = (file_stat.st_size * 100.0 / expectedSize);
            std::cout << "Upload completeness: " << percent_complete << "%" << std::endl;
            
            // FIXED: Only reject if file is suspiciously small (less than 50% of expected)
            if (percent_complete < 50.0 && file_stat.st_size < 1024) {
                std::cout << "File appears incomplete and very small, waiting for more data" << std::endl;
                return;
            }
        }
        
        // If we reach here, process the file regardless of size percentage
        std::cout << "Proceeding with file processing" << std::endl;
    }
    
    // Extract original filename and extension from multipart form data
    std::string content_type = request->GetHeader("Content-Type");
    std::string file_extension = ".bin"; // Default extension
    std::string original_filename;
    
    // Read the first part of the file to extract metadata
    if (content_type.find("multipart/form-data") != std::string::npos) {
        int source_fd = open(file_path.c_str(), O_RDONLY);
        if (source_fd >= 0) {
            char buffer[16384]; // Read first 16KB to find Content-Disposition header
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
            close(source_fd);
        }
    }
    
    // Fallback extension detection based on content type
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
        }
    }
    
    // Generate destination path
    std::string uploads_dir = getUploadsDirectory();
    std::string unique_filename;
    if (!original_filename.empty()) {
        unique_filename = generateUniqueFilename(original_filename);
    } else {
        unique_filename = generateUniqueFilename("upload" + file_extension);
    }
    std::string dest_path = uploads_dir + "/" + unique_filename;
    
    // Calculate content offset for multipart data
    off_t content_offset = 0;
    size_t content_length = file_stat.st_size;
    
    if (content_type.find("multipart/form-data") != std::string::npos) {
        int header_fd = open(file_path.c_str(), O_RDONLY);
        if (header_fd >= 0) {
            char header_buffer[32768]; // 32KB 
            ssize_t header_bytes_read = read(header_fd, header_buffer, sizeof(header_buffer) - 1);
            close(header_fd);
            
            if (header_bytes_read > 0) {
                header_buffer[header_bytes_read] = '\0';
                std::string header_data(header_buffer, header_bytes_read);
                
                // Find the double CRLF that separates headers from content
                size_t content_start = header_data.find("\r\n\r\n");
                if (content_start != std::string::npos) {
                    content_offset = content_start + 4;
                    std::cout << "Multipart content starts at offset: " << content_offset << std::endl;
                    
                    // Estimate content length (subtract headers and boundary footer)
                    // For streaming uploads, we'll be more conservative with boundary detection
                    content_length = file_stat.st_size - content_offset;
                    
                    // Try to subtract the approximate size of the closing boundary
                    if (content_length > 100) {
                        content_length -= 50; // Conservative estimate for boundary footer
                    }
                    
                    std::cout << "Estimated actual content length: " << content_length << std::endl;
                }
            }
        }
    }
    
    // Open source file
    int source_fd = open(file_path.c_str(), O_RDONLY);
    if (source_fd < 0) {
        std::cerr << "Failed to open source file: " << strerror(errno) << std::endl;
        setErrorResponse(request, 500, "Failed to process uploaded file");
        return;
    }
    
    // Seek to content start if needed
    if (content_offset > 0) {
        if (lseek(source_fd, content_offset, SEEK_SET) != content_offset) {
            std::cerr << "Failed to seek to content start: " << strerror(errno) << std::endl;
            close(source_fd);
            setErrorResponse(request, 500, "Failed to process uploaded file");
            return;
        }
    }
    
    std::cout << "Copying file from " << file_path << " to " << dest_path 
              << " (offset: " << content_offset << ", length: " << content_length << ")" << std::endl;
    
    // Create destination file
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
    
    std::cout << "Starting file copy..." << std::endl;
    
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
        
        // Progress indicator for large files
        if (total_written % (1024 * 1024) == 0 || remaining_content == 0) {
            std::cout << "Copied " << total_written << " / " << content_length << " bytes" << std::endl;
        }
    }
    
    close(source_fd);
    close(dest_fd);
    
    std::cout << "File copy completed. Total written: " << total_written << " bytes" << std::endl;
    
    // Clean up temporary file
    if (unlink(file_path.c_str()) == 0) {
        std::cout << "Temporary file deleted: " << file_path << std::endl;
    } else {
        std::cerr << "Failed to delete temporary file: " << file_path << std::endl;
    }
    
    if (copy_success && bytes_read >= 0 && total_written > 0) {
        std::cout << "✅ UPLOAD SUCCESS: " << dest_path 
                  << " (" << total_written << " bytes)" << std::endl;
        
        // Mark this upload as processed
        request->SetHeader("X-Upload-Processed", "true");
        
        // Create success response
        std::stringstream response;
        response << "<!DOCTYPE html><html><head><title>Upload Success</title>";
        response << "<style>body{font-family:system-ui,sans-serif;max-width:800px;margin:0 auto;padding:20px;line-height:1.6}";
        response << "h1{color:#2c3e50}strong{color:#3498db}.success{color:#27ae60;font-weight:bold}";
        response << "a{display:inline-block;margin-top:20px;background:#3498db;color:white;padding:10px 15px;text-decoration:none;border-radius:4px}</style>";
        response << "</head><body>";
        response << "<h1><span class='success'>✅</span> File Upload Successful!</h1>";
        response << "<p><strong>Original Filename:</strong> " << (original_filename.empty() ? "Unknown" : original_filename) << "</p>";
        response << "<p><strong>Saved As:</strong> " << unique_filename << "</p>";
        response << "<p><strong>File Size:</strong> " << formatFileSize(total_written) << "</p>";
        response << "<p><strong>Content Type:</strong> " << content_type << "</p>";
        response << "<p><strong>Upload Method:</strong> Memory-efficient streaming</p>";
        response << "<p><strong>Saved To:</strong> " << dest_path << "</p>";
        response << "<p><a href=\"/\">← Back to Home</a></p>";
        response << "</body></html>";
        
        setSuccessResponse(request, response.str());
    } else {
        std::cerr << "❌ UPLOAD FAILED: Copy failed or no data written" << std::endl;
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