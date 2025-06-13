#include "../../include/error/InternalServerError.hpp"

InternalServerError::InternalServerError()
{
}

bool    InternalServerError::CanHandle(ERROR_TYPE type) const
{
    if (type == INTERNAL_SERVER_ERROR)
    {
        std::cout << "Internal Server Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return INTERNAL_SERVER_ERROR == type;
}

void    InternalServerError::ProcessError(Error &error, const ServerConfig & config)
{
    std::cout << "================= [Start of Processing Internal Server Error] ====================\n";
    std::cout << "Internal Server Error: " << error.GetErroeMessage() << std::endl;
    if (IsErrorPageDefined(config, error.GetCodeError()))
    {
        std::cout << "[INFO] [---ERRORS HANDLING --- Internal Server Error Page is defined in the server configuration --- ]\n";    
        ErrorPageChecker(error, config);
        return;
    }

    std::stringstream iss;

    iss << "<html><head><title>500 Internal Server Error</title></head>";
    iss << "<body><h1>Internal Server Error</h1>";
    iss << "<p>The server encountered an internal error and was unable to complete your request.</p>";
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

const char *    InternalServerError::what() const throw()
{
    return "Internal Server Error";
}

InternalServerError::~InternalServerError()
{
}