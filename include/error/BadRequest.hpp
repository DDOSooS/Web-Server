#pragma once

#include "./ErrorHandler.hpp"


class BadRequest : public ErrorHandler
{
    private:
    /* data */

    public:
        BadRequest();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error, const ServerConfig & /* server Configuration*/); 
        const char *    what() const throw();   
        ~BadRequest();
};
