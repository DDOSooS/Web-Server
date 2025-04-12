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
#define BUFFER_SIZE 4096


class  TcpListner {

public:
    TcpListner(const char *ipAddress, int port);
    int init();
    int run();

protected:
    // Handler for client connection
    virtual void onClientConnected(int clientSocket);
    // Handler for client disconnection
    virtual void onClientDisconnected(int clientSocket);
    // Handler when a message is recieved from the client
    virtual void onMessageRecieved(int clientSocket, const char *message, int lenght);
    // Broadcast a message rom a client ?? TODO: why we need this @aezzahir?
    void sendToCleint(int clientSocket, const char *message, int lenght);


private:
    const char      *m_ipAddress; // Server will run on 
    int             m_port;        // Port # for the web service
    int             m_socket;      // Internal socket FD for the listening socket
    fd_set          m_master;      // Master files descriptor set
};
