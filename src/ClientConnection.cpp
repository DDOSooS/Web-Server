#include "../include/ClientConnection.hpp"
#include <iostream>
// Constructor
// Default constructor

ClientConnection::ClientConnection(): fd(-1), port(0), ipAddress(0), connectTime(0), lastActivity(0)
            , http_request(nullptr), http_response(nullptr), handler_chain(nullptr) {}

// Constructor with socket and client address
ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) : 
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(nullptr)),
    lastActivity(time(nullptr)),
    http_request(nullptr)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

void ClientConnection::GenerateRequest(int fd)
{
    char buffer[REQUSET_LINE_BUFFER];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0)
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
    else if (bytesRead == 0)
    {
        //throw internal server error
    }
    std::string rawRequest;

    rawRequest = std::string(buffer);
    HttpRequestBuilder build = HttpRequestBuilder();
    build.ParseRequest(rawRequest);

}

ClientConnection::~ClientConnection()
{
    if (http_request)
        delete http_request;
    if (http_response)
        delete http_response;
    if (builder)
        delete builder;
    if (handler_chain)
        delete handler_chain;
}



/*


ClientConnection::ClientConnection() : fd(-1), port(0), connectTime(0), lastActivity(0)
            , bytesSent(0),  http_request(NULL){}

ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) : 
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(nullptr)),
    lastActivity(time(nullptr)),
    bytesSent(0),
    http_request(nullptr)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
    }    
    
    void ClientConnection::parseRequest(char *buff)
    {
        std::stringstream   stream;
        // std::istringstream  iss;
        std::string         tmp;
        size_t              pos;
    
        if(!http_request)
            this->http_request =  new RequestData();
        tmp = std::string(buff);
        if (tmp.find("\r\n") != std::string::npos)
            http_request->request_line = true;
        stream.str(tmp);
        std::istringstream iss(tmp);
        iss >> http_request->method >> http_request->request_path >> http_request->http_version ;
        // std::cout << iss;
        // std::cout << "TMP :" << tmp << std::endl;
        // std::cout << "Method: " << http_request->method << std::endl;
        // std::cout << "Path: " << http_request->request_path << std::endl;
        // std::cout << "Version: " << http_request->http_version << std::endl;
        std::getline(stream, tmp);
        while (std::getline(stream, tmp))
        {
            pos = tmp.find(":");
            if (pos != std::string::npos)
                http_request->headers.insert({tmp.substr(0,pos), tmp.substr(pos + 2)});
        }
        if (std::string(buff).find("\r\n\r\n") != std::string::npos)
            http_request->crlf_flag = true;
        if (http_request->findHeader(std::string("Content-Length")))
        {
            http_request->content_length =  atol(http_request->getHeader("Contenet-Length").c_str());
        }
        stream.clear();
        stream.str("");
    }
    
    RequestData::RequestData(): method(""), request_path(""), http_version(""), request_body(""),content_length(0), keep_alive(false), crlf_flag(false), request_line(false), request_status(0), remaine_bytes(0) {}
    
    bool RequestData::findHeader(std::string header)
    {
        auto res = headers.find(header);
        return   res != headers.end() ;
    }
    
    std::string RequestData::getHeader(std::string header) const
    {
        auto res = headers.find(header);
        if (res ==  headers.end())
            return "nullptr";
        return res->second;
    }
    
    bool RequestData::findHeaderValue(std::string header, std::string value)
    {
        auto res = headers.find(header);
        if (res != headers.end())
        {
            if (res->second ==  value)
                return true;
        }
        return false;
    }
    
    std::unordered_map<std::string , std::string> RequestData::getAllHeaders() const
    {
        return headers;
    }

*/

// Update activity timestamp
void ClientConnection::updateActivity()
{
    lastActivity = time(nullptr);
}    

// Check if connection is stale (timeout)
bool ClientConnection::isStale(time_t timeoutSec) const
{
    return (time(nullptr) - lastActivity) > timeoutSec;
}    
