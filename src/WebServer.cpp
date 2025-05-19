#include "../include/WebServer.hpp"
#include "../include/request/RequestHandler.hpp"
#include "../include/error/ErrorHandler.hpp"
#include "../include/request/HttpException.hpp"
#include "../include/error/NotFound.hpp"
#include "../include/error/BadRequest.hpp"
#include "../include/error/InternalServerError.hpp"
#include "../include/error/MethodNotAllowed.hpp"
#include "../include/error/NotImplemented.hpp"
#include <vector>
#include <algorithm>
#include <fcntl.h>

WebServer::WebServer():
    m_socket(0),
    maxfds(DEFAULT_MAX_CONNECTIONS)
{
    pollfds = new struct pollfd[maxfds];
}

WebServer::~WebServer()
{
    // delete requestHandler;
    if (pollfds)
    {
        delete[] pollfds;
    }
}

void WebServer::setServerConfig(ServerConfig& config){
    this->m_config = config;
}
const ServerConfig& WebServer::getServerConfig(){
    return (this->m_config);
}
// Location* WebServer::getLocationForPath(const std::string& path){
//     // Get all locations from server config
//     std::vector<Location> locations = m_config.get_locations();
//     //TODO:@aghergho find the best match
//     (void)path;
//     return &(locations[0]);
// }
int WebServer::init(ServerConfig& config)
{
    // set configuration
    this->setServerConfig(config);
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
    hint.sin_port = htons(m_config.get_port());
    if (inet_pton(AF_INET, m_config.get_host().c_str(), &hint.sin_addr) <= 0)
    {
        perror("Error: Invalid IP address format");
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
    std::cout << "Server " << m_config.get_server_name() << ": 'http://" << m_config.get_host() << ":" << m_config.get_port() << "'" << std::endl;
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
            {
                std::cout << "START OF SENDING HTTP RESPONSE TO THE CLIENT\n";
                handleClientResponse(fd);
            }
                //client.RespondToClient(fd);    
        }
    }
    std::cout << "Server '" << m_config.get_server_name() << "' is runing on port: " << m_config.get_port() << " ... " << std::endl;
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
    // Create and store ClientConnection
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
    clients[clientFd]._server = this;
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
    std::cout << "============== (START OF HANDLING CLIENT REQUEST) ==============\n";
    ClientConnection &client = clients[fd];
    // Set error chain handler
    ErrorHandler *errorHandler = new NotFound();

    errorHandler->SetNext(new BadRequest())
                ->SetNext(new InternalServerError())
                ->SetNext(new NotImplemented())
                ->SetNext(new MethodNotAllowed());
    try
    {
        client.GenerateRequest(fd);
        client.ProcessRequest(fd);
    }
    catch(HttpException &e)
    {
        std::cerr << "HttpException: " << e.what() << std::endl;
        // Handle the exception using the error handler
        // Create an error object
        std::cout << "Error Type: " << static_cast<int> (e.GetErrorType()) << std::endl;
        std::cout << " Error code :" << e.GetCode() << std::endl;
        Error error(client, e.GetCode(), e.GetMessage(), e.GetErrorType());
        errorHandler->HanldeError(error);
        this->updatePollEvents(fd, POLLOUT);
    }
}

void WebServer::handleClientResponse(int fd)
{
    //check if the client exists in our map
    std::cout << "============== (START OF HANDLING CLIENT RESPONSE) ==============\n";
    if (clients.find(fd) == clients.end())
        return;
    ClientConnection &client = clients[fd];
    if (client.http_response)
        std::cout << "Client ip: " << client.ipAddress << " is sending response============\n";
    else
        std::cout << "Client ip: " << client.ipAddress << "  response (NULLL)============\n";

    if(client.http_request)
        std::cout << "Client ip: " << client.ipAddress << " is sending request============\n";
    else
        std::cout << "Client ip: " << client.ipAddress << "  request (NULLL)============\n";
    if (client.http_response == NULL)
    {
        std::cerr << "Warning: client.http_response is null for fd " << fd << std::endl;
        updatePollEvents(fd, POLLIN);
        // exit(0);
        return;
    }

    // Check if we have data to send
    if (!client.http_response->checkAvailablePacket())
    {
        try
        {            
            if (client.http_response->isChunked() )
            {
                client.http_response->sendChunkedResponse(fd);
                return ;
            }
            else
            {
                if (client.http_response->isFile())
                {
                    client.http_response->sendResponse(fd);
                }
                else
                {
                    client.http_response->sendChunkedResponse(fd);
                }

                this->updatePollEvents(fd, POLLIN);
                // Reset request state for next request
                if (client.http_response->isKeepAlive())
                {
                    std::cout << "after Resesting the request !!!\n";
                    client.http_response->clear();
                    client.http_request->ResetRequest();
                    return ;
                }
                else
                {
                    closeClientConnection(fd);
                    return;
                }
            }
        }
        catch(HttpException & e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
    
    /*
    
        try
        {
            //generate the rquest 
            //handle request..
            //generate response
        }
        catch (error)
        {
            sendResponse as an error 
        }
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
*/

/*
void WebServer::handleClientWrite(int fd)
{
    // First, check if the client exists in our map
    if (clients.find(fd) == clients.end())
    return;
    ClientConnection &client = clients[fd];
    if (!client.sendBuffer.empty())
    {
        ssize_t bytes_sent = send(fd, client.sendBuffer.data(),
        client.sendBuffer.size(), MSG_NOSIGNAL);
        if (bytes_sent <= 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            closeClientConnection(fd);
            return;
        }
        client.sendBuffer.erase(0, bytes_sent);
    }
    
    // Handle next steps based on current state
    if (client.sendBuffer.empty())
    {
        // Debug: Check and log if http_request is null
        if (client.http_request == nullptr)
        {
            std::cerr << "Warning: client.http_request is null for fd " << fd << std::endl;
            updatePollEvents(fd, POLLIN);
            return;
        }
        // If we're serving a file and have more data
        if (client.http_request->request_status && static_cast<size_t>(client.http_request->remaine_bytes) > 0)
        requestHandler->serveFile(fd, client, client.http_request->full_path);
        // If completely done with file transfer
        else if (client.http_request->request_status && 
        client.http_request->remaine_bytes == 0)
        {
            if (!client.http_request->keep_alive)
            closeClientConnection(fd);
            else
            {
                std::cout << "reset previous request !!!!!!!!!\n";
                updatePollEvents(fd, POLLIN);
                bool keepAlive = client.http_request->keep_alive;
                
                // Reset request state for next request, but be careful not to delete the object
                client.http_request->reset();
                if (client.http_request == nullptr)
                {
                    client.http_request = new RequestData();
                    client.http_request->keep_alive = keepAlive;
                }
            }
        }
        else
        updatePollEvents(fd, POLLIN);
    }
}

void WebServer::processHttpRequest(int fd)
{
    ClientConnection &client = clients[fd];
    // Check if we have complete headers
    if (!client.http_request->crlf_flag)
    return; // Incomplete headers, wait for more data
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
*/
