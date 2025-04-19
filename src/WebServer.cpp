#include "../include/WebServer.hpp"
#include "../include/RequestHandler.hpp"
#include <vector>
#include <algorithm>
#include <fcntl.h>

WebServer::WebServer()
    : m_ipAddress("0.0.0.0"),
      m_port(8080),
      m_socket(0),
      maxfds(DEFAULT_MAX_CONNECTIONS),
      requestHandler(new RequestHandler(this))
{
    pollfds = new struct pollfd[maxfds];
}

WebServer::WebServer(const char *ipAddress, int port)
    : m_ipAddress(ipAddress),
      m_port(port),
      m_socket(0),
      maxfds(DEFAULT_MAX_CONNECTIONS),
      requestHandler(new RequestHandler(this))
{
    pollfds = new struct pollfd[maxfds];
}
WebServer::~WebServer()
{
    delete requestHandler;
    if (pollfds)
    {
        delete[] pollfds;
    }
}
int WebServer::init()
{
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket <= 0)
    {
        perror("socket failed");
        return -1;
    }
    // Set socket options for robust port reuse
    int optval = 1;
    // Allow immediate reuse of the port (SO_REUSEADDR)
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(m_socket);
        return -1;
    }
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)))
    {
        perror("setsockopt(SO_REUSEPORT) failed");
        // Continue anyway as this is optional
    }
    // Bind the socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ipAddress, &hint.sin_addr) <= 0)
    {
        perror("inet_pton failed");
        close(m_socket);
        return -1;
    }
    if (bind(m_socket, (struct sockaddr *)&hint, sizeof(hint)) < 0)
    {
        perror("bind failed");
        close(m_socket);
        return -1;
    }
    // Listen
    if (listen(m_socket, SOMAXCONN) < 0)
    {
        perror("listen failed");
        close(m_socket);
        return -1;
    }
    // Initialize pollfd structure
    memset(pollfds, 0, sizeof(struct pollfd) * maxfds);
    pollfds[0].fd = m_socket;
    pollfds[0].events = POLLIN;
    numfds = 1;
    printf("Server initialized on %s:%d\n", m_ipAddress, m_port);
    return 0;
}

int WebServer::run()
{
    bool running = true;
    while (running)
    {
        int ready = poll(pollfds, numfds, -1);
        if (ready == -1)
        {
            perror("poll");
            continue;
        }
        for (int i = 0; i < numfds; i++)
        {
            if (pollfds[i].revents == 0)
                continue;
            int fd = pollfds[i].fd;
            // always check for errors first
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                closeClientConnection(fd);
                continue;
            }
            // handle incoming data
            if (pollfds[i].revents & POLLIN)
            {
                if (fd == m_socket)
                    acceptNewConnection();
                else
                    handleClientRequest(fd);
            }
            // handle outgoing data
            if (pollfds[i].revents & POLLOUT)
                handleClientWrite(fd);
        }
    }
    return (0);
}

// Modified acceptNewConnection
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
    // Create and store ClientData
    clients.emplace(std::piecewise_construct,
                    std::forward_as_tuple(clientFd),
                    std::forward_as_tuple(clientFd, clientAddr));
    // Add to poll set
    if (numfds >= maxfds)
    {
        close(clientFd);
        clients.erase(clientFd);
        return;
    }
    pollfds[numfds].fd = clientFd;
    pollfds[numfds].events = POLLIN;
    numfds++;
    printf("Client ip: %s connected\n", clients[clientFd].ipAddress.c_str());
}
// Close client connection
void WebServer::closeClientConnection(int clientSocket)
{
    auto it = clients.find(clientSocket);
    printf("Client ip: %s disconnected\n", clients[clientSocket].ipAddress.c_str());
    if (it != clients.end())
    {
        close(clientSocket);
        clients.erase(it);
        // Remove from poll set
        for (int i = 0; i < numfds; i++)
        {
            if (pollfds[i].fd == clientSocket)
            {
                pollfds[i] = pollfds[numfds - 1];
                numfds--;
                break;
            }
        }
    }
}
void WebServer::updatePollEvents(int fd, short events)
{
    for (int i = 0; i < numfds; i++)
    {
        if (pollfds[i].fd == fd)
        {
            pollfds[i].events = events;
            break;
        }
    }
}
void WebServer::handleClientRequest(int fd)
{
    ClientData &client = clients[fd];
    char buf[4096];
    ssize_t bytes_recv = recv(fd, buf, sizeof(buf), 0);
    if (bytes_recv <= 0)
    {
        closeClientConnection(fd);
        return;
    }
    client.parseRequest(buf);
    // client.requestHeaders.append(buf, bytes_recv);
    // If we haven't parsed headers yet, look for end of headers
    if (!client.http_request->crlf_flag)
        return ;
    // Parse headers if not already done
    if (client.http_request->method.empty())
    {
        if (!client.http_request->request_line)
        {
            sendErrorResponse(fd, 400, "Bad Request");
            return;
        }
    }
    // For POST, accumulate body
    if (client.http_request->method == "POST")
    {
        if (client.http_request->crlf_flag)
        {
            std::string tmp;
            size_t body_start;
            tmp = buf;
            body_start = tmp.find("\r\n\r\n");
            body_start += 4;
            std::string body = tmp.substr(body_start);
            client.http_request->request_body += body;
            // Remove body from headers string to avoid duplication
            // client.requestHeaders.erase(body_start);
        }
        // If Content-Length, wait for full body
        if (client.http_request->content_length > 0 && client.http_request->request_body.size() < client.http_request->content_length)
        {
            return; // Wait for more data
        }
        // If chunked, wait for end of chunks (handled in processHttpRequest)
    }
    std::cout << "Process Request\n";
    processHttpRequest(fd);
}

void WebServer::handleClientWrite(int fd)
{
    ClientData &client = clients[fd];
    if (client.sendBuffer.empty())
    {
        // No more data to send, switch back to reading
        updatePollEvents(fd, POLLIN);
        return;
    }
    ssize_t bytes_sent = send(fd,
                              client.sendBuffer.data(),
                              client.sendBuffer.size(),
                              MSG_NOSIGNAL);
    if (bytes_sent <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            closeClientConnection(fd);
        }
        return;
    }
    // Remove sent data from buffer
    client.sendBuffer.erase(0, bytes_sent);
    // If buffer is empty and connection should close
    if (client.sendBuffer.empty() && !client.http_request->keep_alive)
    {
        closeClientConnection(fd);
    }
    // If buffer is empty but keep-alive
    else if (client.sendBuffer.empty())
    {
        updatePollEvents(fd, POLLIN); // Back to reading
    }
}
void WebServer::processHttpRequest(int fd)
{
    ClientData &client = clients[fd];
    // Check if we have complete headers
    if (!client.http_request->crlf_flag)
    {
        return; // Incomplete headers, wait for more data
    }
    // Extract request line (first line of headers)
    if (!client.http_request->request_line)
    {
        sendErrorResponse(fd, 400, "Bad Request");
        return;
    }
    requestHandler->processRequest(fd);
}


void WebServer::sendErrorResponse(int fd, int code, const std::string &message)
{
    std::string body = "<html><body><h1>" + std::to_string(code) + " " + message + "</h1></body></html>";
    std::string response = "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n"; // Always close on errors
    response += "\r\n";
    response += body;
    clients[fd].sendBuffer = response;
    updatePollEvents(fd, POLLOUT);
}
