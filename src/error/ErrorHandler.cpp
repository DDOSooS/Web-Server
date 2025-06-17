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

ErrorHandler *ErrorHandler::GetNext()
{
    return this->nextHandler;
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

bool ErrorHandler::IsErrorPageDefined(const ServerConfig &config, short error_code) const
{
    std::cout << " [DEBUG] : Checking if error page is defined for error code: " << error_code << std::endl;

    std::map<short, std::string> error_pages = config.get_error_pages();
    if (error_pages.find(error_code) != error_pages.end() && !error_pages[error_code].empty())
    {
        std::string root = config.get_root();
        if (!root.empty() && root[root.length() - 1] != '/')
            root += '/';
        std::string error_page_path = root + error_pages[error_code];
        std::cout << " [ INFO ] : Error page is defined for error code: " << error_code << " at " << error_page_path << std::endl;
        struct stat _statinfo;
        if (stat(error_page_path.c_str(), &_statinfo) != 0)
        {
            std::cerr << " [ ERROR ] : Error page file does not exist for error code: " << error_code << std::endl;
            return false;
        }
        return true;
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
        // exit(1);
    }
    // std::cout << " [ Debug] : Error page path : " << config.get_root() + error_pages[error.GetCodeError()] << " ] " << std::endl; 
    error.GetClientData().http_response->setFilePath((config.get_root()[config.get_root().length() -1] == '/' ? config.get_root() : config.get_root() + "/" ) + error_pages[error.GetCodeError()]);
    error.GetClientData().http_response->setStatusCode(error.GetCodeError());
    // std::cout << " [ Debug] : Error page file path : " << error.GetClientData().http_response->getFilePath() <<" ] " << std::endl; 
    return ;
}

void ErrorHandler::HanldeError(Error &error, const ServerConfig & config)
{
   
    std::cout << "Error Type ::::: " << error.GetCodeError() << "=================\n\n" ;
    try
    {   
        if (CanHandle(error.GetErrorType()))
           ProcessError(error, config);
        else if (this->nextHandler != NULL)
        {
            
            this->nextHandler->HanldeError(error, config);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return ;
}