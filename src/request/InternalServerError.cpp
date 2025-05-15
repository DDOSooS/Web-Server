#include "../include/request/error/InternalServerError.hpp"

InternalServerError::InternalServerError()
{
}

bool    InternalServerError::CanHandle(ERROR_TYPE type) const
{
    return ERROR_TYPE::INTERNAL_SERVER_ERROR == type;
}

void    InternalServerError::ProcessError(Error &error)
{
    std::stringstream iss;

    iss << "HTTP/1.1 500 Internal Server Error\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>500 Internal Server Error</title></head>";
    iss << "<body><h1>Internal Server Error</h1>";
    iss << "<p>The server encountered an internal error and was unable to complete your request.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // error->GetClientData().http_response.send(response, response);
    std::cerr << "Internal Server Error: " << error.GetErroeMessage() << std::endl;
}

const char *    InternalServerError::what() const throw()
{
    return "Internal Server Error";
}

InternalServerError::~InternalServerError()
{
}