#include "WebServer.hpp"
#include <vector>
#include <algorithm>

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
    if (m_socket <= 0) {
        fprintf(stderr, "Server not initialized\n");
        return -1;
    }

    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char ip_addr[INET_ADDRSTRLEN];

    while (true) {
        int poll_count = poll(pollfds, numfds, -1);
        if (poll_count == -1) {
            perror("poll");
            continue;
        }

        for (int i = 0; i < numfds; i++) {
            if (pollfds[i].revents == 0)
                continue;

            if (pollfds[i].revents & POLLIN) {
                if (pollfds[i].fd == m_socket) {
                    // Handle new connection
                    int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &addrlen);
                    if (client_fd < 0) {
                        perror("accept");
                        continue;
                    }

                    // Add client to poll set
                    if (numfds >= maxfds) {
                        fprintf(stderr, "Maximum connections reached (%d)\n", maxfds);
                        close(client_fd);
                        continue;
                    }

                    pollfds[numfds].fd = client_fd;
                    pollfds[numfds].events = POLLIN;
                    numfds++;

                    // Get client IP
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip_addr, INET_ADDRSTRLEN);
                    printf("New connection from %s:%d\n", ip_addr, ntohs(client_addr.sin_port));
                } else {
                    // Handle data from client
                    char buf[BUFFER_SIZE];
                    ssize_t bytes_recv = recv(pollfds[i].fd, buf, BUFFER_SIZE, 0);

                    if (bytes_recv <= 0) {
                        // Client disconnected
                        if (bytes_recv == 0) {
                            printf("Client %d disconnected\n", pollfds[i].fd);
                        } else {
                            perror("recv");
                        }
                        onClientDisconnected(pollfds[i].fd);
                        close(pollfds[i].fd);
                        // Remove from poll set by swapping with last element
                        removeClient(pollfds[i].fd);
                        continue;  // Skip the rest of the loop
                    } else {
                        onMessageReceived(pollfds[i].fd, buf, bytes_recv);
                        close(pollfds[i].fd);
                        removeClient(pollfds[i].fd);
                    }
                }
            } else if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Handle error conditions
                onClientDisconnected(pollfds[i].fd);
                close(pollfds[i].fd);

                // Remove from poll set
                pollfds[i] = pollfds[numfds-1];
                numfds--;
                i--;
            }
        }
    }

    return 0;
}

void WebServer::sendToClient(int clientSocket, const char* message, int length) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << length << "\r\n"
        << "Connection: close\r\n"  // Explicitly close connection
        << "\r\n"  // End of headers
        << std::string(message, length);
    
    std::string response = oss.str();
    int total_sent = 0;
    while (total_sent < response.size()) {
        int bytes_sent = send(clientSocket, response.c_str() + total_sent, 
                             response.size() - total_sent, 0);
        if (bytes_sent <= 0) {
            perror("send failed");
            onClientDisconnected(clientSocket);
            close(clientSocket);
            removeClient(clientSocket);
            break;
        }
        total_sent += bytes_sent;
    }
    
    // Close the socket after sending response TODO: keep alive
    shutdown(clientSocket, SHUT_WR);
}

void WebServer::removeClient(int clientSocket) {
    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].fd == clientSocket) {
            pollfds[i] = pollfds[numfds-1]; // Swap it with the last element;
            numfds--;
            break;
        }
    }
}


void WebServer::onClientConnected(int clientSocket) {
    std::cout << "Client connected Number : " << clientSocket << std::endl;
    // TODO: 
}

void WebServer::onClientDisconnected(int clientSocket) {
    std::cout << "Client disconnected Number: " << clientSocket << std::endl;
    // TODO:
}

void WebServer::onMessageReceived(int clientSocket, const char *message, int length) {
    std::string request(message, length);
    
    // Simple check if this is a HTTP GET request
    if (request.find("GET ") == 0) {
        std::string response = 
            "<html><body><h1>Hello World</h1></body></html>";
        
        sendToClient(clientSocket, response.c_str(), response.size());
    }
    else if (request.find("POST ") == 0) {
        std::string response = 
            "<html><body><h1>This is a post request</h1></body></html>";
        
        sendToClient(clientSocket, response.c_str(), response.size());
    }
    else {
        // Handle other cases or invalid requests
        std::string response = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        
        send(clientSocket, response.c_str(), response.size(), 0);
    }
    
    // Close the connection after response
    close(clientSocket);
    removeClient(clientSocket);
}
