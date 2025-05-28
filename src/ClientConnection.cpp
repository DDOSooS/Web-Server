#include "../include/ClientConnection.hpp"
#include "../include/WebServer.hpp"
#include "../include/request/Get.hpp"
#include "../include/request/Post.hpp"
#include "../include/request/CgiHandler.hpp"
#include <cstring>  

ClientConnection::ClientConnection(): fd(-1), ipAddress(""), port(0), connectTime(0), lastActivity(0)
            , builder(NULL), http_response(NULL), http_request(NULL), handler_chain(NULL)
{
    this->_server = NULL;
}

// Constructor with socket and client address
ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) :
    _server(NULL),
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(NULL)),
    lastActivity(time(NULL)),
    builder(NULL),
    http_response(NULL),
    http_request(NULL),
    handler_chain(NULL)
{
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    ipAddress = ipStr;
}

void ClientConnection::GenerateRequest(int fd)
{
    char buffer[REQUSET_LINE_BUFFER];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0)
    {
        std::cerr << "Error receiving data: "
                  << (bytesRead == 0 ? "Connection closed" : strerror(errno))
                  << "\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }    
    // Null-terminate the buffer
    buffer[bytesRead] = '\0';

    std::string rawRequest(buffer);
    std::cout << "==================== (Buffer:) =====================\n " << buffer <<
        "\n==================================================================" << std::endl;

    // Parse headers first
    HttpRequestBuilder build = HttpRequestBuilder();
    build.ParseRequest(rawRequest, this->_server->getServerConfig());
    
    // Check if we need to read more data (for POST requests with Content-Length)
    std::string contentLengthStr = build.GetHttpRequest().GetHeader("Content-Length");
    if (!contentLengthStr.empty()) {
        long contentLength = atol(contentLengthStr.c_str());
        std::cout << "Content-Length header found: " << contentLength << " bytes" << std::endl;
        
        // Find the body start position (after \r\n\r\n)
        size_t bodyStart = rawRequest.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 4; // Move past \r\n\r\n
            // Calculate how much of the body we've already read
            size_t bodyBytesRead = rawRequest.length() - bodyStart;
            std::cout << "Already read " << bodyBytesRead << " bytes of body" << std::endl;
            
            // If we need to read more of the body
            if (bodyBytesRead < (size_t)contentLength) {
                size_t remainingBytes = contentLength - bodyBytesRead;
                std::cout << "Need to read " << remainingBytes << " more bytes" << std::endl;
                
                // Read the rest of the body in chunks
                char* bodyBuffer = new char[remainingBytes + 1];
                size_t totalBodyBytesRead = 0;
                
                while (totalBodyBytesRead < remainingBytes) {
                    ssize_t chunkRead = recv(fd, bodyBuffer + totalBodyBytesRead, 
                                            remainingBytes - totalBodyBytesRead, 0);
                    if (chunkRead <= 0) {
                        delete[] bodyBuffer;
                        std::cerr << "Error reading request body: " 
                                    << (chunkRead == 0 ? "Connection closed" : strerror(errno))
                                    << std::endl;
                        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
                    }
                    totalBodyBytesRead += chunkRead;
                    std::cout << "Read " << chunkRead << " bytes, total: " 
                                << totalBodyBytesRead << "/" << remainingBytes << std::endl;
                }
                
                // Null-terminate the body buffer
                bodyBuffer[totalBodyBytesRead] = '\0';
                
                // Append the body to the raw request
                std::string fullBody(bodyBuffer, totalBodyBytesRead);
                delete[] bodyBuffer;
                
                // Set the full body in the request
                build.SetBody(rawRequest.substr(bodyStart) + fullBody);
                std::cout << "Full body size: " << build.GetHttpRequest().GetBody().size() << " bytes" << std::endl;
            }
        }
    }
    
    // Clean up any existing request object
    if (this->http_request)
    {
        delete this->http_request;
    }
    
    this->http_request = new HttpRequest(build.GetHttpRequest());
    this->http_request->SetClientData(this);

}

//@todo: Consider logical work flow before !!!!! refactoring this function to handle errors more gracefully
void    ClientConnection::ProcessRequest(int fd)
{
    RequestHandler     *chain_handler;

    chain_handler = new CgiHandler(this);
    (chain_handler->SetNext(new Get()))->SetNext(new Post());
    if (http_request == NULL)
    {
        std::cerr << "Error: No request to process" << std::endl;
        return;
    }

    // Ensure http_response is properly initialized before proceeding
    if (this->http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        this->http_response = new HttpResponse(200, emptyHeaders, "text/plain", false, false);
    }
    
    chain_handler->HandleRequest(this->http_request);
    std::cout << "END OF PROCESSING THE REQUEST \n";
    if (this->_server != NULL)
    {
        this->_server->updatePollEvents(fd, POLLOUT);
    } 
    else
    {
        std::cerr << "Error: Server pointer is NULL" << std::endl;
    }
       
    delete chain_handler;
}


ClientConnection::~ClientConnection()
{
    // Clean up allocated resources in a safe way
    if (http_request != NULL) {
        delete http_request;
        http_request = NULL;
    }
    
    if (http_response != NULL) {
        delete http_response;
        http_response = NULL;
    }
    
    if (builder != NULL) {
        delete builder;
        builder = NULL;
    }
    
    // Don't delete handler_chain here, it could be a dangling pointer
    // or managed elsewhere
}

/*
ClientConnection::ClientConnection() : fd(-1), port(0), connectTime(0), lastActivity(0)
            , bytesSent(0),  http_request(NULL){}

ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) :
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(NULL)),
    lastActivity(time(NULL)),
    bytesSent(0),
    http_request(NULL)
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
            return "NULL";
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

    std::map<std::string , std::string> RequestData::getAllHeaders() const
    {
        return headers;
    }

*/

// Update activity timestamp
void ClientConnection::updateActivity()
{
    lastActivity = time(NULL);
}

// Check if connection is stale (timeout)
bool ClientConnection::isStale(time_t timeoutSec) const
{
    return (time(NULL) - lastActivity) > timeoutSec;
}
