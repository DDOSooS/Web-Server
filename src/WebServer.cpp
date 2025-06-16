#include "../include/WebServer.hpp"
#include "../include/request/RequestHandler.hpp"
#include "../include/error/ErrorHandler.hpp"
#include "../include/request/HttpException.hpp"
#include "../include/error/NotFound.hpp"
#include "../include/error/BadRequest.hpp"
#include "../include/error/InternalServerError.hpp"
#include "../include/error/MethodNotAllowed.hpp"
#include "../include/error/NotImplemented.hpp"
#include "../include/error/Forbidden.hpp"
#include "../include/error/TooManyRedirection.hpp"
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include <sstream>
#include <signal.h>
#include <sys/wait.h>
#include "../include/request/CgiHandler.hpp" 
#include "../include/request/RequestHandler.hpp" 


WebServer::WebServer():m_socket(0), maxfds(DEFAULT_MAX_CONNECTIONS)
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
    time_t last_timeout_check = time(NULL);  


    std::cout << "Server '" << m_config.get_server_name() << "' is running on port: " << m_config.get_port() << std::endl;
    // Set error chain handler
    ErrorHandler *errorHandler = new NotFound();
    errorHandler->SetNext(new BadRequest())
                ->SetNext(new InternalServerError())
                ->SetNext(new NotImplemented())
                ->SetNext(new MethodNotAllowed())
                ->SetNext(new Forbidden())
                ->SetNext(new TooManyRedirection());
    while (running)
    {
        // ============ CHECK FOR TIME OUT ===============
        time_t current_time = time(NULL);
        if (current_time - last_timeout_check >= 2)
        {
            checkCgiTimeouts();
            last_timeout_check = current_time;
        }
        int ready = poll(pollfds, numfds, 1000);
        
        if (ready == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("poll");
            break;
        }

        // =================== Check for new CGI processes ===================================
        for (std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.begin();
             it != CgiHandler::active_cgis.end(); ++it) {
            int cgi_fd = it->first;
            bool found = false;
            for (int i = 0; i < numfds; i++) {
                if (pollfds[i].fd == cgi_fd) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                addCgiToPoll(cgi_fd);
            }
        }

        for (int i = 0; i < numfds; i++)
        {
            // ==> checking time out for client connections <==
            if (clients.find(pollfds[i].fd) != clients.end())
            {
                ClientConnection& conn = clients[pollfds[i].fd];
                if (conn.isStale(30))
                { 
                    std::cout << "Client connection timed out, closing connection." << std::endl;
                    closeClientConnection(pollfds[i].fd);
                    continue;
                }
            }
            if (pollfds[i].revents == 0)
                continue;
            int fd = pollfds[i].fd;

            // ========================================= handle CGI events first:
            if (isCgiFd(fd)) {
                if (pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                    handleCgiEvent(fd);
                }
                continue;
            }
            
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                if (fd == m_socket) {
                    std::cerr << "Error on listening socket!" << std::endl;
                    running = false;
                    break;
                } else {
                    closeClientConnection(fd);
                }
                continue;
            }

            if (pollfds[i].revents & POLLIN)
            {
                if (fd == m_socket) {
                    acceptNewConnection();
                } else
                {
                    try
                    {
                        handleClientRequest(fd);
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Unhandled exception in handleClientRequest: " << e.what() << std::endl;
                        closeClientConnection(fd);
                    }
                }
            }

            // Handle outgoing data
            if (pollfds[i].revents & POLLOUT)
            {
                //std::cout << "START OF SENDING HTTP RESPONSE TO THE CLIENT\n";
                try {
                    handleClientResponse(fd);
                }
                catch (const HttpException & e)
                {
                    std::cerr << "Unhandled exception in handleClientResponse: " << e.what() << std::endl;
                    this->updatePollEvents(fd, POLLOUT);
                    Error error(clients[fd], e.GetCode(), e.GetMessage(), e.GetErrorType());
                    errorHandler->HanldeError(error, this->getServerConfig());
                    std::cout  << "[DEBUG] : CLOSING CLIENT CONNECITON HAPPENED HERE\n";
                    closeClientConnection(fd);
                }
            }
        }
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
        clients[clientFd].updateActivity(); // Initialize activity timestamp

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
        if (it->second.http_request != NULL)
        {
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
    clients[fd].updateActivity(); // Update last activity timestamp


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
                ->SetNext(new MethodNotAllowed())
                ->SetNext(new Forbidden())
                ->SetNext(new TooManyRedirection());
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
        // std::cout << "Error Type: " << (int)(e.GetErrorType()) << std::endl;
        // std::cout << " Error code :" << e.GetCode() << std::endl;

        try
        {
            Error error(client, e.GetCode(), e.GetMessage(), e.GetErrorType());
            errorHandler->HanldeError(error, this->getServerConfig());
            this->updatePollEvents(fd, POLLOUT);
        }
        catch (std::exception &ex)
        {
            std::cerr << "Error while handling exception: " << ex.what() << std::endl;
            closeClientConnection(fd);
        }
    }
    catch (...)
    {
        std::cerr << "Unknown exception in handleClientRequest" << std::endl;
        closeClientConnection(fd);
    }
    delete errorHandler;
}

// @TO TAKE CARE OF: before any change in the response, check with @abdessalam before making changes
void WebServer::handleClientResponse(int fd)
{
    //check if the client exists in our map
    //std::cout << "============== (START OF HANDLING CLIENT RESPONSE) ==============\n";
    if (clients.find(fd) == clients.end()) {
        std::cerr << "Client with fd " << fd << " not found in clients map" << std::endl;
        return;
    }
    ClientConnection &client = clients[fd];
/*
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
        
        try
        {
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
    */
    // Check if we have data to send
    if (client.http_response == NULL)
    {
        std::cerr << "Warning: client.http_response is null for fd " << fd << std::endl;
        updatePollEvents(fd, POLLIN);
        // exit(0);
        return;
    }
    // Check if we have data to send
    if (client.http_response->checkAvailablePacket())
    {
        std::cout << "\n\ndata to send for client fd: " << fd << std::endl;        
         if (client.http_response->isChunked())
            {
                std::cout << "[Debug] sending chunked response\n";
                try {
                    client.http_response->sendChunkedResponse(fd);
                    
                    std::cout << "----------- [Chunked response Detail] -------------\n";
                    std::cout << "File bytes sent: " << client.http_response->getByteSent() << "\n";
                    std::cout << "File bytes to send: " << client.http_response->getByteToSend() << "\n";
                    if (client.http_response->getByteSent() >= client.http_response->getByteToSend())
                    {
                        client.http_response->sendChunkedResponse(fd);
                        std::cout << "Chunked response sent completely\n";
                        
                        if (client.http_response->isKeepAlive())
                        {
                            std::cout << "Resetting request for keep-alive\n";
                            client.http_response->clear();
                            client.http_request->ResetRequest();
                            this->updatePollEvents(fd, POLLIN);
                        }
                        else
                        {
                            std::cout << "Closing connection after chunked response\n";
                            closeClientConnection(fd);
                        }
                    }
                }
                catch (const HttpException& e)
                {
                    std::cerr << "Error in chunked response: " << e.what() << std::endl;
                    closeClientConnection(fd);
                }
            }
        else
        {
            if (client.http_response->isFile())
            {
                std::cout << "Debug sendfile response11\n";
                // exit(0);
                client.http_response->sendResponse(fd);                
            }
            else
            {
                // std::cout << " -------------------- [Debug] : number of redirections: " << client.redirect_counter << "Is redirection " << client.http_request->IsRedirected() << std::endl;
                if (client.http_request && client.http_request->IsRedirected())
                {
                    client.redirect_counter++;
                    std::cout << " -------------------- [Debug] : number of redirections: " << client.redirect_counter << "Is redirection " << client.http_request->IsRedirected() << std::endl;
                    if (client.redirect_counter > 10)
                    {
                        client.redirect_counter = 0;
                        Error error(client, 429, "Too Many Redirections", TOO_MANY_REDIRECTION);
                        ErrorHandler *errorHandler = new TooManyRedirection();
                        errorHandler->HanldeError(error, this->getServerConfig());
                        delete errorHandler;
                        client.should_close = true; 
                    }
                }
                else
                {
                    std::cout << "Resetting redirect counter to 0\n\n\n";
                    client.redirect_counter = 0; 
                }
                std::cout << "Debug sendfile response22\n";
                client.http_response->sendChunkedResponse(fd);
                this->updatePollEvents(fd, POLLIN);

                if (client.should_close) {
                    std::cout << "----Closing connection after error response\n";
                    closeClientConnection(fd);
                    return;
                }
                client.http_response->sendChunkedResponse(fd);
            }
            this->updatePollEvents(fd, POLLIN);
            if (client.http_response->isKeepAlive())
            {
                std::cout << "after Resesting the request !!!\n";
                client.http_response->clear();
                client.http_request->ResetRequest();
                return ;
            }
            else
            {
                std::cout << "----Closing connection after response\n";
                closeClientConnection(fd);
                return;
            }
        }
    }
}




// ================= CGI TIME OUT MANAGEMENT
void WebServer::addCgiToPoll(int cgi_fd) {
    if (numfds < maxfds) {
        pollfds[numfds].fd = cgi_fd;
        pollfds[numfds].events = POLLIN;
        pollfds[numfds].revents = 0;
        numfds++;
        std::cout << "Added CGI fd " << cgi_fd << " to poll" << std::endl;
    }
}

void WebServer::removeCgiFromPoll(int cgi_fd) {
    for (int i = 1; i < numfds; i++) {
        if (pollfds[i].fd == cgi_fd) {
            pollfds[i] = pollfds[numfds - 1];
            numfds--;
            std::cout << "Removed CGI fd " << cgi_fd << " from poll" << std::endl;
            break;
        }
    }
}

bool WebServer::isCgiFd(int fd) {
    return CgiHandler::active_cgis.find(fd) != CgiHandler::active_cgis.end();
}




// ======================== CGI Time Out ========================
void WebServer::checkCgiTimeouts() {
    time_t current_time = time(NULL);
    std::vector<int> timed_out_fds;
    
    for (std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.begin();
         it != CgiHandler::active_cgis.end(); ++it) {
        
        if (current_time - it->second.start_time > 10) {  // 10 second timeout
            std::cout << "CGI process " << it->second.pid << " timed out after 30 seconds" << std::endl;
            
            // Kill the process
            kill(it->second.pid, SIGTERM);
            usleep(100000); // 100ms
            
            int status;
            if (waitpid(it->second.pid, &status, WNOHANG) == 0) {
                kill(it->second.pid, SIGKILL);
                waitpid(it->second.pid, &status, 0);
            }
            
            // Set timeout error response
            std::map<std::string, std::string> headers;
            headers["Content-Type"] = "text/html";
            it->second.client->http_response = new HttpResponse(504, headers, "text/html", false, false);
            it->second.client->http_response->setBuffer("<html><body><h1>504 Gateway Timeout</h1><p>CGI script exceeded 10 second limit</p></body></html>");
            
            // Signal client to send response
            updatePollEvents(it->second.client->GetFd(), POLLOUT);
            
            timed_out_fds.push_back(it->first);
        }
    }
    
    // Clean up timed out processes
    for (std::vector<int>::iterator it = timed_out_fds.begin(); it != timed_out_fds.end(); ++it) {
        close(*it);
        removeCgiFromPoll(*it);
        CgiHandler::active_cgis.erase(*it);
    }
}


// ================== CGI Events ==================================

void WebServer::handleCgiEvent(int fd) {
    std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.find(fd);
    if (it == CgiHandler::active_cgis.end()) {
        return;
    }
    
    CgiHandler::CgiProcess& cgi = it->second;
    
    // Check timeout (10 seconds)
    if (time(NULL) - cgi.start_time > 10) {
        std::cout << "CGI timeout, killing process " << cgi.pid << std::endl;
        kill(cgi.pid, SIGKILL);
        waitpid(cgi.pid, NULL, 0);
        
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "text/html";
        cgi.client->http_response = new HttpResponse(504, headers, "text/html", false, false);
        cgi.client->http_response->setBuffer("<html><body><h1>504 Gateway Timeout</h1></body></html>");
        
        updatePollEvents(cgi.client->GetFd(), POLLOUT);
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
        return;
    }
    
    // Read CGI output
    char buffer[4096];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        cgi.output += buffer;
        std::cout << "🔍 Read " << bytes << " bytes from CGI process " << cgi.pid << std::endl;
    } else if (bytes == 0) {
        // EOF - CGI finished
        std::cout << "🔍 CGI process " << cgi.pid << " finished (EOF)" << std::endl;
        std::cout << "🔍 Total output length: " << cgi.output.length() << " bytes" << std::endl;
        
        // Print first 200 characters of output for debugging
        if (cgi.output.length() > 0) {
            std::string preview = cgi.output.substr(0, 200);
            std::cout << "🔍 Output preview: " << preview << "..." << std::endl;
        } else {
            std::cout << "🔍 No output received from CGI!" << std::endl;
        }
        
        int status;
        pid_t wait_result = waitpid(cgi.pid, &status, 0);
        std::cout << "🔍 waitpid result: " << wait_result << " for PID " << cgi.pid << std::endl;
        
        if (wait_result == cgi.pid) {
            std::cout << "🔍 Process status raw value: " << status << std::endl;
            
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                std::cout << "🔍 Process exited normally with code: " << exit_code << std::endl;
                
                if (exit_code == 0) {
                    std::cout << "🔍 SUCCESS: Processing CGI output" << std::endl;
                    
                    // Parse headers and body
                    std::string headers, body;
                    size_t header_end = cgi.output.find("\r\n\r\n");
                    if (header_end == std::string::npos) {
                        header_end = cgi.output.find("\n\n");
                        if (header_end != std::string::npos) {
                            headers = cgi.output.substr(0, header_end);
                            body = cgi.output.substr(header_end + 2);
                        } else {
                            headers = "";
                            body = cgi.output;
                        }
                    } else {
                        headers = cgi.output.substr(0, header_end);
                        body = cgi.output.substr(header_end + 4);
                    }
                    
                    std::cout << "🔍 Headers found: " << headers.length() << " chars" << std::endl;
                    std::cout << "🔍 Body found: " << body.length() << " chars" << std::endl;
                    
                    // Parse CGI headers
                    std::map<std::string, std::string> response_headers;
                    response_headers["Content-Type"] = "text/html";
                    
                    int status_code = 200;
                    std::string status_message = "OK";
                    std::vector<std::string> set_cookies;
                    
                    // Parse each header line
                    std::istringstream header_stream(headers);
                    std::string line;
                    
                    while (std::getline(header_stream, line)) {
                        // Remove carriage return if present
                        if (!line.empty() && line[line.length() - 1] == '\r') {
                            line.erase(line.length() - 1);
                        }
                        
                        // Skip empty lines
                        if (line.empty()) continue;
                        
                        // Find colon separator
                        size_t colon_pos = line.find(':');
                        if (colon_pos == std::string::npos) continue;
                        
                        std::string header_name = line.substr(0, colon_pos);
                        std::string header_value = line.substr(colon_pos + 1);
                        
                        // Trim whitespace from header value
                        while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t')) {
                            header_value.erase(0, 1);
                        }
                        while (!header_value.empty() && (header_value.back() == '\r' || header_value.back() == ' ' || header_value.back() == '\t')) {
                            header_value.pop_back();
                        }
                        
                        std::cout << "🔍 Processing CGI header: " << header_name << ": " << header_value << std::endl;
                        
                        // Handle special CGI headers
                        if (header_name == "Status") {
                            size_t space_pos = header_value.find(' ');
                            if (space_pos != std::string::npos) {
                                status_code = std::atoi(header_value.substr(0, space_pos).c_str());
                                status_message = header_value.substr(space_pos + 1);
                            } else {
                                status_code = std::atoi(header_value.c_str());
                            }
                            std::cout << "🔍 Set status: " << status_code << " " << status_message << std::endl;
                        } 
                        else if (header_name == "Content-Type" || header_name == "Content-type") {
                            response_headers["Content-Type"] = header_value;
                            std::cout << "🔍 Set content-type: " << header_value << std::endl;
                        } 
                        else if (header_name == "Location") {
                            response_headers["Location"] = header_value;
                            std::cout << "🔍 Set redirect location: " << header_value << std::endl;
                        }
                        else if (header_name == "Set-Cookie") {
                            set_cookies.push_back(header_value);
                            std::cout << "🔍 [CRITICAL] Found Set-Cookie: " << header_value << std::endl;
                        }
                        else {
                            response_headers[header_name] = header_value;
                            std::cout << "🔍 Set header: " << header_name << ": " << header_value << std::endl;
                        }
                    }
                    
                    // Create HTTP response
                    cgi.client->http_response = new HttpResponse(status_code, response_headers, response_headers["Content-Type"], false, false);
                    
                    // Set cookies if any
                    for (std::vector<std::string>::iterator cookie_it = set_cookies.begin(); 
                         cookie_it != set_cookies.end(); ++cookie_it) {
                        cgi.client->http_response->setHeader("Set-Cookie", *cookie_it);
                        std::cout << "🔍 [CRITICAL] Added Set-Cookie to response: " << *cookie_it << std::endl;
                    }
                    
                    cgi.client->http_response->setBuffer(body);
                    
                    std::cout << "🔍 Final CGI response - Status: " << status_code << " " << status_message << std::endl;
                    updatePollEvents(cgi.client->GetFd(), POLLOUT);
                } else {
                    std::cout << "🔍 ERROR: CGI process exited with non-zero code: " << exit_code << std::endl;
                    // Show some output for debugging
                    if (cgi.output.length() > 0) {
                        std::cout << "🔍 CGI output was: " << cgi.output.substr(0, 500) << std::endl;
                    }
                    
                    std::map<std::string, std::string> error_headers;
                    error_headers["Content-Type"] = "text/html";
                    cgi.client->http_response = new HttpResponse(500, error_headers, "text/html", false, false);
                    cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Exit code: " + std::string(1, '0' + exit_code) + "</p></body></html>");
                    updatePollEvents(cgi.client->GetFd(), POLLOUT);
                }
            } else if (WIFSIGNALED(status)) {
                int signal_num = WTERMSIG(status);
                std::cout << "🔍 ERROR: CGI process terminated by signal: " << signal_num << std::endl;
                
                std::map<std::string, std::string> error_headers;
                error_headers["Content-Type"] = "text/html";
                cgi.client->http_response = new HttpResponse(500, error_headers, "text/html", false, false);
                cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Killed by signal: " + std::string(1, '0' + signal_num) + "</p></body></html>");
                updatePollEvents(cgi.client->GetFd(), POLLOUT);
            } else {
                std::cout << "🔍 ERROR: CGI process ended abnormally" << std::endl;
                
                std::map<std::string, std::string> error_headers;
                error_headers["Content-Type"] = "text/html";
                cgi.client->http_response = new HttpResponse(500, error_headers, "text/html", false, false);
                cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Process ended abnormally</p></body></html>");
                updatePollEvents(cgi.client->GetFd(), POLLOUT);
            }
        } else {
            std::cout << "🔍 ERROR: waitpid failed or returned unexpected result" << std::endl;
            
            std::map<std::string, std::string> error_headers;
            error_headers["Content-Type"] = "text/html";
            cgi.client->http_response = new HttpResponse(500, error_headers, "text/html", false, false);
            cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>waitpid failed</p></body></html>");
            updatePollEvents(cgi.client->GetFd(), POLLOUT);
        }
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
    } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cout << "🔍 ERROR: read() failed: " << strerror(errno) << std::endl;
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
    }
}