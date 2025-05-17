#include "../include/request/error/MethodNotAllowed.hpp"


MethodNotAllowed::MethodNotAllowed()
{
}

bool    MethodNotAllowed::CanHandle(ERROR_TYPE type) const
{
    if (type == ERROR_TYPE::METHOD_NOT_ALLOWED)
    {
        std::cout << "Method Not Allowed Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return ERROR_TYPE::METHOD_NOT_ALLOWED == type;
}

void    MethodNotAllowed::ProcessError(Error &error)
{
    std::cout << "================= (Start of Processing Method Not Allowed Error) ====================\n";
    std::cout << "Method Not Allowed Error: " << error.GetErroeMessage() << std::endl;
    std::stringstream iss;

    iss << "HTTP/1.1 405 Method Not Allowed\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>405 Method Not Allowed</title></head>";
    iss << "<body><h1>Method Not Allowed</h1>";
    iss << "<p>The requested method is not allowed for the specified resource.</p>";
    iss << "</body></html>";
    std::string response = iss.str();

    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), {}, "text/plain", false, false);
    // Set the response buffer
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
}

const char *    MethodNotAllowed::what() const throw()
{
    return "Method Not Allowed Error";
}