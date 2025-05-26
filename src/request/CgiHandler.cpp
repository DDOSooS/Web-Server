#include "../../include/request/CgiHandler.hpp"
#include "../../include/request/RequestHandler.hpp"
#include "../../include/WebServer.hpp"
#include "../../include/request/HttpRequest.hpp"
#include <iostream>
#include <stdexcept>
#include "../../include/config/ServerConfig.hpp"

CgiHandler::CgiHandler(WebServer* webServer) : m_webServer(webServer) {}
CgiHandler::~CgiHandler() {}


void CgiHandler::HandleRequest(HttpRequest *request) {
    try {
        std::string response = executeCgiScript(request);
        request->SetBody(response);
        request->GetClientDatat()->http_response->setStatusCode(200);
        request->GetClientDatat()->http_response->setStatusMessage("OK");
        request->GetClientDatat()->http_response->setContentType("text/html");
        request->GetClientDatat()->http_response->setChunked(false);
        request->GetClientDatat()->http_response->setBuffer(response);
    } catch (const std::exception &e) {
        std::cerr << "Error handling CGI request: " << e.what() << std::endl;
        request->GetClientDatat()->http_response->setStatusCode(500);
        request->GetClientDatat()->http_response->setStatusMessage("Internal Server Error");
        request->GetClientDatat()->http_response->setContentType("text/plain");
        request->GetClientDatat()->http_response->setBuffer("Internal Server Error");
    }
}

bool CgiHandler::CanHandle(std::string method) {
    // Check if the method is POST or GET, which are commonly used with CGI scripts
    return (method == "POST" || method == "GET");
}

void CgiHandler::ProccessRequest(HttpRequest *request) {
    // This method is called to process the request.
    // In this case, it will just call HandleRequest.
    HandleRequest(request);
}
std::string CgiHandler::executeCgiScript(HttpRequest *request) {
    // This function should execute the CGI script and return the output.
    // For simplicity, we will just return a dummy response.
    // In a real implementation, you would use fork/exec to run the CGI script.
    
    std::string scriptOutput = "<html><body><h1>CGI Script Output</h1>";
    scriptOutput += "<p>Method: " + request->GetMethod() + "</p>";
    scriptOutput += "<p>Location: " + request->GetLocation() + "</p>";
    scriptOutput += "<p>Body: " + request->GetBody() + "</p>";
    scriptOutput += "<h1>Note: In a real-world scenario, you would need to handle the execution of the CGI script here.</h1>";
    scriptOutput += "<p>This is a placeholder response.</p>";
    scriptOutput += "<p>Headers:</p><ul>";
    scriptOutput += "</body></html>";
    return scriptOutput;
}
// Note: In a real-world scenario, you would need to handle the execution of the CGI script here.
// This might involve using fork/exec to run the script and capture its output.
// For now, this is a placeholder implementation that simulates CGI script execution.
// You would replace this with actual logic to execute the CGI script and return its output.        