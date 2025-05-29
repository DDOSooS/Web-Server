#include "../../include/request/Post.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/ClientConnection.hpp" // This should contain ClientData
#include "../../include/response/HttpResponse.hpp"  // And this should contain HttpResponse

#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits>
#include <limits.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <cstring> 

Post::Post() {}
Post::~Post() {}

bool Post::CanHandle(std::string method) {
    return method == "POST";
}

void Post::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig) {
    std::string contentType = request->GetHeader("Content-Type");
    std::string transferEncoding = request->GetHeader("Transfer-Encoding");
    std::string body = request->GetBody();

    std::cout << "POST request to: " << request->GetLocation() << std::endl;
    std::cout << "Content-Type: " << contentType << std::endl;
    std::cout << "Transfer-Encoding: " << transferEncoding << std::endl;
    std::cout << "Body size: " << body.size() << " bytes" << std::endl;

    if (transferEncoding.find("chunked") != std::string::npos) {
        handleChunkedTransfer(request);
        return;
    }

    if (body.empty()) {
        request->GetClientDatat()->http_response->setStatusCode(400);
        request->GetClientDatat()->http_response->setStatusMessage("Bad Request");
        request->GetClientDatat()->http_response->setContentType("text/plain");
        request->GetClientDatat()->http_response->setBuffer("Empty request body");
        return;
    }

    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        handleUrlEncodedForm(request, contentType);
    } else if (contentType.find("multipart/form-data") != std::string::npos) {
        std::string boundary = extractBoundary(contentType);
        if (!boundary.empty()) {
            handleMultipartForm(request, contentType, boundary);
        } else {
            request->GetClientDatat()->http_response->setStatusCode(400);
            request->GetClientDatat()->http_response->setStatusMessage("Bad Request");
            request->GetClientDatat()->http_response->setContentType("text/plain");
            request->GetClientDatat()->http_response->setBuffer("Invalid multipart boundary");
        }
    } else {
        handleRawData(request, contentType);
    }
}

std::string Post::extractBoundary(const std::string &contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    std::string boundary = contentType.substr(boundaryPos + 9);
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1, boundary.find('"', 1) - 1);
    }
    return boundary;
}

void Post::handleUrlEncodedForm(HttpRequest *request, const std::string &contentType) {
    (void)contentType; // Silence unused parameter warning
    std::string body = request->GetBody();
    std::map<std::string, std::string> formData = parseUrlEncodedForm(body);
    std::stringstream responseContent;
    responseContent << "<html><head><title>Form Submission Result</title></head><body>";
    responseContent << "<h1>Form Data Received</h1><ul>";
    for (std::map<std::string, std::string>::const_iterator it = formData.begin(); it != formData.end(); ++it) {
        responseContent << "<li><strong>" << it->first << ":</strong> " << it->second << "</li>";
    }
    responseContent << "</ul></body></html>";
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(responseContent.str());
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

void Post::handleMultipartForm(HttpRequest *request, const std::string &contentType, const std::string &boundary) {
    (void)contentType;
    std::string body = request->GetBody();
    std::string contentLength = request->GetHeader("Content-Length");
    size_t fileSize = 0;

    if (!contentLength.empty()) {
        std::stringstream ss(contentLength);
        ss >> fileSize;
    }

    bool isLargeFile = fileSize > MAX_MEMORY_CHUNK;

    if (isLargeFile) {
        std::cout << "Large file detected " << fileSize << " bytes. Using chunked processing." << std::endl;
    }

    std::vector<FormPart> parts = parseMultipartForm(body, boundary);
    std::stringstream responseContent;
    responseContent << "<html><head><title>Form Submission Result</title></head><body>";
    responseContent << "<h1>Multipart Form Data Received</h1><ul>";

    for (std::vector<FormPart>::iterator it = parts.begin(); it != parts.end(); ++it) {
        if (isLargeFile && !it->filename.empty()) {
            it->isFile = true;
            it->uploadId = initializeFileUpload(it->filename);
            processChunkedFormPart(*it, request, false);
        } else {
            processFormPart(*it, request);
        }

        if (!it->filename.empty()) {
            responseContent << "<li><strong>File:</strong> " << it->name << " (" << it->filename << ") - "
                           << (isLargeFile ? fileSize : it->body.size()) << " bytes";
            if (isLargeFile) {
                responseContent << " (processed in chunks)";
            }
            responseContent << "</li>";
        } else {
            responseContent << "<li><strong>" << it->name << ":</strong> " << it->body << "</li>";
        }
    }

    if (isLargeFile) {
        for (std::vector<FormPart>::const_iterator it2 = parts.begin(); it2 != parts.end(); ++it2) {
            if (it2->isFile && !it2->uploadId.empty()) {
                finalizeFileUpload(it2->uploadId);
            }
        }
    }

    responseContent << "</ul></body></html>";
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(responseContent.str());
}

std::vector<Post::FormPart> Post::parseMultipartForm(const std::string &body, const std::string &boundary) {
    std::vector<FormPart> parts;
    std::string startBoundary = "--" + boundary;
    std::string endBoundary = "--" + boundary + "--";
    size_t pos = body.find(startBoundary);
    while (pos != std::string::npos) {
        size_t boundaryEnd = body.find("\r\n", pos);
        if (boundaryEnd == std::string::npos) break;
        if (body.substr(pos, boundaryEnd - pos) == endBoundary) {
            break;
        }
        size_t nextBoundary = body.find(startBoundary, boundaryEnd);
        if (nextBoundary == std::string::npos) {
            nextBoundary = body.find(endBoundary, boundaryEnd);
            if (nextBoundary == std::string::npos) break;
        }
        std::string partContent = body.substr(boundaryEnd + 2, nextBoundary - boundaryEnd - 4);
        FormPart part;
        part.isFile = false;
        size_t headersEnd = partContent.find("\r\n\r\n");
        if (headersEnd != std::string::npos) {
            std::string headersStr = partContent.substr(0, headersEnd);
            std::istringstream headerStream(headersStr);
            std::string headerLine;
            while (std::getline(headerStream, headerLine)) {
                if (headerLine.empty() || headerLine == "\r") continue;
                size_t colonPos = headerLine.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = headerLine.substr(0, colonPos);
                    std::string value = headerLine.substr(colonPos + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of("\r\n \t") + 1);
                    part.headers[key] = value;
                    if (key == "Content-Disposition") {
                        size_t namePos = value.find("name=\"");
                        if (namePos != std::string::npos) {
                            size_t nameStart = namePos + 6;
                            size_t nameEnd = value.find('"', nameStart);
                            if (nameEnd != std::string::npos) {
                                part.name = value.substr(nameStart, nameEnd - nameStart);
                            }
                        }
                        size_t filenamePos = value.find("filename=\"");
                        if (filenamePos != std::string::npos) {
                            size_t filenameStart = filenamePos + 10;
                            size_t filenameEnd = value.find('"', filenameStart);
                            if (filenameEnd != std::string::npos) {
                                part.filename = value.substr(filenameStart, filenameEnd - filenameStart);
                                part.isFile = true;
                            }
                        }
                    }
                }
            }
            part.body = partContent.substr(headersEnd + 4);
        } else {
            part.body = partContent;
        }
        parts.push_back(part);
        pos = nextBoundary;
    }
    return parts;
}

void Post::processFormPart(const FormPart &part, HttpRequest *request) {
    (void)request;
    if (!part.filename.empty()) {
        std::string uniqueFilename = generateUniqueFilename(part.filename);
        bool success = saveUploadedFile(part.body, uniqueFilename);
        if (!success) {
            std::cerr << "Failed to save uploaded file: " << uniqueFilename << std::endl;
        } else {
            std::cout << "Successfully saved file: " << uniqueFilename << std::endl;
        }
    }
}

void Post::processChunkedFormPart(const FormPart &part, HttpRequest *request, bool isFinalChunk) {
    (void)request;
    if (!part.isFile || part.filename.empty() || part.uploadId.empty()) {
        return;
    }
    bool success = writeFileChunk(part.uploadId, part.body);
    if (!success) {
        std::cerr << "Failed to write chunk for file: " << part.filename << std::endl;
    } else {
        std::cout << "Successfully wrote chunk for file: " << part.filename
                  << " (" << part.body.size() << " bytes)" << std::endl;
    }
    if (isFinalChunk) {
        finalizeFileUpload(part.uploadId);
    }
}

void Post::handleChunkedTransfer(HttpRequest *request) {
    std::string contentType = request->GetHeader("Content-Type");
    std::string body = request->GetBody();
    if (contentType.find("multipart/form-data") != std::string::npos) {
        std::string boundary = extractBoundary(contentType);
        if (!boundary.empty()) {
            handleMultipartForm(request, contentType, boundary);
        } else {
            request->GetClientDatat()->http_response->setStatusCode(400);
            request->GetClientDatat()->http_response->setStatusMessage("Bad Request");
            request->GetClientDatat()->http_response->setContentType("text/plain");
            request->GetClientDatat()->http_response->setBuffer("Invalid multipart boundary");
        }
    } else {
        handleRawData(request, contentType);
    }
}

std::string Post::getUploadsDirectory() {
    static std::string uploadsDir;
    if (uploadsDir.empty()) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            uploadsDir = std::string(cwd) + "/uploads";
        } else {
            uploadsDir = "uploads";
        }
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        if (stat(uploadsDir.c_str(), &st) == -1) {
            if (mkdir(uploadsDir.c_str(), 0755) != 0) {
                std::cerr << "[ERROR] Failed to create uploads directory: " << uploadsDir
                          << " (errno: " << errno << ")" << std::endl;
            }
        }
    }
    return uploadsDir;
}

std::string Post::initializeFileUpload(const std::string &originalFilename) {
    std::stringstream ss;
    ss << std::time(NULL) << "_" << rand();
    std::string uploadId = ss.str();

    FileUpload upload;
    upload.filename = originalFilename;
    upload.bytesReceived = 0;
    upload.complete = false;
    std::string uniqueFilename = generateUniqueFilename(originalFilename);
    std::string uploadsDir = getUploadsDirectory();
    upload.tempPath = uploadsDir + "/temp_" + uniqueFilename;
    upload.finalPath = uploadsDir + "/" + uniqueFilename;

    upload.fileStream = new std::ofstream();
    upload.fileStream->open(upload.tempPath.c_str(), std::ios::binary);

    if (!upload.fileStream->is_open()) {
        std::cerr << "[ERROR] Failed to open file for writing: " << upload.tempPath
                  << " (errno: " << errno << ")" << std::endl;
        delete upload.fileStream;
        upload.fileStream = NULL;
        return "";
    }

    // In C++98, we can't use std::make_pair with a temporary object
    // So we'll just use operator[] directly
    activeUploads[uploadId] = upload;

    return uploadId;
}

bool Post::writeFileChunk(const std::string &uploadId, const std::string &chunk) {
    std::map<std::string, FileUpload>::iterator it = activeUploads.find(uploadId);
    if (it == activeUploads.end()) {
        std::cerr << "[ERROR] Upload ID not found: " << uploadId << std::endl;
        return false;
    }
    FileUpload &upload = it->second;
    if (upload.complete || !upload.fileStream) {
        std::cerr << "Upload already complete or stream not available: " << uploadId << std::endl;
        return false;
    }
    upload.fileStream->write(chunk.c_str(), chunk.size());
    if (!upload.fileStream->good()) {
        std::cerr << "Failed to write chunk to file: " << upload.tempPath << std::endl;
        return false;
    }
    upload.bytesReceived += chunk.size();
    return true;
}

bool Post::finalizeFileUpload(const std::string &uploadId) {
    std::map<std::string, FileUpload>::iterator it = activeUploads.find(uploadId);
    if (it == activeUploads.end()) {
        std::cerr << "[ERROR] Upload ID not found for finalization: " << uploadId << std::endl;
        return false;
    }
    FileUpload &upload = it->second;

    if (upload.fileStream && upload.fileStream->is_open()) {
        upload.fileStream->close();
    }
    
    struct stat st;
    std::memset(&st, 0, sizeof(st));
    if (stat(upload.tempPath.c_str(), &st) == -1) {
        std::cerr << "[ERROR] Temp file does not exist: " << upload.tempPath
                  << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    if (rename(upload.tempPath.c_str(), upload.finalPath.c_str()) != 0) {
        std::cerr << "[ERROR] Failed to move file from " << upload.tempPath
                  << " to " << upload.finalPath
                  << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    upload.complete = true;
    std::cout << "[INFO] Successfully finalized file upload: " << upload.filename
              << " to " << upload.finalPath << std::endl;
    
    activeUploads.erase(it);
    return true;
}

std::string Post::generateUniqueFilename(const std::string &originalName) {
    std::time_t now = std::time(NULL);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&now));
    
    std::stringstream ss;
    ss << buf << "_" << rand() % 10000 << "_" << originalName;
    return ss.str();
}

bool Post::saveUploadedFile(const std::string &content, const std::string &filename) {
    std::string uploadsDir = getUploadsDirectory();
    std::string filePath = uploadsDir + "/" + filename;
    
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file for writing: " << filePath
                  << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

void Post::handleRawData(HttpRequest *request, const std::string &contentType) {
    std::string body = request->GetBody();
    std::stringstream responseContent;
    responseContent << "<html><head><title>Raw Data Received</title></head><body>";
    responseContent << "<h1>Raw Data Received</h1>";
    responseContent << "<p><strong>Content-Type:</strong> " << contentType << "</p>";
    responseContent << "<p><strong>Size:</strong> " << body.size() << " bytes</p>";
    if (contentType.find("text/") != std::string::npos ||
        contentType.find("application/json") != std::string::npos ||
        contentType.find("application/xml") != std::string::npos) {
        responseContent << "<h2>Content Preview:</h2>";
        responseContent << "<pre>";
        if (body.size() > 1000) {
            responseContent << body.substr(0, 1000) << "...";
        } else {
            responseContent << body;
        }
        responseContent << "</pre>";
    }
    responseContent << "</body></html>";
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(responseContent.str());
}