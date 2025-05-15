#include "../include/request/error/BadRequest.hpp"

BadRequest::BadRequest()
{}

bool    BadRequest::CanHandle(ERROR_TYPE type) const
{
    return ERROR_TYPE::BAD_REQUEST == type;
}

void    BadRequest::ProcessError(Error &error)
{
    std::stringstream iss;

    iss << "HTTP/1.1 400 Bad Request\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>400 Bad Request</title></head>";
    iss << "<body><h1>Bad Request</h1>";
    iss << "<p>The server could not understand the request due to invalid syntax.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // error->GetClientData().http_response.send(response, response);
    std::cerr << "Bad Request Error: " << error.GetErroeMessage() << std::endl;
}

const char *    BadRequest::what() const throw()
{
    return "400 Bad Request";
}