#pragma once

#include "./ErrorHandler.hpp"

class InternalServerError : public ErrorHandler
{
    private:
    
    public:
        InternalServerError();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/); 
        const char *    what() const throw();   
        ~InternalServerError();
};