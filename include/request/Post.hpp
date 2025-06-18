#ifndef POST_HPP
#define POST_HPP

#include "RequestHandler.hpp"
#include "HttpRequest.hpp"
#include "../config/ServerConfig.hpp"
#include <map>
#include <vector>
#include <string>
#include <fstream>

#define CHUNK_SIZE 8192  // 8KB chunks for processing

class Post : public RequestHandler
{
private:
    struct FormPart {
        std::string name;
        std::string filename;
        std::string contentType;
        std::string body;
        bool isFile;
        
        FormPart() : isFile(false) {}
    };
    
    // Core handlers - ONLY REQUIRED FOR 42
    void handleUrlEncodedForm(HttpRequest *request);
    void handleMultipartForm(HttpRequest *request, const std::string &boundary);
    void processFormParts(HttpRequest *request, const std::vector<FormPart> &parts);
    void handleStreamingUpload(HttpRequest *request, const std::string &file_path);
    
    void handlePlainText(HttpRequest *request);
    void handleJsonData(HttpRequest *request);
    

    // File operations
    bool saveFileInChunks(const std::string &content, const std::string &filename);
    std::string generateUniqueFilename(const std::string &originalName);
    std::string getUploadsDirectory();
    
    // Parsing utilities
    std::string extractBoundary(const std::string &contentType);
    std::vector<FormPart> parseMultipartForm(const std::string &body, const std::string &boundary);  // Removed duplicate
    std::map<std::string, std::string> parseUrlEncodedForm(const std::string &body);
    std::string urlDecode(const std::string &encoded);
    
    // Response utilities
    void setSuccessResponse(HttpRequest *request, const std::string &message);
    void setErrorResponse(HttpRequest *request, int statusCode, const std::string &message);
    std::string htmlEscape(const std::string &input);
    std::string formatFileSize(size_t bytes);

public:
    Post();
    ~Post();
    
    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig);
};

#endif