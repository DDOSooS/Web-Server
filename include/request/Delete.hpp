#ifndef DELETE_HPP
#define DELETE_HPP

#include "./RequestHandler.hpp"
#include <string>
#include <map>

class Delete : public RequestHandler {
private:
    std::string htmlEscape(const std::string &input);
    void setErrorResponse(HttpRequest *request, int statusCode, const std::string &statusMessage, const std::string &message);
    bool isPathSafe(const std::string &path);
    bool deleteFile(const std::string &filePath);
    bool deleteDirectory(const std::string &dirPath);
    std::string getUploadsDirectory();
    std::map<std::string, std::string> parseQueryString(const std::string &queryString);
    std::string urlDecode(const std::string &encoded);
    bool isDirectoryEmpty(const std::string &dirPath);

public:
    Delete();
    ~Delete();
    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);
};

#endif