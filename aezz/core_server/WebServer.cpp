#include "WebServer.hpp"
#include <vector>
#include <algorithm>
#include <fcntl.h>

WebServer::WebServer() : m_ipAddress("0.0.0.0"), m_port(8080), m_socket(0), maxfds(DEFAULT_MAX_CONNECTIONS) {
    pollfds = new struct pollfd[maxfds];
};

WebServer::WebServer(const char *ipAddress, int port) 
    : m_ipAddress(ipAddress), m_port(port), m_socket(0), maxfds(DEFAULT_MAX_CONNECTIONS) {
    pollfds = new struct pollfd[maxfds];
};

WebServer::~WebServer() {
    if (pollfds) {
        delete[] pollfds;
    }
}

int WebServer::init() {
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket <= 0) {
        perror("socket failed");
        return -1;
    }

    // Set socket options for robust port reuse
    int optval = 1;
    
    // Allow immediate reuse of the port (SO_REUSEADDR)
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(m_socket);
        return -1;
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
        perror("setsockopt(SO_REUSEPORT) failed");
        // Continue anyway as this is optional
    }

    

    // Bind the socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(m_port);
    
    if (inet_pton(AF_INET, m_ipAddress, &hint.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(m_socket);
        return -1;
    }

    if (bind(m_socket, (struct sockaddr *)&hint, sizeof(hint)) < 0) {
        perror("bind failed");
        close(m_socket);
        return -1;
    }

    // Listen
    if (listen(m_socket, SOMAXCONN) < 0) {
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

int WebServer::run() {
    bool running = true;

    while (running) {
    int ready = poll(pollfds, numfds, -1);
    if (ready == -1) {
        perror("poll");
        continue;
    }

    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].revents == 0) continue;

        int fd = pollfds[i].fd;
        
        // always check for errors first
        if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            closeClientConnection(fd);
            continue;
        }

        // handle incoming data
        if (pollfds[i].revents & POLLIN) {
            if (fd == m_socket) {
                acceptNewConnection();
            } else {
                handleClientRequest(fd);
            }
        }

        // handle outgoing data
        if (pollfds[i].revents & POLLOUT) {
            handleClientWrite(fd);
        }
    }
}
return (0);
}

// Modified acceptNewConnection
void WebServer::acceptNewConnection() {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(m_socket, (struct sockaddr*)&clientAddr, &addrLen);
        
        if (clientFd < 0) {
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
        if (numfds >= maxfds) {
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
void WebServer::closeClientConnection(int clientSocket) {
    auto it = clients.find(clientSocket);
    
    printf("Client ip: %s disconnected\n",clients[clientSocket].ipAddress.c_str());
    
    if (it != clients.end()) {
        close(clientSocket);
        clients.erase(it);
            
// Remove from poll set
    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].fd == clientSocket) {
            pollfds[i] = pollfds[numfds-1];
            numfds--;
            break;
        }
    }
    }

}


void WebServer::updatePollEvents(int fd, short events) {
    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].fd == fd) {
            pollfds[i].events = events;
            break;
        }
    }
}

void WebServer::handleClientRequest(int fd) {
    ClientData& client = clients[fd];
    
    char buf[4096];
    ssize_t bytes_recv = recv(fd, buf, sizeof(buf), 0);
    
    if (bytes_recv <= 0) {
        closeClientConnection(fd);
        return;
    }
    
    client.requestHeaders.append(buf, bytes_recv);
    processHttpRequest(fd);
}


void WebServer::handleClientWrite(int fd) {
    ClientData& client = clients[fd];
    
    if (client.sendBuffer.empty()) {
        // No more data to send, switch back to reading
        updatePollEvents(fd, POLLIN);
        return;
    }

    ssize_t bytes_sent = send(fd, 
                            client.sendBuffer.data(), 
                            client.sendBuffer.size(), 
                            MSG_NOSIGNAL);

    if (bytes_sent <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            closeClientConnection(fd);
        }
        return;
    }

    // Remove sent data from buffer
    client.sendBuffer.erase(0, bytes_sent);

    // If buffer is empty and connection should close
    if (client.sendBuffer.empty() && !client.keepAlive) {
        closeClientConnection(fd);
    }
    // If buffer is empty but keep-alive
    else if (client.sendBuffer.empty()) {
        updatePollEvents(fd, POLLIN); // Back to reading
    }
}














void WebServer::processHttpRequest(int fd) {
    ClientData& client = clients[fd];
    
    // Check if we have complete headers
    size_t header_end = client.requestHeaders.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return; // Incomplete headers, wait for more data
    }

    // Extract request line (first line of headers)
    size_t line_end = client.requestHeaders.find("\r\n");
    if (line_end == std::string::npos) {
        sendErrorResponse(fd, 400, "Bad Request");
        return;
    }

    std::string request_line = client.requestHeaders.substr(0, line_end);
    
    // Parse request method and path
    std::istringstream iss(request_line);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;
    
    // Only handle GET requests
    if (method != "GET") {
        sendErrorResponse(fd, 501, "Not Implemented");
        return;
    }

    // Default to index.html if path is /
    if (path == "/") {
        path = "/index.html";
    }

    // Security: prevent path traversal
    if (path.find("../") != std::string::npos) {
        sendErrorResponse(fd, 403, "Forbidden");
        return;
    }

    // Build filesystem path (assuming web root is in "./www")
    std::string full_path = "./www" + path;
    
    // Try to open the file
    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        sendErrorResponse(fd, 404, "Not Found");
        return;
    }

    // Get file size
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read file content
    std::vector<char> file_data(file_size);
    if (!file.read(file_data.data(), file_size)) {
        sendErrorResponse(fd, 500, "Internal Server Error");
        return;
    }

    // Determine content type
    std::string content_type = "text/plain";
    size_t dot_pos = path.rfind('.');
    if (dot_pos != std::string::npos) {
        std::string ext = path.substr(dot_pos + 1);
        if (ext == "html") content_type = "text/html";
        else if (ext == "css") content_type = "text/css";
        else if (ext == "js") content_type = "application/javascript";
        else if (ext == "png") content_type = "image/png";
        else if (ext == "jpg" || ext == "jpeg") content_type = "image/jpeg";
        else if (ext == "gif") content_type = "image/gif";
    }

    // Build response
    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
    client.responseHeaders += "Content-Type: " + content_type + "\r\n";
    client.responseHeaders += "Content-Length: " + std::to_string(file_size) + "\r\n";
    
    // Check for keep-alive
    bool keepAlive = (client.requestHeaders.find("Connection: keep-alive") != std::string::npos);
    if (keepAlive) {
        client.responseHeaders += "Connection: keep-alive\r\n";
    } else {
        client.responseHeaders += "Connection: close\r\n";
    }
    client.responseHeaders += "\r\n";
    
    // Combine headers and body
    client.sendBuffer = client.responseHeaders + std::string(file_data.data(), file_size);
    
    // Update poll events
    updatePollEvents(fd, POLLOUT);
    client.requestHeaders.clear(); // Prepare for next request if keep-alive
}

void WebServer::sendErrorResponse(int fd, int code, const std::string& message) {
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