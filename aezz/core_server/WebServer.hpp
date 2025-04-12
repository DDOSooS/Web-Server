#include "TcpListner.hpp"


class WebServer : public TcpListner {


    public:
        WebServer(const char * ipAddress, int port);
    

    protected:
        // Handler for client connection
        virtual void onClientConnected(int clientSocket);

        // Handler for client disconnection
        virtual void onClientDisconnected(int clientSocket);
        
        // Handler when a message is recieved from the client
        virtual void onMessageRecieved(int clientSocket, const char *message, int lenght);
};