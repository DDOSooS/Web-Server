#include "../include/ClientConnection.hpp"
#include "../include/WebServer.hpp"
#include "../include/request/Get.hpp"
#include "../include/request/Post.hpp"
#include "../include/request/CgiHandler.hpp"
#include "../include/request/Delete.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include <errno.h>
#include <set>
#include <iomanip>
#include <chrono>

int ClientConnection::redirect_counter = 0;

ClientConnection::ClientConnection() 
    : fd(-1), ipAddress(""), port(0), connectTime(0), lastActivity(0),
      builder(NULL), http_response(NULL), http_request(NULL)
      , temp_upload_fd(-1), should_close(false)
{
    this->_server = NULL;
}

ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) 
    : _server(NULL), fd(socketFd), port(ntohs(clientAddr.sin_port)),
      connectTime(time(NULL)), lastActivity(time(NULL)),
      builder(NULL), http_response(NULL), http_request(NULL),
     temp_upload_fd(-1), should_close(false)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

void ClientConnection::setServerConfig(const ServerConfig& config)
{
    server_config = config;
}

ServerConfig ClientConnection::getServerConfig() const
{
    return server_config;
}

ClientConnection::~ClientConnection()
{
    if (http_request != NULL)
    {
        delete http_request;
        http_request = NULL;
    }
    
    if (http_response != NULL)
    {
        delete http_response;
        http_response = NULL;
    }
    
    if (builder != NULL)
    {
        delete builder;
        builder = NULL;
    }
}


/*
------geckoformboundary7de324a472cdf0c02af4e4199d73dc0d
Content-Disposition: form-data; name="upload_file"; filename="1747505457077.jpeg"
Content-Type: image/jpeg

����
------------------------
*/

void ClientConnection::GenerateRequest(int fd)
{
    std::cout << "Socket Fd: " << fd << "=====" << std::endl;
    char buffer[REQUSET_LINE_BUFFER];
    size_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead < 0)
    {
        std::cerr << "Error receiving data: "
                  << (bytesRead == 0 ? "Connection closed" : strerror(errno))
                  << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    std::string rawRequest(buffer, bytesRead);  // This preserves all bytes including nulls
    HttpRequestBuilder build = HttpRequestBuilder();
    build.ParseRequest(rawRequest, this->_server->getConfigForClient(this->GetFd()), fd);
    
    // Create final HTTP request object
    if (this->http_request)
    {
        delete this->http_request;
    }
    this->http_request = new HttpRequest(build.GetHttpRequest());

    // checking if the method is nor valid
    ServerConfig client_config = this->_server->getConfigForClient(fd);
    const Location *cur_location = client_config.findMatchingLocation(this->http_request->GetLocation());
    std::vector<std::string> allowed_methods = cur_location ? cur_location->get_allowMethods() : std::vector<std::string>();
    int flag = 0;

    flag = cur_location->get_return().size() > 0 ? 1 : 0;
    if (std::find(allowed_methods.begin(), allowed_methods.end(), this->http_request->GetMethod()) == allowed_methods.end() && !flag)
    {
        std::cerr << "Method not allowed: " << this->http_request->GetMethod() << std::endl;
        throw HttpException(405, "Method Not Allowed", METHOD_NOT_ALLOWED);
        return;
    }

    this->http_request->SetClientData(this);
    this->setServerConfig(this->_server->getConfigForClient(this->GetFd()));
    std::cout << "Server Name is: " << this->getServerConfig().get_server_name() << "=============\n\n\n" << std::endl;
    std::cout << "Server Root is: " << this->getServerConfig().get_root() << "=============\n\n\n" << std::endl;
}

void ClientConnection::ProcessRequest(int fd)
{
    RequestHandler *chain_handler = NULL;
    
    if (http_request == NULL)
    {
        std::cerr << "No request to process" << std::endl;
        return;
    }
    if (this->http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        this->http_response = new HttpResponse(200, emptyHeaders, "text/plain", false, false);
    }
    chain_handler = new CgiHandler(this);
    chain_handler->SetNext(new Get())
                ->SetNext(new Post())
                ->SetNext(new Delete());
    chain_handler->HandleRequest(this->http_request, 
                                this->_server->getConfigForClient(this->GetFd()), 
                                this->server_config);
    
    // update client file descriptor to POLLOUT
    if (!this->http_request->IsStreamingUpload())
        this->_server->updatePollEvents(fd, POLLOUT);
    else
        this->_server->updatePollEvents(fd, POLLIN);
    // Clean up handler chain
    while (chain_handler->GetNext() != NULL)
    {
        RequestHandler *next_handler = chain_handler->GetNext();
        chain_handler->SetNext(NULL);
        delete chain_handler; 
        chain_handler = next_handler; 
    }
    if (chain_handler != NULL) 
        delete chain_handler;
}

void ClientConnection::updateActivity()
{
    lastActivity = time(NULL);
}

bool ClientConnection::isStale(time_t timeoutSec) const
{
    return (time(NULL) - lastActivity) > timeoutSec;
}

void ClientConnection::RespondToClient(int fd)
{
    (void)fd;
}

void ClientConnection::parseRequest(char *buff)
{
    (void)buff;
}

int ClientConnection::GetFd() const 
{
    return fd;
}