#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include <string>
#include "../WebServer.hpp"

#include "../ClientData.hpp"

class WebServer;
struct ClientData;

class RequestHandler {
public:
    RequestHandler(WebServer* server);
    void processRequest(int fd);

private:
    WebServer* server;

    // Core request handlers
    void handleGet(int fd, ClientData& client);
    void handlePost(int fd, ClientData& client);
    
    // Special endpoint handlers
    void handleTestGetEndpoint(int fd, ClientData& client);
    void handleTestPostEndpoint(int fd, ClientData& client);
    void handleTestUploadEndpoint(int fd, ClientData& client);
    
    // Utility methods
    void serveFile(int fd, ClientData& client, const std::string& path);
    std::string determineContentType(const std::string& path);
    void processMultipartData(int fd, ClientData& client);
    void processChunkedData(int fd, ClientData& client);
};

#endif 