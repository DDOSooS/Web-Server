#pragma once

#include "./ErrorHandler.hpp"

class Forbidden : public ErrorHandler
{
    private:
        /* data */
    public:
        Forbidden();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/); 
        const char *    what() const throw();   
        ~Forbidden();
};