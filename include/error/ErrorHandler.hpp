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
        virtual void            HanldeError(Error &error, const ServerConfig & /* Server Configuration*/); ;
        virtual const char*     what() const throw();
        virtual bool            CanHandle(ERROR_TYPE ) const = 0;
        virtual void            ProcessError(Error &error, const ServerConfig & /* server Configuration*/) = 0;
        virtual void            DefaultErrorHandler(Error &error);
        bool                    IsErrorPageDefined(const ServerConfig &config, short error_code) const;
        void                    ErrorPageChecker(Error &error, const ServerConfig &config);
        virtual ~ErrorHandler();
};
