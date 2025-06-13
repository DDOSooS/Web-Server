#include "../../include/error/MethodNotAllowed.hpp"


MethodNotAllowed::MethodNotAllowed()
{
}

bool    MethodNotAllowed::CanHandle(ERROR_TYPE type) const
{
    if (type == METHOD_NOT_ALLOWED)
    {
        std::cout << "Method Not Allowed Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return METHOD_NOT_ALLOWED == type;
}

MethodNotAllowed::~MethodNotAllowed() {}

void    MethodNotAllowed::ProcessError(Error &error, const ServerConfig & config)
{
    std::cout << "================= [Start of Processing Method Not Allowed Error] ====================\n";
    std::cout << "Method Not Allowed Error: " << error.GetErroeMessage() << std::endl;

    if (IsErrorPageDefined(config, error.GetCodeError()))
    {
        std::cout << "[INFO] [---ERRORS HANDLING --- Method Not Allowed Error Page is defined in the server configuration --- ]\n";
        ErrorPageChecker(error, config);
        return;
    }
    std::stringstream iss;

    iss << "<html><head><title>405 Method Not Allowed</title></head>";
    iss << "<body><h1>Method Not Allowed</h1>";
    iss << "<p>The requested method is not allowed for the specified resource.</p>";
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
    if (!error.GetClientData().http_response->getContentType().empty())
        error.GetClientData().http_response->setContentType("text/html");
}

const char *    MethodNotAllowed::what() const throw()
{
    return "Method Not Allowed Error";
}