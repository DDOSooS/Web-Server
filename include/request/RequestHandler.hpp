#pragma once 

#include <string>
#include "./WebServer.hpp"
#include "../ClientConnection.hpp"

class WebServer;
struct ClientConnection;

class RequestHandler
{
    public:
        RequestHandler(WebServer* server);
        void processRequest(int fd);
        void serveFile(int fd, ClientConnection& client, const std::string& path);
    private:
        WebServer*  server;

        // Core request handlers
        void handleGet(int fd, ClientConnection& client);
        void handlePost(int fd, ClientConnection& client);
        void handleDelete(int fd, ClientConnection& client);
        
        // Utility methods
        std::string determineContentType(const std::string& path);
        void processMultipartData(int fd, ClientConnection& client);
        void processChunkedData(int fd, ClientConnection& client);
        void listingDir(int fd, ClientConnection& client, const std::string& path);
};

