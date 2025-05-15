#include "../include/request/error/MethodNotAllowed.hpp"


MethodNotAllowed::MethodNotAllowed()
{
}

bool    MethodNotAllowed::CanHandle(ERROR_TYPE type) const
{
    return ERROR_TYPE::METHOD_NOT_ALLOWED == type;
}

void    MethodNotAllowed::ProcessError(Error &error)
{
    std::stringstream iss;

    iss << "HTTP/1.1 405 Method Not Allowed\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>405 Method Not Allowed</title></head>";
    iss << "<body><h1>Method Not Allowed</h1>";
    iss << "<p>The requested method is not allowed for the specified resource.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // error->GetClientData().http_response.send(response, response);
    std::cerr << "Method Not Allowed Error: " << error.GetErroeMessage() << std::endl;
}

const char *    MethodNotAllowed::what() const throw()
{
    return "Method Not Allowed Error";
}