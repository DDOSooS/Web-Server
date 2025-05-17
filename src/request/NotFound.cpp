#include "../../include/request/error/NotFound.hpp"

NotFound::NotFound()
{
}

bool    NotFound::CanHandle(ERROR_TYPE type) const
{
    std::cout << "Can Handle Error Type: " << static_cast<int>(type) << std::endl;
    if (type == ERROR_TYPE::NOT_FOUND)
    {
        std::cout << "Not Found Error Handler is being used!!!!!!!!!!!!!\n";
    }
    return ERROR_TYPE::NOT_FOUND == type;
}

void    NotFound::ProcessError(Error &error)
{
    std::cout << "================= (Start of Processing Not Found Error) ====================\n";
    std::cout << "Not Found Error: " << error.GetErroeMessage() << std::endl;
    std::stringstream iss;

    iss << "HTTP/1.1 404 Not Found\r\n";
    iss << "Content-Type: text/html\r\n\r\n";
    iss << "<html><head><title>404 Not Found</title></head>";
    iss << "<body><h1>Not Found</h1>";
    iss << "<p>The requested resource could not be found on the server.</p>";
    iss << "</body></html>";
    std::string response = iss.str();
    // Set the response buffer
    if (error.GetClientData().http_response == NULL)
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), {}, "text/plain", false, false);
    error.GetClientData().http_response->setBuffer(response);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
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