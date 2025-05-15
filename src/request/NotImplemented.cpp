#include "../../include/request/error/NotImplemented.hpp"


NotImplemented::NotImplemented()
{
}

bool    NotImplemented::CanHandle(ERROR_TYPE type) const
{
    return ERROR_TYPE::NOT_IMPLEMENTED == type;
}

void    NotImplemented::ProcessError(Error &error)
{
    std::stringstream iss;

    iss << "HTTP/1.1 501 Not Implemented\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>501 Not Implemented</title></head>";
    iss << "<body><h1>Not Implemented</h1>";
    iss << "<p>The server does not support the functionality required to fulfill the request.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // error->GetClientData().http_response.send(response, response);
    std::cerr << "Not Implemented Error: " << error.GetErroeMessage() << std::endl;
}

const char *    NotImplemented::what() const throw()
{
    return "The server does not support the functionality\n";
}

NotImplemented::~NotImplemented()
{
}

