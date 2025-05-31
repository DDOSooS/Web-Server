#include "../../include/error/ErrorHandler.hpp"

ErrorHandler::ErrorHandler():nextHandler(NULL)
{

}

ErrorHandler::~ErrorHandler()
{

}

const char *    ErrorHandler::what() const throw()
{
    return "Error Handler Error\n";
}

ErrorHandler *ErrorHandler::SetNext(ErrorHandler *handler)
{
    if (handler)
    {
        this->nextHandler = handler;
        return handler;
    }
    /*
        hand the csr when the error is being null
    */
   return this;
}

void	ErrorHandler::DefaultErrorHandler(Error &error)
{

}

bool ErrorHandler::IsErrorPageDefined(const ServerConfig &config, short error_code) const
{
    std::cout << " [DEBUG] : Checking if error page is defined for error code: " << error_code << std::endl;


    std::map<short, std::string> error_pages = config.get_error_pages();
    if (error_pages.find(error_code) != error_pages.end())
    {
        if (!error_pages[error_code].empty())
        {
            std::cout << " [ INFO ] : Error page is defined for error code: " << error_code << std::endl;
            return true;
        }
        return false;
    }
    return false;
}

void ErrorHandler::ErrorPageChecker(Error &error, const ServerConfig &config)
{

    std::cout << " [ INFO ] : Error Page is defined for error code: " << error.GetCodeError() << std::endl;
    if (error.GetClientData().http_response == NULL)
    {
        std::map<std::string, std::string> emptyHeaders;
        error.GetClientData().http_response = new HttpResponse(error.GetCodeError(), emptyHeaders, "", false, false);
    }
    // Set the response buffer
    std::map<short, std::string> error_pages = config.get_error_pages();
    if (error_pages.find(error.GetCodeError()) == error_pages.end())
    {
        std::cerr << " [ ERROR ] : Error page not defined for error code: " << error.GetCodeError() << std::endl;
        exit(1);
    }
    else
    {
        std::cout << " [ INFO ] : Error page is defined for error code: " << error.GetCodeError() << std::endl;
    }
    std::cout << " [ Debug] : Error page path : " << config.get_root() + error_pages[error.GetCodeError()] << " ] " << std::endl; 
    error.GetClientData().http_response->setFilePath(config.get_root() + error_pages[error.GetCodeError()]);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    return ;

}

void ErrorHandler::HanldeError(Error &error, const ServerConfig & config)
{
   
    std::cout << "Error Type ::::: " << error.GetCodeError() << "=================\n\n" ;
    // exit(1);
    if (CanHandle(error.GetErrorType()))
        ProcessError(error, config);
    else if (this->nextHandler != NULL)
    {

        this->nextHandler->HanldeError(error, config);
    }
    else
        DefaultErrorHandler(error);
    /*
        decide even handle the error based on the error code padded 
        or pass it to the next error handler
    */
    return ;
}