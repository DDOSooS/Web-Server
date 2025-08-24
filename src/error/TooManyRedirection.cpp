#include "../../include/error/TooManyRedirection.hpp"


TooManyRedirection::TooManyRedirection(/* args */)
{
    this->SetNext(NULL);
}

bool TooManyRedirection::CanHandle(ERROR_TYPE error_type) const
{
    return error_type == TOO_MANY_REDIRECTION;
}

void TooManyRedirection::ProcessError(Error &error, const ServerConfig & config)
{
    std::cout << "================= [Start of Processing Too Many Redirection Error] ====================\n";
    if (IsErrorPageDefined(config, error.GetCodeError()))
    {
        ErrorPageChecker(error, config);
        return;
    }
    std::stringstream iss;

    iss << "<html><head><title>Too Many Redirections</title></head>";
    iss << "<body><h1>Too Many Redirections</h1>";
    iss << "<p>The request has been redirected too many times.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "text/html", false, false);
    }
    if (!error.GetClientData().http_response->getContentType().empty())
        error.GetClientData().http_response->setContentType("text/html");
    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    error.GetClientData().http_response->setStatusMessage("Too Many Redirections");
    return ;
}

const char * TooManyRedirection::what() const throw()
{
    return "Too Many Redirections Error\n";
}

TooManyRedirection::~TooManyRedirection()
{
}