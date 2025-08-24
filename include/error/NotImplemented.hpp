#pragma once

#include "./ErrorHandler.hpp"

class NotImplemented : public ErrorHandler
{

    public:
        NotImplemented(/* args */);
        bool            CanHandle(ERROR_TYPE ) const;
        void            ProcessError(Error &error, const ServerConfig & /* server Configuration*/);
        const char *    what() const throw();
        ~NotImplemented();
};
