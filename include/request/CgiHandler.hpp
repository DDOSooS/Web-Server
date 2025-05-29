#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "RequestHandler.hpp"
#include <string>

// Forward declarations for other classes
class ClientConnection;
class HttpRequest;

class CgiHandler : public RequestHandler {
    private:
        ClientConnection* _client;

        std::string executeCgiScript(HttpRequest *request);

        bool isCgiRequest(HttpRequest *) const;
    public:
        CgiHandler(ClientConnection *);
        ~CgiHandler();


        bool CanHandle(std::string method);
        void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig);
        char ** setGgiEnv(HttpRequest *request);
        std::string  getCgiPath(HttpRequest *request) const;
};

#endif // CGI_HANDLER_HPP