#include "./include/WebServer.hpp"
#include "./include/config/ConfigParser.hpp"

// Fixed acceptNewConnection function for WebServer class
void WebServer::acceptNewConnection()
{
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(m_socket, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientFd < 0)
    {
        perror("accept");
        return;
    }
    // Set non-blocking
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    // Create the ClientConnection object with proper initialization
    ClientConnection clientConn(clientFd, clientAddr);
    clientConn._server = this;

    // Check if we've reached maximum connections
    if (numfds >= maxfds)
    {
        close(clientFd);
        return;
    }

    // Add to poll set
    pollfds[numfds].fd = clientFd;
    pollfds[numfds].events = POLLIN;
    pollfds[numfds].revents = 0;
    numfds++;

    // Store in clients map
    clients[clientFd] = clientConn;

    std::cout << "Client ip: " << clients[clientFd].ipAddress << " connected" << std::endl;
}
