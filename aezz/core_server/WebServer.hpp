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
#include <unordered_map>
#define BUFFER_SIZE 4096
#include "ClientData.hpp"
#include "include/RequestHandler.hpp"

class RequestHandler;

class  WebServer {

public:
    WebServer();
    WebServer(const char *ipAddress, int port);
    ~WebServer();
    int init();
    void acceptNewConnection();
    int run();
    ClientData& getClient(int fd) { return clients[fd]; }
    void sendErrorResponse(int fd, int code, const std::string& message);
    void updatePollEvents(int fd, short events);

protected:
    void closeClientConnection(int clientSocket);
    void handleClientRequest(int fd);
    void processHttpRequest(int fd);
    void handleClientWrite(int fd);


private:
    const char      *m_ipAddress; // Server will run on 
    int             m_port;        // Port # for the web service
    int             m_socket;      // Internal socket FD for the listening socket
    // fd_set          m_master;      // Master files descriptor set
    struct pollfd *pollfds; // files descriptor using poll
    // nfds_t         nfds = 0;
    int maxfds = 0, numfds = 0;
    
    static const int DEFAULT_MAX_CONNECTIONS = 1024;


    std::unordered_map<int, ClientData> clients;  // Map of fd to ClientData

    RequestHandler* requestHandler; 
};


#endif