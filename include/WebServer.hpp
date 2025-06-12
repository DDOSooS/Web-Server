#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/select.h>
#include <poll.h>
#include <map>
#include "./config/ServerConfig.hpp"
#include "./ClientConnection.hpp"



class CgiHandler;

#define BUFFER_SIZE 4096

class   RequestHandler;
class  WebServer
{
    public:
        WebServer();
        ~WebServer();
        int init(ServerConfig& config);
        void acceptNewConnection();
        int run();
        ClientConnection& getClient(int fd) { return clients[fd]; }
        void updatePollEvents(int fd, short events);
        void setServerConfig(ServerConfig& config); // !!! After creation of a server we have to set it's config
        const ServerConfig& getServerConfig();
        // void sendErrorResponse(int fd, int code, const std::string& message);
        //Location getLocationForPath(const std::string& path);
        CgiHandler* getCgiHandler() const { return cgiHandler; }

    protected:
        void closeClientConnection(int clientSocket);
        void handleClientRequest(int fd);
        void handleClientResponse(int fd);
        // void processHttpRequest(int fd);
    private:
        static const int                    DEFAULT_MAX_CONNECTIONS = 1024;
        ServerConfig                        m_config; // this is a variable that will hold the server configuration
        // RequestHandler*                     requestHandler;
        int                                 m_socket;      // Internal socket FD for the listening socket
        struct pollfd                       *pollfds; // files descriptor using poll
        int                                 maxfds, numfds;
        std::map<int, ClientConnection> clients;  // Map of fd to ClientConnection
        CgiHandler                          *cgiHandler; // Pointer to the CGI handler
        // fd_set          m_master;      // Master files descriptor set
        // nfds_t         nfds = 0;
};


#endif
