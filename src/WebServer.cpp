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
#include <map>
#include <sstream>

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
    std::cout << "Server '" << m_config.get_server_name() << "' is running on port: " << m_config.get_port() << std::endl;

    while (running)
    {
        // Wait for events with poll
        int ready = poll(pollfds, numfds, -1);
        if (ready == -1)
        {
            if (errno == EINTR) {
                // Interrupted by signal, just continue
                continue;
            }
            perror("poll");
            break;  // Exit on serious poll errors
        }

        // Process all ready file descriptors
        for (int i = 0; i < numfds; i++)
        {
            // Skip if no events
            if (pollfds[i].revents == 0)
                continue;

            int fd = pollfds[i].fd;

            // Always check for errors first
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                if (fd == m_socket) {
                    // Error on listening socket is fatal
                    std::cerr << "Error on listening socket!" << std::endl;
                    running = false;
                    break;
                } else {
                    // Close client connection on error
                    closeClientConnection(fd);
                }
                continue;
            }

            // Handle incoming data
            if (pollfds[i].revents & POLLIN)
            {
                if (fd == m_socket) {
                    // Accept new connection
                    acceptNewConnection();
                } else {
                    // Handle client request
                    try {
                        handleClientRequest(fd);
                    } catch (const std::exception& e) {
                        std::cerr << "Unhandled exception in handleClientRequest: " << e.what() << std::endl;
                        closeClientConnection(fd);
                    }
                }
            }

            // Handle outgoing data
            if (pollfds[i].revents & POLLOUT)
            {
                std::cout << "START OF SENDING HTTP RESPONSE TO THE CLIENT\n";
                try {
                    handleClientResponse(fd);
                } catch (const std::exception& e) {
                    std::cerr << "Unhandled exception in handleClientResponse: " << e.what() << std::endl;
                    closeClientConnection(fd);
                }
            }
        }

        // Check for and remove stale connections (optional timeout handling)
        // This would iterate through clients and close those inactive for too long
    }

    // Clean up before exiting
    for (int i = 1; i < numfds; i++) {  // Start from 1 to skip the listening socket
        close(pollfds[i].fd);
    }

    std::cout << "Server '" << m_config.get_server_name() << "' has shut down." << std::endl;
    return 0;
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

    // Check if we've reached maximum connections
    if (numfds >= maxfds)
    {
        std::cerr << "Maximum connections reached, rejecting client" << std::endl;
        close(clientFd);
        return;
    }

    try {
        // Create a client connection object first
        ClientConnection conn(clientFd, clientAddr);
        conn._server = this;

        // Add to poll set
        pollfds[numfds].fd = clientFd;
        pollfds[numfds].events = POLLIN;
        pollfds[numfds].revents = 0;
        numfds++;

        // Now store in map after poll is updated
        clients[clientFd] = conn;

        std::cout << "Client ip: " << clients[clientFd].ipAddress << " connected" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating client connection: " << e.what() << std::endl;
        close(clientFd);
        // Don't increment numfds as we failed to create the connection
    }
}
// Close client connection
void WebServer::closeClientConnection(int clientSocket)
{
    std::map<int, ClientConnection>::iterator it = clients.find(clientSocket);
    if (it != clients.end())
    {
        printf("Client ip: %s disconnected\n", it->second.ipAddress.c_str());

        // Clean up allocated resources
        if (it->second.http_request != NULL) {
            delete it->second.http_request;
            it->second.http_request = NULL;
        }

        if (it->second.http_response != NULL) {
            delete it->second.http_response;
            it->second.http_response = NULL;
        }

        // Close socket and remove from clients map
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
    else
    {
        std::cerr << "Warning: Attempted to close non-existent client connection: " << clientSocket << std::endl;
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

    // Check if client exists
    if (clients.find(fd) == clients.end()) {
        std::cerr << "Client not found for fd " << fd << std::endl;
        return;
    }

    ClientConnection &client = clients[fd];

    // Set error chain handler
    ErrorHandler *errorHandler = new NotFound();

    errorHandler->SetNext(new BadRequest())
                ->SetNext(new InternalServerError())
                ->SetNext(new NotImplemented())
                ->SetNext(new MethodNotAllowed());
    try
    {
        // Generate and process the request
        client.GenerateRequest(fd);
        client.ProcessRequest(fd);
    }
    catch(HttpException &e)
    {
        std::cerr << "HttpException: " << e.what() << std::endl;

        // Handle the exception using the error handler
        std::cout << "Error Type: " << (int)(e.GetErrorType()) << std::endl;
        std::cout << " Error code :" << e.GetCode() << std::endl;

        try {
            Error error(client, e.GetCode(), e.GetMessage(), e.GetErrorType());
            errorHandler->HanldeError(error);
            this->updatePollEvents(fd, POLLOUT);
        }
        catch (std::exception &ex) {
            std::cerr << "Error while handling exception: " << ex.what() << std::endl;
            closeClientConnection(fd);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Standard exception in handleClientRequest: " << e.what() << std::endl;
        closeClientConnection(fd);
    }
    catch (...)
    {
        std::cerr << "Unknown exception in handleClientRequest" << std::endl;
        closeClientConnection(fd);
    }

    // Clean up the error handler chain
    delete errorHandler;
}

void WebServer::handleClientResponse(int fd)
{
    //check if the client exists in our map
    std::cout << "============== (START OF HANDLING CLIENT RESPONSE) ==============\n";
    if (clients.find(fd) == clients.end()) {
        std::cerr << "Client with fd " << fd << " not found in clients map" << std::endl;
        return;
    }

    ClientConnection &client = clients[fd];
    if (client.http_response)
        std::cout << "Client ip: " << client.ipAddress << " is sending response============\n";
    else
        std::cout << "Client ip: " << client.ipAddress << "  response (NULLL)============\n";

    if(client.http_request)
        std::cout << "Client ip: " << client.ipAddress << " is sending request============\n";
    else
        std::cout << "Client ip: " << client.ipAddress << "  request (NULLL)============\n";

    // Handle null http_response
    if (client.http_response == NULL)
    {
        std::cerr << "Warning: client.http_response is null for fd " << fd << std::endl;

        try {
            // Create a default response for error handling
            std::map<std::string, std::string> headers;
            headers["Content-Type"] = "text/html";
            headers["Connection"] = "close"; // Force connection close on error

            client.http_response = new HttpResponse(500, headers, "text/html", false, false);
            if (client.http_response == NULL) {
                std::cerr << "Failed to allocate HttpResponse" << std::endl;
                closeClientConnection(fd);
                return;
            }

            client.http_response->setBuffer("<html><body><h1>500 Internal Server Error</h1><p>Invalid response state</p></body></html>");

            // Send this error response
            try {
                client.http_response->sendResponse(fd);
            } catch (const std::exception& e) {
                std::cerr << "Error sending error response: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error sending error response" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception creating error response: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception creating error response" << std::endl;
        }

        // Always close the connection when there's an error
        closeClientConnection(fd);
        return;
    }

    // Check if we have data to send
    if (!client.http_response->checkAvailablePacket())
    {
        try
        {
            if (client.http_response->isChunked())
            {
                client.http_response->sendChunkedResponse(fd);
                return;
            }
            else
            {
                if (client.http_response->isFile())
                {
                    try {
                        client.http_response->sendResponse(fd);
                    } catch (const std::exception& e) {
                        std::cerr << "Error sending file response: " << e.what() << std::endl;
                        closeClientConnection(fd);
                        return;
                    }
                }
                else
                {
                    try {
                        client.http_response->sendChunkedResponse(fd);
                    } catch (const std::exception& e) {
                        std::cerr << "Error sending chunked response: " << e.what() << std::endl;
                        closeClientConnection(fd);
                        return;
                    }
                }

                this->updatePollEvents(fd, POLLIN);

                // Reset request state for next request
                if (client.http_response->isKeepAlive())
                {
                    std::cout << "after Resetting the request !!!\n";

                    // Safely clear and reset
                    if (client.http_response) {
                        client.http_response->clear();
                    }

                    if (client.http_request) {
                        client.http_request->ResetRequest();
                    }
                    return;
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
            std::cerr << "HttpException in handleClientResponse: " << e.what() << '\n';

            // Try to send an error response
            try {
                if (client.http_response) {
                    client.http_response->setStatusCode(e.GetCode());
                    client.http_response->setStatusMessage(e.GetMessage());
                    std::stringstream ss;
                    ss << e.GetCode();
                    client.http_response->setBuffer("<html><body><h1>" + ss.str() + " " + e.GetMessage() + "</h1></body></html>");
                    client.http_response->sendResponse(fd);
                }
            } catch (const std::exception& ex) {
                std::cerr << "Error sending exception response: " << ex.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error sending exception response" << std::endl;
            }

            // Close connection on error
            closeClientConnection(fd);
        }
        catch(std::exception & e)
        {
            std::cerr << "Standard exception in handleClientResponse: " << e.what() << '\n';
            closeClientConnection(fd);
        }
        catch(...)
        {
            std::cerr << "Unknown exception in handleClientResponse" << '\n';
            closeClientConnection(fd);
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
        if (client.http_request == NULL)
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
                if (client.http_request == NULL)
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
