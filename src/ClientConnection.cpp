#include "../include/ClientConnection.hpp"
#include "./WebServer.hpp"
#include "./request/Get.hpp"
// Constructor
// Default constructor

ClientConnection::ClientConnection(): fd(-1), ipAddress(0), port(0), connectTime(0), lastActivity(0)
            , http_response(NULL), http_request(NULL), handler_chain(NULL) 
{
    this->_server = NULL;
}

// Constructor with socket and client address
ClientConnection::ClientConnection(int socketFd, const sockaddr_in& clientAddr) : 
    _server(NULL),
    fd(socketFd),
    port(ntohs(clientAddr.sin_port)),
    connectTime(time(nullptr)),
    lastActivity(time(nullptr)),
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
    if (bytesRead > 0)
        buffer[bytesRead] = '\0';// Null-terminate the buffer
    else if (bytesRead == 0)
    {
        std::cerr << "Internal Server EROOOOOOOOOOOOOR Generate Request\n";
        throw HttpException(500 , "Internal Server Error", ERROR_TYPE::INTERNAL_SERVER_ERROR);
    }

    std::cout << "==================== (Buffer:) =====================\n " << buffer <<
        "\n==================================================================" << std::endl;

    std::string rawRequest;

    rawRequest = std::string(buffer);
    HttpRequestBuilder build = HttpRequestBuilder(); 
    build.ParseRequest(rawRequest);
    this->http_request = new HttpRequest(build.GetHttpRequest());
    this->http_request->SetClientData(this);
}


void    ClientConnection::ProcessRequest(int fd)
{
    RequestHandler     *chain_handler;

    chain_handler = new Get();

    if (http_request == NULL)
    {
        std::cerr << "Error: No request to process" << std::endl;
        return;
    }

    if (!this->http_response)
    {
        this->http_response = new HttpResponse(200,{}, "text/plain", false, false);
    }
    chain_handler->HandleRequest(this->http_request);
    // std::unordered_map <std::string , std::string> headers;
    // // headers["Content-Type"] = "text/html";
    // this->http_response = new HttpResponse(200, headers, "text/html", false, false);
    // this->http_response->setFilePath("index.html");
    // std::cout << "END OF PROCESSING THE REQUEST \n";
    this->_server->updatePollEvents(fd, POLLOUT);
}


ClientConnection::~ClientConnection()
{
    // if (http_request)
    //     delete http_request;
    // if (http_response)
    //     delete http_response;
    // if (builder)
    //     delete builder;
    /*
        delete the handler chain
    */
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
