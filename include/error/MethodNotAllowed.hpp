#pragma once

#include "./ErrorHandler.hpp"

class MethodNotAllowed : public ErrorHandler
{
    private:
    /* data */

    public:
        MethodNotAllowed();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/);  
        const char *    what() const throw();  
        ~MethodNotAllowed();
};