#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "RequestHandler.hpp"
#include "../WebServer.hpp"
#include "../config/ServerConfig.hpp"
#include "HttpRequest.hpp"
#include <string>
#include <vector>

class ClientConnection;
class HttpRequest;
class ServerConfig;

class CgiHandler : public RequestHandler {
private:
    ClientConnection* _client;

public:
    // Constructor and Destructor
    CgiHandler(ClientConnection* client);
    ~CgiHandler();

    // Main handler methods
    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig);

    // CGI-specific methods
    bool isCgiRequest(HttpRequest *request) const;
    std::string getCgiPath(HttpRequest *request) const;
    std::string executeCgiScript(HttpRequest *request);
    
    // Environment setup
    char** setGgiEnv(HttpRequest *request);
    void cleanupEnvironment(char** env);
    
    // Helper methods for path and URL handling
    std::string extractQueryString();
    std::string extractPathInfo(const std::string& url);
    std::string getDirectoryFromPath(const std::string& full_path);
    std::string getFilenameFromPath(const std::string& full_path);
    
    // Validation methods
    bool isFileExecutable(const std::string& file_path);
    bool fileExists(const std::string& file_path);
    bool isValidScriptPath(const std::string& script_path);
    bool isValidInterpreterPath(const std::string& interpreter_path);
    
    // HTTP response parsing
    void parseHttpHeaders(const std::string& cgi_output, std::string& headers, std::string& body);
    void setCgiResponseHeaders(HttpRequest* request, const std::string& headers);
    
    // Security helpers  
    bool isPathTraversalSafe(const std::string& path);
};

#endif // CGIHANDLER_HPP