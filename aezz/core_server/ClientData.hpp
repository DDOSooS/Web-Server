#include <iostream>
#include <string>
#include <cstdint>


class  ClientData {

public:
    ClientData();
    ClientData(const char *ipAddress, int port);
    ~ClientData();




    int fd;                      // Socket file descriptor
    std::string ipAddress;       // Client IP address as string
    uint16_t port;               // Client port number
    time_t connectTime;          // When client connected
    time_t lastActivity;         // Last activity timestamp
    // we can add the bufer etc .. to chunks the posted data etc ...
};
