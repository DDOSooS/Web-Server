#include "WebServer.hpp"

WebServer::WebServer(const char * ipAddress, int port): TcpListner(ipAddress, port){};


// Handler for client connection
void WebServer::onClientConnected(int clientSocket){

    
}

        // Handler for client disconnection
void WebServer::onClientDisconnected(int clientSocket){

    
}
        
        // Handler when a message is recieved from the client
void WebServer::onMessageRecieved(int clientSocket, const char *message, int lenght){

    
}