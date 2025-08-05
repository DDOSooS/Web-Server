#ifndef DELETE_HPP
#define DELETE_HPP

#include "RequestHandler.hpp"
#include "HttpRequest.hpp"
#include "Get.hpp"
#include "../config/ServerConfig.hpp"

class Delete : public Get {
public:
    Delete();
    ~Delete();
    
    // Inherited from RequestHandler
    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);
    void sendSuccessResponse(HttpRequest *request);
    void sendErrorResponse(HttpRequest *request, int statusCode, const std::string &statusMessage, const std::string &errorMessage);
    void handleFileDeletion(HttpRequest *request, const std::string &filePath);
    void handleDirectoryDeletion(HttpRequest *request, const std::string &dirPath, const std::string &originalUri);
    bool deleteDirectoryRecursive(const std::string &dirPath);
};

#endif // DELETE_HPP