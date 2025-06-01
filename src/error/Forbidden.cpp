#include "../../include/error/Forbidden.hpp"

Forbidden::Forbidden()
{
    this->SetNext(NULL);
}

bool Forbidden::CanHandle(ERROR_TYPE error_type) const
{
    return error_type == FORBIDDEN;
}


void    Forbidden::ProcessError(Error &error, const ServerConfig & config)
{
    std::cout << "================= [Start of Processing Forbidden Error] ====================\n";
    if (IsErrorPageDefined(config, error.GetCodeError()))
    {
        std::cout << "[INFO] [---ERRORS HANDLING --- Forbidden Error Page is defined in the server configuration --- ]\n";
        ErrorPageChecker(error, config);
        return;
    }
    std::cout << "Forbidden Error: " << error.GetErroeMessage() << std::endl;
    std::stringstream iss;
    iss << "<html><head><title>403 Forbidden</title></head>";
    iss << "<body><h1>Forbidden</h1>";
    iss << "<p>You do not have permission to access this resource.</p>";
    iss << "</body></html>";
    std::string response = iss.str();
    // Set the response buffer
    if (!error.GetClientData().http_response)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "text/html", false, false);
    }
    
    if (!error.GetClientData().http_response->getContentType().empty())
        error.GetClientData().http_response->setContentType("text/html");
    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    error.GetClientData().http_response->setStatusMessage("Forbidden");

    std::cout << "================= (End of Processing Forbidden Error) ====================\n";
    return ;
}

const char *    Forbidden::what() const throw()
{
    return "403 Forbidden";
}

Forbidden::~Forbidden()
{
    // std::cout << "Forbidden Error Handler Destructor Called" << std::endl;
}