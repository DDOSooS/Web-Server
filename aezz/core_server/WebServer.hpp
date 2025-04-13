#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <sys/select.h>
#include <poll.h>
#define BUFFER_SIZE 4096


class  WebServer {

public:
    WebServer();
    WebServer(const char *ipAddress, int port);
    ~WebServer();
    int init();
    int run();

protected:
    // Handler for client connection
    virtual void onClientConnected(int clientSocket);
    // Handler for client disconnection
    virtual void onClientDisconnected(int clientSocket);
    // Handler when a message is recieved from the client
    virtual void onMessageReceived(int clientSocket, const char *message, int length);
    // Broadcast a message rom a client ?? TODO: why we need this @aezzahir?
    void sendToClient(int clientSocket, const char *message, int lenght);
    void removeClient(int clientSocket);


private:
    const char      *m_ipAddress; // Server will run on 
    int             m_port;        // Port # for the web service
    int             m_socket;      // Internal socket FD for the listening socket
    fd_set          m_master;      // Master files descriptor set
    struct pollfd *pollfds; // files descriptor using poll
    nfds_t         nfds = 0;
    int maxfds = 0, numfds = 0;
    
    static const int DEFAULT_MAX_CONNECTIONS = 1024;
};
