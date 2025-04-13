#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>

struct ClientData {
    int fd;                      // Socket file descriptor
    std::string ipAddress;       // Client IP address as string
    uint16_t port;               // Client port number
    time_t connectTime;          // When client connected
    time_t lastActivity;         // Last activity timestamp
    
    // Request data
    std::string requestMethod;   // GET, POST, etc.
    std::string requestPath;     // Requested path
    std::string requestHeaders;  // All headers
    std::string requestBody;     // Request body (for POST)
    size_t contentLength;        // Content-Length header value
    
    // Response data
    std::string responseHeaders; // Prepared response headers
    std::string responseBody;    // Response content
    size_t bytesSent;            // How many bytes already sent
    
    // Buffer management
    std::string sendBuffer;      // Data waiting to be sent
    bool keepAlive;             // Connection: keep-alive
    
    // Constructor
      // Default constructor
    ClientData() : fd(-1), port(0), connectTime(0), lastActivity(0),
                  contentLength(0), bytesSent(0), keepAlive(false) {}
    ClientData(int socketFd, const sockaddr_in& clientAddr) : 
        fd(socketFd),
        port(ntohs(clientAddr.sin_port)),
        connectTime(time(nullptr)),
        lastActivity(time(nullptr)),
        contentLength(0),
        bytesSent(0),
        keepAlive(false)
    {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        ipAddress = ipStr;
    }
    
    // Update activity timestamp
    void updateActivity() {
        lastActivity = time(nullptr);
    }
    
    // Check if connection is stale (timeout)
    bool isStale(time_t timeoutSec) const {
        return (time(nullptr) - lastActivity) > timeoutSec;
    }
};