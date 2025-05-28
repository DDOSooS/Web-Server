#include "../../include/request/CgiHandler.hpp"
#include "../../include/request/RequestHandler.hpp"
#include "../../include/WebServer.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/request/RequestHandler.hpp"
#include <iostream>
#include <stdexcept>
#include "../../include/config/ServerConfig.hpp"
#include "../../include/config/Location.hpp"

CgiHandler::CgiHandler(ClientConnection* client) : _client(client) {}
CgiHandler::~CgiHandler() {}

bool CgiHandler::isCgiRequest(HttpRequest *request) const {
    // Keep your existing implementation
    std::string request_path = request->GetLocation();
    std::cout << "=== CGI Detection Debug ===" << std::endl;
    std::cout << "Request path: " << request_path << std::endl;
    
    const Location* matching_location = _client->_server->getServerConfig().findMatchingLocation(request_path);
    
    if (!matching_location) {
        std::cout << "No matching location found" << std::endl;
        return false;
    }
    
    std::cout << "Found matching location: " << matching_location->get_path() << std::endl;
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        std::cout << "Configured extension[" << i << "]: '" << cgi_extensions[i] << "' -> (" << cgi_paths[i] << ")"<< std::endl;
    }
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        std::cout << "❌No CGI configured for this location" << std::endl;
        return false;
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        std::cout << "❌No file extension found" << std::endl;
        return false;
    }
    
    std::string extension = request_path.substr(dot_pos);
    std::cout << "Request extension: '" << extension << "'" << std::endl;
    
    for (std::vector<std::string>::const_iterator it = cgi_extensions.begin(); 
         it != cgi_extensions.end(); ++it) {
        std::cout << "Comparing '" << extension << "' with '" << *it << "'" << std::endl;
        if (*it == extension) {
            std::cout << "✅CGI match found for extension: " << extension << std::endl;
            return true;
        }
    }
    
    std::cout << "❌No CGI extension match found" << std::endl;
    return false;
}

bool CgiHandler::CanHandle(std::string method) {
    return ((method == "POST" || method == "GET") && isCgiRequest(_client->http_request));
}

void CgiHandler::ProccessRequest(HttpRequest *request) {
    if (!isCgiRequest(request)) {
        std::cout << "Not a CGI request, passing to next handler" << std::endl;
        if (this->GetNext()) {
            this->GetNext()->HandleRequest(request);
        }
        return;
    }
    try {
            std::cout << "CGI process started successfully - streaming output directly" << std::endl;
            std::string response = executeCgiScript(request);
            request->GetClientDatat()->http_response->setStatusCode(200);
            request->GetClientDatat()->http_response->setStatusMessage("OK");
            request->GetClientDatat()->http_response->setChunked(false);
            request->GetClientDatat()->http_response->setBuffer(response);
            request->GetClientDatat()->http_response->setContentType("text/html");
    } catch (const std::exception &e) {
        std::cerr << "Error in CGI processing: " << e.what() << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
}
char ** CgiHandler::setGgiEnv(HttpRequest *request){
   char *envp[] =
        {
            "HOME=/",
            "PATH=/bin:/usr/bin",
            "TZ=UTC0",
            "USER=beelzebub",
            "LOGNAME=tarzan",
            0
        };
    return envp;
}

std::string CgiHandler::executeCgiScript(HttpRequest *request) {
    int pipe_in[2];
    int pie_out[2];
    if(pipe(pipe_in) == -1 || pipe(pie_out) == -1){
        std::cerr << "Error in CGI pipe" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    int cgi_pid = fork();
    if (cgi_pid < 0){
        std::cerr << "Error in CGI Fork" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    if (cgi_pid == 0){
        
    }
    return ("<h1 color='red'> Hello from CGI handler </h1>");
}


bool CgiHandler::cgiExec(){
    // setup env
    // create pipe
    // execute script
    return (true);
}
