#pragma once

#include "ErrorHandler.hpp"

class TooManyRedirection : public ErrorHandler
{
    private:
        /* data */
    public:
        TooManyRedirection(/* args */);
        bool            CanHandle(ERROR_TYPE ) const;
        void            ProcessError(Error &error, const ServerConfig & /* server Configuration*/);
        const char *    what() const throw();
        ~TooManyRedirection();
};