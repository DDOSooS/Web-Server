#include "../../include/error/NotFound.hpp"

NotFound::NotFound()
{
}

bool    NotFound::CanHandle(ERROR_TYPE type) const
{
    std::cout << "Can Handle Error Type: " << (int)(type) << std::endl;
    if (type == NOT_FOUND)
    {
        std::cout << "Not Found Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return NOT_FOUND == type;
}

void    NotFound::ProcessError(Error &error, const ServerConfig & /* server Configuration*/)
{
    std::cout << "================= (Start of Processing Not Found Error) ====================\n";
    std::cout << "Not Found Error: " << error.GetErroeMessage() << std::endl;
    std::stringstream iss;


    iss << "<html><head><title>404 Not Found</title></head>";
    iss << "<body><h1>Not Found</h1>";
    iss << "<p>The requested resource could not be found on the server.</p>";
    iss << "</body></html>";
    std::string response = iss.str();
    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "text/html", false, false);
    }
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    if (!error.GetClientData().http_response->getContentType().empty())
        error.GetClientData().http_response->setContentType("text/html");
    // error->GetClientData().http_response.send(response, response);
    std::cerr << "Not Found Error: " << error.GetErroeMessage() << std::endl;
}

const char *    NotFound::what() const throw()
{
    return "404 Not Found ERROR\n";
}

NotFound::~NotFound()
{
}