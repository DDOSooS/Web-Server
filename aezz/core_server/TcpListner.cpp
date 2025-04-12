#include "TcpListner.hpp"



TcpListner::TcpListner(const char *ipAddress, int port): m_ipAddress(ipAddress), m_port(port), m_socket(0) {};

int TcpListner::init(){

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket <= 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(m_port);
    inet_pton(AF_INET, m_ipAddress, &hint.sin_addr);
    
    // Bind
    if (bind(m_socket, (struct sockaddr *)&hint, sizeof(hint)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(m_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Create   the master file descriptor set and zero it
    FD_ZERO(&m_master);

    // Add our first socket we re interested in interacting with; the listening socket!
    // It's imporatant that this socket is added for our server or  else we won't 'hear' incoming connections
    FD_SET(m_socket, &m_master);
    printf("init() done ok\n");
    return (0);
};

int TcpListner::run(){
    socklen_t addrlen;
    bool running = true;
    int fdmax = m_socket;
    sockaddr client_saddr;
    sockaddr_in *ipv4_addr;
    sockaddr_in6 *ipv6_addr;
    char str[INET6_ADDRSTRLEN];

    while (running) {
        fd_set readfds = m_master;  
        // monitor readfs for readiness for reading
        if(-1 == select(fdmax + 1, &readfds, nullptr, nullptr, nullptr)) {
            perror("select");
        }

        /* Some sockets are ready. Examine readfds*/
        for (int fd = 0; fd < (fdmax + 1); fd++) {

            if (FD_ISSET(fd, &readfds)) { // fd is ready for readig
                if (fd == m_socket) { // request for new connection
                    addrlen =  sizeof(sockaddr_storage);
                    int clientSocket;
                    if ((clientSocket = accept(m_socket, &client_saddr, &addrlen)) == -1)
                        perror("failed to accept new connection");
                    FD_SET(clientSocket, &m_master);
                    // TODO: Client connected?
                    onClientConnected(clientSocket);
                    if (clientSocket > fdmax)
                        fdmax = clientSocket;

                    // print IP Address of the client
                    if (client_saddr.sa_family == AF_INET) {
                        ipv4_addr = (sockaddr_in *)&client_saddr;
                        inet_ntop(AF_INET, &(ipv4_addr->sin_addr), str, sizeof(str));
                    }
                    else if (client_saddr.sa_family == AF_INET6) {
                        ipv6_addr = (sockaddr_in6 *)&client_saddr;
                        inet_ntop(AF_INET, &(ipv6_addr->sin6_addr), str, sizeof(str));
                    }
                    else {
                        ipv4_addr = nullptr;
                        fprintf(stderr, "Address family is neither AF_INET nor AF_INET6\n");
                    }
                    if (ipv4_addr){
                        //TODO: access log
                        /*syslog()*/   
                    }
                }
                else { // data from existing client .. recieve it?
                    char buf[BUFFER_SIZE]; // TODO: configure how much bytes can I read ...
                    memset(buf, '\0', BUFFER_SIZE); // TODO: in a better elegant way
                    int bytesIn = recv(fd, buf, BUFFER_SIZE, 0);
                    
                    onMessageRecieved(fd, buf,bytesIn);
                    
                    // Recieve the message
                    if (bytesIn <= 0){
                        // Drop the client
                        //TODO: client disconnected ... closesocket(fd)
                        close(fd);
                        FD_CLR(fd, &m_master);
                    }
                    else {
                        // check to see if it's a command. \quit the server
                        if (buf[0] == '\\') {
                            running = false;
                            //TODO: quit server ...
                            close(m_socket);
                        }
                        printf("the client fd[%d] ip[%s] message is : \n%s\n",fd, str, buf);
                    }

                }

            }
        }
    }
    return (0);
};


void TcpListner::sendToCleint(int clientSocket, const char *message, int lenght){
    send(clientSocket, message, lenght, 0);
    //TODO: is it okay for the last flag to be just a zero ???  
}

void TcpListner::onClientConnected(int clientSocket) {

}
// Handler for client disconnection
void TcpListner::onClientDisconnected(int clientSocket) {

}
// Handler when a message is recieved from the client
void TcpListner::onMessageRecieved(int clientSocket, const char *message, int lenght){

}