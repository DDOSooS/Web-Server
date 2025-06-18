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
    struct CgiProcess {
        pid_t pid;
        int pipe_fd;
        time_t start_time;
        std::string output;
        ClientConnection* client;
        HttpRequest* request;
    };
    
    static std::map<int, CgiProcess> active_cgis;

    CgiHandler(ClientConnection* client);
    ~CgiHandler();

    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);

    bool isCgiRequest(HttpRequest *request) const;
    std::string getCgiPath(HttpRequest *request) const;
    bool startCgiProcess(HttpRequest *request);
    
    char** setGgiEnv(HttpRequest *request);
    void cleanupEnvironment(char** env);
    
    std::string extractPathInfo(const std::string& url);
    std::string getDirectoryFromPath(const std::string& full_path);
    std::string getFilenameFromPath(const std::string& full_path);
    
    bool isFileExecutable(const std::string& file_path);
    bool fileExists(const std::string& file_path);
    bool isValidScriptPath(const std::string& script_path);
    bool isValidInterpreterPath(const std::string& interpreter_path);
    
    void parseHttpHeaders(const std::string& cgi_output, std::string& headers, std::string& body);
    void setCgiResponseHeaders(HttpRequest* request, const std::string& headers);
    
    bool isPathTraversalSafe(const std::string& path);
    template <typename T>
    std::string to_string(const T& value);
};

#endif // CGIHANDLER_HPP