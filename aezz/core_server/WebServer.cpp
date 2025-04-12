#include "WebServer.hpp"

WebServer::WebServer(const char* ipAddress, int port)
    : TcpListner(ipAddress, port) {
    std::cout << "WebServer created at " << ipAddress << ":" << port << std::endl;
}

void WebServer::onClientConnected(int clientSocket) {
    std::cout << "Client connected: " << clientSocket << std::endl;
    // TODO: 
}

void WebServer::onClientDisconnected(int clientSocket) {
    std::cout << "Client disconnected: " << clientSocket << std::endl;
    // TODO:
}

void WebServer::onMessageReceived(int clientSocket, const char* message, int length) {
    std::cout << " " << clientSocket << ": " << message << std::endl;
    // GET /index.html HTTP/1.1

    // TODO:Parse the request ...
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-lenght: 31\r\n";
    oss << "\r\n";
    oss << "<h1>Hello From Web Server</h1>";

    std::string response = oss.str();
    int size = response.size() + 1;

    sendToClient(clientSocket, response.c_str(), size);
}
