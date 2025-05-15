#include "../../include/request/error/NotFound.hpp"

NotFound::NotFound()
{
}

bool    NotFound::CanHandle(ERROR_TYPE type) const
{
    return ERROR_TYPE::NOT_FOUND == type;
}

void    NotFound::ProcessError(Error &error)
{
    std::stringstream iss;

    iss << "HTTP/1.1 404 Not Found\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>404 Not Found</title></head>";
    iss << "<body><h1>Not Found</h1>";
    iss << "<p>The requested resource could not be found on the server.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

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