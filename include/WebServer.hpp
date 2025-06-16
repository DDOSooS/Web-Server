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
#include <vector>
#include "./config/ServerConfig.hpp"
#include "./ClientConnection.hpp"

class CgiHandler;

#define BUFFER_SIZE 4096

class RequestHandler;
class WebServer
{
    public:
        WebServer();
        ~WebServer();

        int init(std::vector<ServerConfig>& configs);
        void acceptNewConnection(int listening_socket);
        int run();

        const std::vector<ServerConfig>& getConfigs() const;
        const ServerConfig& getConfigForSocket(int socket) const;
        const ServerConfig& getConfigForClient(int client_fd) const;

        ClientConnection& getClient(int fd) { return clients[fd]; }
        void updatePollEvents(int fd, short events);
        
        // Manage CGI time out ============ 
        void addCgiToPoll(int cgi_fd);
        void removeCgiFromPoll(int cgi_fd);
        bool isCgiFd(int fd);
        void handleCgiEvent(int fd);
        void checkCgiTimeouts();
        
    protected:
        void closeClientConnection(int clientSocket);
        void handleClientRequest(int fd);
        void handleClientResponse(int fd);
        bool isListeningSocket(int fd) const;
        int getServerIndexForSocket(int socket) const;
        
    private:
        static const int                    DEFAULT_MAX_CONNECTIONS = 1024;
        std::vector<ServerConfig>           m_configs;              // Vector of server configurations
        std::vector<int>                    m_sockets;              // Vector of listening sockets
        std::map<int, int>                  socket_to_config_index; // Map listening socket to config index
        std::map<int, int>                  client_to_server_index; // Map client socket to server index
        
        struct pollfd                       *pollfds;               // Files descriptor using poll
        int                                 maxfds, numfds;
        std::map<int, ClientConnection>     clients;                // Map of fd to ClientConnection
        CgiHandler                          *cgiHandler;            // Pointer to the CGI handler
};

#endif