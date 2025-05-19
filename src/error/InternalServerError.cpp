#include "../../include/error/InternalServerError.hpp"

InternalServerError::InternalServerError()
{
}

bool    InternalServerError::CanHandle(ERROR_TYPE type) const
{
    if (type == ERROR_TYPE::INTERNAL_SERVER_ERROR)
    {
        std::cout << "Internal Server Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return ERROR_TYPE::INTERNAL_SERVER_ERROR == type;
}

void    InternalServerError::ProcessError(Error &error)
{
    std::cout << "================= (Start of Processing Internal Server Error) ====================\n";
    std::cout << "Internal Server Error: " << error.GetErroeMessage() << std::endl;

    std::stringstream iss;

    iss << "HTTP/1.1 500 Internal Server Error\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>500 Internal Server Error</title></head>";
    iss << "<body><h1>Internal Server Error</h1>";
    iss << "<p>The server encountered an internal error and was unable to complete your request.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), {}, "text/plain", false, false);
    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
}

const char *    InternalServerError::what() const throw()
{
    return "Internal Server Error";
}

InternalServerError::~InternalServerError()
{
}