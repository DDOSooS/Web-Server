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
#include <set>
#include <iomanip>

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

void ClientConnection::GenerateRequest(int fd)
{
    std::cout << "Socket Fd: " << fd << "=====" << std::endl;
    char buffer[REQUSET_LINE_BUFFER];
    size_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (!bytesRead)
    {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    std::string rawRequest(buffer, bytesRead);
    HttpRequestBuilder build = HttpRequestBuilder();

    build.ParseRequest(rawRequest, this->_server->getConfigForClient(this->GetFd()), fd);
    if (this->http_request)
    {
        delete this->http_request;
    }
    this->http_request = new HttpRequest(build.GetHttpRequest());
    
    ServerConfig base_config = this->_server->getConfigForClient(fd);
    std::string server_ip = base_config.get_host();
    int server_port = base_config.get_port();
    std::string host_header = this->http_request->GetHeader("Host");
    
    ServerConfig final_config = base_config;
    if (!host_header.empty())
    {
        size_t pos = host_header.find(':');
        if (pos != std::string::npos)
        {
            host_header = host_header.substr(0, pos);
        }
        ServerConfig matched_config = this->_server->getConfigByIpPortAndHost(server_ip, server_port, host_header);
        final_config = matched_config;
        this->_server->updateClientServerMapping(fd, matched_config);
    }    
    this->setServerConfig(final_config);

    const Location *cur_location = final_config.findBestMatchingLocation(this->http_request->GetLocation());
    std::vector<std::string> allowed_methods = cur_location ? cur_location->get_allowMethods() : std::vector<std::string>();
    int flag = cur_location->get_return().size() > 0 ? 1 : 0;
    if (std::find(allowed_methods.begin(), allowed_methods.end(), this->http_request->GetMethod()) == allowed_methods.end() && !flag)
    {
        throw HttpException(405, "Method Not Allowed", METHOD_NOT_ALLOWED);
        return;
    }
    this->http_request->SetClientData(this);
}

void ClientConnection::ProcessRequest(int fd)
{
    if (http_request == NULL)
    {
        return;
    }
    if (this->http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        this->http_response = new HttpResponse(200, emptyHeaders, "text/plain", false, false);
    }
    CgiHandler cgi(this);
    Get        getH;
    Post       postH;
    Delete     delH;
    cgi.SetNext(&getH)->SetNext(&postH)->SetNext(&delH);
    cgi.HandleRequest(this->http_request,
                      this->_server->getConfigForClient(this->GetFd()),
                      this->server_config);
    
    // update client file descriptor to POLLOUT
    if (!this->http_request->IsStreamingUpload())
        this->_server->updatePollEvents(fd, POLLOUT);
    else
        this->_server->updatePollEvents(fd, POLLIN);
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