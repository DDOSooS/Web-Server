#include "../include/request/ErrorHandler.hpp"

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

void ErrorHandler::HanldeError(Error &error)
{
   
    std::cout << "Error Type ::::: " << static_cast<int>( error.GetErrorType()) << "=================\n\n" ;
    exit(1);
    if (CanHandle(error.GetErrorType()))
        ProcessError(error);
    else if (this->nextHandler != NULL)
        this->nextHandler->HanldeError(error);
    else
        DefaultErrorHandler(error);
    /*
        decide even handle the error based on the error code padded 
        or pass it to the next error handler
    */
    return ;
}