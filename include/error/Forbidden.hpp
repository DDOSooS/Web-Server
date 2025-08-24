#pragma once

#include "./ErrorHandler.hpp"

class Forbidden : public ErrorHandler
{

    public:
        Forbidden();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/); 
        const char *    what() const throw();   
        ~Forbidden();
};