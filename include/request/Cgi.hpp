#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <vector>

// Forward declarations to avoid circular dependencies
class RequestHandler;
class HttpRequest;
class CgiHandler;

class Cgi : public RequestHandler {
private:
    CgiHandler* cgi_handler;
    
    // Helper methods
    bool isCgiPath(const std::string& path) const;
    std::string getScriptPath(const std::string& request_path) const;
    std::string buildQueryString(HttpRequest* request) const;
    void handleCgiError(HttpRequest* request, const std::string& error_msg);

public:
    explicit Cgi(CgiHandler* handler);
    virtual ~Cgi();
    
    // Override RequestHandler methods
    virtual bool CanHandle(std::string method);
    virtual void ProccessRequest(HttpRequest* request);
};

#endif // CGI_HPP