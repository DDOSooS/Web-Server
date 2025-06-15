#include "../../include/error/NotImplemented.hpp"


NotImplemented::NotImplemented()
{
}

bool    NotImplemented::CanHandle(ERROR_TYPE type) const
{
    if (type == NOT_IMPLEMENTED)
    {
        std::cout << "Not Implemented Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return NOT_IMPLEMENTED == type;
}

void    NotImplemented::ProcessError(Error &error, const ServerConfig & config)
{
    std::cout << "================= [Start of Processing Not Implemented Error] ====================\n";
    std::cout << "Not Implemented Error: " << error.GetErroeMessage() << std::endl;

    if (IsErrorPageDefined(config, error.GetCodeError()))
    {
        std::cout << "[INFO] [---ERRORS HANDLING --- Not Implemented Error Page is defined in the server configuration --- ]\n";
        ErrorPageChecker(error, config);
        return;
    }
    std::stringstream iss;

    iss << "<html><head><title>501 Not Implemented</title></head>";
    iss << "<body><h1>Not Implemented</h1>";
    iss << "<p>The server does not support the functionality required to fulfill the request.</p>";
    iss << "</body></html>";
    std::string response = iss.str();
    
    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "text/html", false, false);
    }
    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    error.GetClientData().http_response->setStatusMessage("Not Implemented");
    if (!error.GetClientData().http_response->getContentType().empty())
        error.GetClientData().http_response->setContentType("text/html");
}

const char *    NotImplemented::what() const throw()
{
    return "The server does not support the functionality\n";
}

NotImplemented::~NotImplemented()
{
}

