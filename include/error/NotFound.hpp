#pragma once

#include "./ErrorHandler.hpp"

class NotFound : public ErrorHandler
{
    private:
    /* data */

    public:
        NotFound();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/);   
        const char *    what() const throw(); 
        ~NotFound();
};