// src/request/Cgi.cpp
#include "../../include/request/Cgi.hpp"
#include "../../include/request/RequestHandler.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/request/CgiHandler.hpp"
#include "../../include/request/HttpException.hpp"
#include "../../include/ClientConnection.hpp"
#include <iostream>

Cgi::Cgi(CgiHandler* handler) : RequestHandler(), cgi_handler(handler) {
    std::cout << "CGI handler initialized" << std::endl;
}

Cgi::~Cgi() {}

bool Cgi::CanHandle(std::string method) {
    // CGI can handle GET, POST, and DELETE methods
    return (method == "GET" || method == "POST" || method == "DELETE");
}

void Cgi::ProccessRequest(HttpRequest* request) {
    std::cout << "=== CGI PROCESSING REQUEST ===" << std::endl;
    
    if (!request) {
        std::cerr << "Invalid request object" << std::endl;
        return;
    }
    
    std::string path = request->GetLocation();
    std::string method = request->GetMethod();
    
    std::cout << "CGI Request - Method: " << method << ", Path: " << path << std::endl;
    
    // Check if this is a CGI request
    if (!isCgiPath(path)) {
        std::cout << "Not a CGI path, passing to next handler" << std::endl;
        // Pass to next handler in chain
        if (this->_nextHandler) {
            this->_nextHandler->HandleRequest(request);
        }
        return;
    }
    
    // Check if CGI handler is available
    if (!cgi_handler) {
        std::cerr << "CGI handler not available" << std::endl;
        handleCgiError(request, "CGI not configured");
        return;
    }
    
    // Get client connection
    ClientConnection* client = request->GetClientDatat();
    if (!client) {
        std::cerr << "No client connection available" << std::endl;
        handleCgiError(request, "No client connection");
        return;
    }
    
    try {
        // Build script path
        std::string script_path = getScriptPath(path);
        
        // Build query string from URL parameters
        std::string query_string = buildQueryString(request);
        
        // Get request body
        std::string body = request->GetBody();
        
        std::cout << "Starting CGI:" << std::endl;
        std::cout << "  Script: " << script_path << std::endl;
        std::cout << "  Method: " << method << std::endl;
        std::cout << "  Query: " << query_string << std::endl;
        std::cout << "  Body size: " << body.size() << " bytes" << std::endl;
        
        // Start CGI process
        int cgi_fd = cgi_handler->startCgi(
            method,
            script_path,
            query_string,
            body,
            client->fd
        );
        
        if (cgi_fd > 0) {
            std::cout << "CGI process started successfully, fd: " << cgi_fd << std::endl;
            
            // Set request status to indicate CGI is processing
            request->SetStatus(PROCESSING);
            
            // IMPORTANT: Don't call next handler or set POLLOUT
            // The WebServer poll loop will handle CGI output when ready
            return;
        } else {
            std::cerr << "Failed to start CGI process" << std::endl;
            handleCgiError(request, "CGI execution failed");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in CGI processing: " << e.what() << std::endl;
        handleCgiError(request, "Internal CGI error");
    }
}

// Private helper methods

bool Cgi::isCgiPath(const std::string& path) const {
    // Check if path has CGI extension
    if (!cgi_handler) return false;
    
    return cgi_handler->isCgiRequest(path);
}

std::string Cgi::getScriptPath(const std::string& request_path) const {
    // Return the request path directly
    // The CgiHandler will combine it with cgi_path from config
    return request_path;
}

std::string Cgi::buildQueryString(HttpRequest* request) const {
    std::string query_string;
    
    // Get query parameters from request
    std::vector<std::pair<std::string, std::string> > query_params = request->GetQueryString();
    
    for (size_t i = 0; i < query_params.size(); ++i) {
        if (i > 0) query_string += "&";
        query_string += query_params[i].first + "=" + query_params[i].second;
    }
    
    return query_string;
}

void Cgi::handleCgiError(HttpRequest* request, const std::string& error_msg) {
    std::cerr << "CGI Error: " << error_msg << std::endl;
    
    // Set request status back to allow error handling
    request->SetStatus(ERROR);
    
    // Throw HTTP exception to be handled by error handler chain
    throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
}