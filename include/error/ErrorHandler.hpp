#pragma once


#include "./Error.hpp"
#include "../ClientConnection.hpp"

class ErrorHandler 
{
    private:
        ErrorHandler *nextHandler;

    public:
        ErrorHandler();
        ErrorHandler *          SetNext(ErrorHandler *);
        virtual void            HanldeError(Error &error) ;
        virtual const char*     what() const throw();
        virtual bool            CanHandle(ERROR_TYPE ) const = 0;
        virtual void            ProcessError(Error &error) = 0;
        virtual void            DefaultErrorHandler(Error &error);
        virtual ~ErrorHandler();
};
