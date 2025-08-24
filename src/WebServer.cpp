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

WebServer::WebServer() : maxfds(DEFAULT_MAX_CONNECTIONS)
{
    pollfds_vec.resize(maxfds);
    pollfds = &pollfds_vec[0];
     numfds = 0;
     _active_events = 0;
}

WebServer::~WebServer()
{
    for (size_t i = 0; i < m_sockets.size(); ++i)
    {
        if (m_sockets[i] > 0)
            close(m_sockets[i]);
    }
}

const std::vector<ServerConfig>& WebServer::getConfigs() const
{
    return m_configs;
}

const ServerConfig& WebServer::getConfigForSocket(int socket) const {
    std::map<int, int>::const_iterator it = socket_to_config_index.find(socket);
    if (it != socket_to_config_index.end())
    {
        return m_configs[it->second];
    }
    throw std::runtime_error("Socket not found in configuration mapping");
}

const ServerConfig& WebServer::getConfigForClient(int client_fd) const {
    std::map<int, int>::const_iterator it = client_to_server_index.find(client_fd);
    if (it != client_to_server_index.end())
    {
        return m_configs[it->second];
    }
    throw std::runtime_error("Client not found in server mapping");
}

bool WebServer::isListeningSocket(int fd) const {
    return socket_to_config_index.find(fd) != socket_to_config_index.end();
}

int WebServer::getServerIndexForSocket(int socket) const {
    std::map<int, int>::const_iterator it = socket_to_config_index.find(socket);
    if (it != socket_to_config_index.end())
    {
        return it->second;
    }
    return -1;
}

void WebServer::setActiveEvents(int events)
{
    _active_events = events;
}

int WebServer::getActiveEvents() const
{
    return _active_events;
}

int WebServer::init(std::vector<ServerConfig>& configs)
{
    if (configs.empty())
    {
        std::cerr << "Error: No server configurations provided" << std::endl;
        return -1;
    }

    m_configs = configs;
    m_sockets.resize(configs.size());

   memset(&pollfds_vec[0], 0, sizeof(struct pollfd) * maxfds);

    for (size_t i = 0; i < configs.size(); ++i)
    {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket <= 0)
        {
            perror("socket failed");
            return -1;
        }

        int optval = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            close(server_socket);
            return -1;
        }
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)))
        {
            perror("setsockopt(SO_REUSEPORT) failed");
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(configs[i].get_port());
        if (inet_pton(AF_INET, configs[i].get_host().c_str(), &hint.sin_addr) <= 0)
        {
            perror("Error: Invalid IP address format");
            close(server_socket);
            return -1;
        }

        if (bind(server_socket, (struct sockaddr *)&hint, sizeof(hint)) < 0)
        {
            std::cerr << "bind failed" << std::endl;
            close(server_socket);
            return -1;
        }

        if (listen(server_socket, SOMAXCONN) < 0)
        {
            std::cerr << "listen failed" << std::endl;
            close(server_socket);
            return -1;
        }

        m_sockets[i] = server_socket;
        socket_to_config_index[server_socket] = i;

        pollfds[numfds].fd = server_socket;
        pollfds[numfds].events = POLLIN;
        pollfds[numfds].revents = 0;
        numfds++;

        std::cout << "Server " << configs[i].get_server_name() 
                  << ": 'http://" << configs[i].get_host() 
                  << ":" << configs[i].get_port() << "'" << std::endl;
    }

    return 0;
}

int WebServer::run()
{
    bool running = true;
    time_t last_timeout_check = time(NULL);

    for (size_t i = 0; i < m_configs.size(); ++i)
    {
        std::cout << "Server '" << m_configs[i].get_server_name() 
                  << "' is listening on port: " << m_configs[i].get_port() << std::endl;
    }

    NotFound             eh0;
    BadRequest           eh1;
    InternalServerError  eh2;
    NotImplemented       eh3;
    MethodNotAllowed     eh4;
    Forbidden            eh5;
    TooManyRedirection   eh6;
    eh0.SetNext(&eh1)->SetNext(&eh2)->SetNext(&eh3)
       ->SetNext(&eh4)->SetNext(&eh5)->SetNext(&eh6);
    ErrorHandler* errorHandler = &eh0;

    while (running)
    {
        time_t current_time = time(NULL);
        if (current_time - last_timeout_check >= 2)
        {
            checkCgiTimeouts();
            last_timeout_check = current_time;
        }

        int ready = poll(pollfds, numfds, 1000);
        setActiveEvents(ready);

        std::vector<int> new_cgi_fds;
        for (std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.begin();
             it != CgiHandler::active_cgis.end(); ++it)
            {
                int cgi_fd = it->first;
                bool found = false;
                for (int i = 0; i < numfds; i++)
                {
                    if (pollfds[i].fd == cgi_fd)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    new_cgi_fds.push_back(cgi_fd);
                }
        }
        
        for (std::vector<int>::iterator it = new_cgi_fds.begin(); it != new_cgi_fds.end(); ++it)
        {
            addCgiToPoll(*it);
        }
        for (int i = 0; i < numfds; i++)
        {
            if (clients.find(pollfds[i].fd) != clients.end())
            {
                ClientConnection& conn = clients[pollfds[i].fd];
                if (conn.isStale(3000))
                { 
                    std::cout << "Client connection timed out, closing connection." << std::endl;
                    closeClientConnection(pollfds[i].fd);
                    continue;
                }
            }

            if (pollfds[i].revents == 0)
                continue;
            int fd = pollfds[i].fd;

            if (isCgiFd(fd))
            {
                if (pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                    handleCgiEvent(fd);
                }
                continue;
            }
            
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                if (isListeningSocket(fd))
                {
                    running = false;
                    break; 
                }
                else
                {
                    closeClientConnection(fd);
                }
                continue;
            }

            if (pollfds[i].revents & POLLIN)
            {
                if (isListeningSocket(fd))
                    acceptNewConnection(fd);
                else
                       handleClientRequest(fd);
            }
            if (pollfds[i].revents & POLLOUT)
            {
                try
                {
                    handleClientResponse(fd);
                }
                catch (const HttpException & e)
                {
                    this->updatePollEvents(fd, POLLOUT);
                    Error error(clients[fd], e.GetCode(), e.GetMessage(), e.GetErrorType());
                    errorHandler->HanldeError(error, this->getConfigForClient(fd));
                    closeClientConnection(fd);
                }
            }
        }
    }
    for (int i = 0; i < numfds; i++)
    {
        if (!isListeningSocket(pollfds[i].fd))
        {
            close(pollfds[i].fd);
        }
    }
    std::cout << "WebServer has shut down all servers." << std::endl;
    return 0;
}

void WebServer::acceptNewConnection(int listening_socket)
{
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(listening_socket, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientFd < 0)
    {
        std::cerr << "accept failed" << std::endl;
        return;
    }

    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    if (numfds >= maxfds - 10)
    {
        std::cerr << "Maximum connections reached (" << numfds << "/" << maxfds << "), rejecting client" << std::endl;
        close(clientFd);
        return;
    }

    try {
        int server_index = getServerIndexForSocket(listening_socket);
        if (server_index == -1)
        {
            std::cerr << "Unable to find server configuration for socket " << listening_socket << std::endl;
            close(clientFd);
            return;
        }

        ClientConnection conn(clientFd, clientAddr);
        conn._server = this;

        pollfds[numfds].fd = clientFd;
        pollfds[numfds].events = POLLIN;
        pollfds[numfds].revents = 0;
        numfds++;

        clients[clientFd] = conn;
        client_to_server_index[clientFd] = server_index;

        std::cout << "Client ip: " << clients[clientFd].ipAddress 
                  << " connected to server '" << m_configs[server_index].get_server_name() 
                  << "' (numfds=" << numfds << "/" << maxfds << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating client connection: " << e.what() << std::endl;
        close(clientFd);
        if (numfds > 0 && pollfds[numfds-1].fd == clientFd) {
            numfds--;
        }
    }
}

void WebServer::closeClientConnection(int clientSocket)
{
    std::map<int, ClientConnection>::iterator it = clients.find(clientSocket);
    if (it != clients.end())
    {
        printf("Client ip: %s disconnected\n", it->second.ipAddress.c_str());

        if (it->second.http_request != NULL)
        {
            delete it->second.http_request;
            it->second.http_request = NULL;
        }

        if (it->second.http_response != NULL) {
            delete it->second.http_response;
            it->second.http_response = NULL;
        }

        clients.erase(it);
        client_to_server_index.erase(clientSocket);
        
        close(clientSocket);

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

void WebServer::updatePollEvents(int fd, short events) {
    for (int i = 0; i < numfds; i++)
    {
        if (pollfds[i].fd == fd)
        {
            if (pollfds[i].events != events) {
                pollfds[i].events = events;
                pollfds[i].revents = 0;
            }
            return;
        }
    }
}

ServerConfig WebServer::getConfigByHost(std::string host) {
    for (size_t i = 0; i < m_configs.size(); ++i)
    {
        if (m_configs[i].get_server_name() == host)
            return m_configs[i];
    }
    return m_configs[0];
}

void WebServer::handleClientRequest(int fd)
{

    std::cout << "============== (START OF HANDLING CLIENT REQUEST) ==============\n";
    
    clients[fd].updateActivity();

    ClientConnection &client = clients[fd];

    NotFound             eh0;
    BadRequest           eh1;
    InternalServerError  eh2;
    NotImplemented       eh3;
    MethodNotAllowed     eh4;
    Forbidden            eh5;
    TooManyRedirection   eh6;
    eh0.SetNext(&eh1)->SetNext(&eh2)->SetNext(&eh3)
       ->SetNext(&eh4)->SetNext(&eh5)->SetNext(&eh6);
    ErrorHandler* errorHandler = &eh0;

    try
    {
        if (client.http_request == NULL)
        {
            client.GenerateRequest(fd);        
        }
        client.ProcessRequest(fd);
    }
    catch(HttpException &e)
    {
        try
        {
            Error error(client, e.GetCode(), e.GetMessage(), e.GetErrorType());
            errorHandler->HanldeError(error, this->getConfigForClient(fd));
            this->updatePollEvents(fd, POLLOUT);
        }
        catch (std::exception &ex)
        {
            closeClientConnection(fd);
        }
    }
    catch (...)
    {
        closeClientConnection(fd);
    }
}

void WebServer::handleClientResponse(int fd)
{
    ClientConnection &client = clients[fd];

    if (client.http_response == NULL)
    {
        updatePollEvents(fd, POLLIN);
        return;
    }
    std::cout << "============== (START OF HANDLING CLIENT RESPONSE) ==============\n";
    if (client.http_response->checkAvailablePacket())
    {
        if (client.http_response->isChunked())
        {
            try
            {
                client.http_response->sendChunkedResponse(fd);
                if (client.http_response->getByteSent() == client.http_response->getByteToSend())
                {
                    client.http_response->sendLastChunk(fd);
                    if (client.http_response->isKeepAlive())
                    {
                        client.http_response->clear();
                        client.http_request->ResetRequest();
                        this->updatePollEvents(fd, POLLIN);
                    }
                    else
                    {
                        closeClientConnection(fd);
                    }
                    return;
                }
            }
            catch (const HttpException& e)
            {
                std::cerr << "Error in chunked response: " << e.what() << std::endl;
                closeClientConnection(fd);
                return;
            }
        }
        else
        {
            if (client.http_response->isFile())
                client.http_response->sendResponse(fd);                
            else
            {
                if (client.http_request && client.http_request->IsRedirected())
                {
                    client.redirect_counter++;
                    if (client.redirect_counter > 10)
                    {
                        client.redirect_counter = 0;
                        Error error(client, 429, "Too Many Redirections", TOO_MANY_REDIRECTION);
                        TooManyRedirection tr;
                        tr.HanldeError(error, this->getConfigForClient(fd));  
                        client.should_close = true; 
                    }
                }
                else
                    client.redirect_counter = 0; 
                if (client.http_response->isFile())
                    client.http_response->sendResponse(fd);
                else  
                    client.http_response->sendChunkedResponse(fd);
                if (client.should_close)
                {
                    closeClientConnection(fd);
                    return;
                }
            }
            this->updatePollEvents(fd, POLLIN);
            if (client.http_response->isKeepAlive())
            {
                delete client.http_response;
                client.http_response = NULL;
                delete client.http_request;
                client.http_request = NULL;
                return;
            }
            else
            {
                closeClientConnection(fd);
                return;
            }
        }
    }
    else
        updatePollEvents(fd, POLLIN);
}

void WebServer::addCgiToPoll(int cgi_fd) {
    if (numfds >= maxfds - 2)
        return;
    
    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].fd == cgi_fd) {
            return;
        }
    }
    
    pollfds[numfds].fd = cgi_fd;
    pollfds[numfds].events = POLLIN;
    pollfds[numfds].revents = 0;
    numfds++;
}

void WebServer::removeCgiFromPoll(int cgi_fd) {
    for (int i = 0; i < numfds; i++) {
        if (pollfds[i].fd == cgi_fd) {
            pollfds[i] = pollfds[numfds - 1];
            numfds--;
            break;
        }
    }
}

bool WebServer::isCgiFd(int fd) {
    return CgiHandler::active_cgis.find(fd) != CgiHandler::active_cgis.end();
}

void WebServer::checkCgiTimeouts() {
    time_t current_time = time(NULL);
    std::vector<int> timed_out_fds;
    
    for (std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.begin();
         it != CgiHandler::active_cgis.end(); ++it) {
        
        if (current_time - it->second.start_time > 10) {
            timed_out_fds.push_back(it->first);
        }
    }
    
    for (std::vector<int>::iterator fd_it = timed_out_fds.begin(); 
         fd_it != timed_out_fds.end(); ++fd_it) {
        
        int cgi_fd = *fd_it;
        std::map<int, CgiHandler::CgiProcess>::iterator cgi_it = CgiHandler::active_cgis.find(cgi_fd);
        
        if (cgi_it != CgiHandler::active_cgis.end()) {
            kill(cgi_it->second.pid, SIGTERM);
            usleep(100000);
            
            int status;
            if (waitpid(cgi_it->second.pid, &status, WNOHANG) == 0) {
                kill(cgi_it->second.pid, SIGKILL);
                waitpid(cgi_it->second.pid, &status, 0);
            }
            
            std::map<std::string, std::string> headers;
            headers["Content-Type"] = "text/html";
            cgi_it->second.client->http_response = new HttpResponse(504, headers, "text/html", false, false);
            cgi_it->second.client->http_response->setBuffer("<html><body><h1>504 Gateway Timeout</h1><p>CGI script exceeded 10 second limit</p></body></html>");
            
            updatePollEvents(cgi_it->second.client->GetFd(), POLLOUT);
            
            close(cgi_fd);
            removeCgiFromPoll(cgi_fd);
            CgiHandler::active_cgis.erase(cgi_it);
        }
    }
}

void WebServer::handleCgiEvent(int fd) {
    std::map<int, CgiHandler::CgiProcess>::iterator it = CgiHandler::active_cgis.find(fd);
    if (it == CgiHandler::active_cgis.end()) {
        return;
    }
    
    CgiHandler::CgiProcess& cgi = it->second;
    
    if (time(NULL) - cgi.start_time > 10) {
        kill(cgi.pid, SIGKILL);
        waitpid(cgi.pid, NULL, 0);
        
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "text/html";
        cgi.client->http_response->setStatusCode(504);
        cgi.client->http_response->setHeaders(headers);
        cgi.client->http_response->setContentType("text/html");

        cgi.client->http_response->setBuffer("<html><body><h1>504 Gateway Timeout</h1></body></html>");
        
        updatePollEvents(cgi.client->GetFd(), POLLOUT);
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
        return;
    }
    
    char buffer[4096];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        cgi.output += buffer;
    } else if (bytes == 0) {
        
        if (cgi.output.length() > 0) {
            std::string preview = cgi.output.substr(0, 200);
        }
        int status;
        pid_t wait_result = waitpid(cgi.pid, &status, 0);
        
        if (wait_result == cgi.pid) {
            
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                
                if (exit_code == 0) {
                    
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
                    
                    
                    std::map<std::string, std::string> response_headers;
                    response_headers["Content-Type"] = "text/html";
                    
                    int status_code = 200;
                    std::string status_message = "OK";
                    std::vector<std::string> set_cookies;
                    
                    std::istringstream header_stream(headers);
                    std::string line;
                    
                    while (std::getline(header_stream, line)) {
                        if (!line.empty() && line[line.length() - 1] == '\r') {
                            line.erase(line.length() - 1);
                        }
                        
                        if (line.empty()) continue;
                        
                        size_t colon_pos = line.find(':');
                        if (colon_pos == std::string::npos) continue;
                        
                        std::string header_name = line.substr(0, colon_pos);
                        std::string header_value = line.substr(colon_pos + 1);
                        
                         while (!header_value.empty() &&
                                (header_value[0] == ' ' || header_value[0] == '\t')) {
                             header_value.erase(0, 1);
                         }
                         while (!header_value.empty()) {
                             char last = header_value[header_value.size() - 1];
                             if (last == '\r' || last == ' ' || last == '\t') {
                                 header_value.erase(header_value.size() - 1, 1);
                             } else {
                                 break;
                             }
                         }
                        
                        if (header_name == "Status") {
                            size_t space_pos = header_value.find(' ');
                            if (space_pos != std::string::npos) {
                                status_code = std::atoi(header_value.substr(0, space_pos).c_str());
                                status_message = header_value.substr(space_pos + 1);
                            } else {
                                status_code = std::atoi(header_value.c_str());
                            }
                        } 
                        else if (header_name == "Content-Type" || header_name == "Content-type") {
                            response_headers["Content-Type"] = header_value;
                        } 
                        else if (header_name == "Location") {
                            response_headers["Location"] = header_value;
                        }
                        else if (header_name == "Set-Cookie") {
                            set_cookies.push_back(header_value);
                        }
                        else {
                            response_headers[header_name] = header_value;
                        }
                    }
                    
                    cgi.client->http_response->setStatusCode(status);
                    cgi.client->http_response->setHeaders(response_headers);
                    cgi.client->http_response->setContentType(response_headers["Content-Type"]);
                    for (std::vector<std::string>::iterator cookie_it = set_cookies.begin(); 
                        cookie_it != set_cookies.end(); ++cookie_it) {
                        cgi.client->http_response->setHeader("Set-Cookie", *cookie_it);
                    }
                    
                    cgi.client->http_response->setBuffer(body);
                    
                    updatePollEvents(cgi.client->GetFd(), POLLOUT);
                } else {
                    
                    std::map<std::string, std::string> error_headers;
                    error_headers["Content-Type"] = "text/html";
                    cgi.client->http_response->setStatusCode(500);
                    cgi.client->http_response->setHeaders(error_headers);
                    cgi.client->http_response->setContentType("text/html");
                    cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Exit code: " + std::string(1, '0' + exit_code) + "</p></body></html>");
                    updatePollEvents(cgi.client->GetFd(), POLLOUT);
                }
            } else if (WIFSIGNALED(status)) {
                int signal_num = WTERMSIG(status);
                
                std::map<std::string, std::string> error_headers;
                error_headers["Content-Type"] = "text/html";
                cgi.client->http_response->setStatusCode(500);
                cgi.client->http_response->setHeaders(error_headers);
                cgi.client->http_response->setContentType("text/html");
                cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Killed by signal: " + std::string(1, '0' + signal_num) + "</p></body></html>");
                updatePollEvents(cgi.client->GetFd(), POLLOUT);
            } else {
                
                std::map<std::string, std::string> error_headers;
                error_headers["Content-Type"] = "text/html";
                cgi.client->http_response->setStatusCode(500);
                cgi.client->http_response->setHeaders(error_headers);
                cgi.client->http_response->setContentType("text/html");
                cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>Process ended abnormally</p></body></html>");
                updatePollEvents(cgi.client->GetFd(), POLLOUT);
            }
        } else {
            
            std::map<std::string, std::string> error_headers;
            error_headers["Content-Type"] = "text/html";
            cgi.client->http_response->setStatusCode(500);
            cgi.client->http_response->setHeaders(error_headers);
            cgi.client->http_response->setContentType("text/html");
            cgi.client->http_response->setBuffer("<html><body><h1>500 CGI Error</h1><p>waitpid failed</p></body></html>");
            updatePollEvents(cgi.client->GetFd(), POLLOUT);
        }
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
    } else if (bytes < 0) {
        
        close(fd);
        removeCgiFromPoll(fd);
        CgiHandler::active_cgis.erase(it);
    }
}

void WebServer::updateClientServerMapping(int client_fd, const ServerConfig& config)
{
    for (size_t i = 0; i < m_configs.size(); ++i)
    {
        if (m_configs[i].get_server_name() == config.get_server_name() && 
            m_configs[i].get_port() == config.get_port())
        {
            client_to_server_index[client_fd] = i;
            std::cout << "Updated mapping: client " << client_fd << " -> server " 
                      << config.get_server_name() << std::endl;
            return;
        }
    }
}


ServerConfig WebServer::getConfigByIpPortAndHost(const std::string& ip, int port, const std::string& host) 
{
    std::vector<ServerConfig> matching_configs;
    
    
    for (size_t i = 0; i < m_configs.size(); ++i)
    {
        if (m_configs[i].get_host() == ip && m_configs[i].get_port() == port)
        {
            matching_configs.push_back(m_configs[i]);
        }
    }
    
    if (matching_configs.empty()) {
        return m_configs[0];
    }
    
    for (std::vector<ServerConfig>::iterator it = matching_configs.begin(); 
         it != matching_configs.end(); ++it)
    {
        if (it->get_server_name() == host)
        {
            return *it;
        }
    }
    return matching_configs[0];
}