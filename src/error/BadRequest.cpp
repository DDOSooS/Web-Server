#include "../../include/error/BadRequest.hpp"

BadRequest::BadRequest()
{}

bool    BadRequest::CanHandle(ERROR_TYPE type) const
{
    if (type == BAD_REQUEST)
    {
        std::cout << "Bad Request Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return BAD_REQUEST == type;
}

BadRequest::~BadRequest() {}

void    BadRequest::ProcessError(Error &error)
{
    std::cout << "================= (Start of Processing Bad Request Error) ====================\n";
    std::cout << "Bad Request Error: " << error.GetErroeMessage() << std::endl;
    std::stringstream iss;

    iss << "HTTP/1.1 400 Bad Request\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>400 Bad Request</title></head>";
    iss << "<body><h1>Bad Request</h1>";
    iss << "<p>The server could not understand the request due to invalid syntax.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // Set the response buffer
    if (! error.GetClientData().http_response)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "text/plain", false, false);
    }

    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
}

const char *    BadRequest::what() const throw()
{
    return "400 Bad Request";
}