#pragma once

#include "../ErrorHandler.hpp"

class InternalServerError : public ErrorHandler
{
    private:
    /* data */

    public:
        InternalServerError();
        bool    CanHandle(ERROR_TYPE ) const;
        void    ProcessError(Error &error); 
        const char *    what() const throw();   
        ~InternalServerError();
};