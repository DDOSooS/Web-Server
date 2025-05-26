#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "RequestHandler.hpp"
#include <string>

// Forward declarations for other classes
class WebServer;
class HttpRequest;

class CgiHandler : public RequestHandler {
    private:
        WebServer* m_webServer;
        std::string executeCgiScript(HttpRequest *request);
    public:
        CgiHandler(WebServer* webServer);
        ~CgiHandler();
        void HandleRequest(HttpRequest *request);
        bool CanHandle(std::string method);
        void ProccessRequest(HttpRequest *request);
};

#endif // CGI_HANDLER_HPP