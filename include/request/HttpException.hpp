#pragma once

#include <stdexcept>
#include <string>
#include "../error/ErrorType.hpp"

// Forward declarations to avoid circular includes
class HttpRequest;

class HttpException : public std::runtime_error
{
    private:
        int             _code;
        std::string     _message;
        ERROR_TYPE      _error_type;
    public:
        HttpException(int code, std::string message, ERROR_TYPE error_type);
        int         GetCode() const;
        std::string GetMessage() const;
        ERROR_TYPE  GetErrorType() const;
        ~HttpException() throw();
};

// // Include these after the class declaration to avoid circular dependency
// #include "./HttpRequest.hpp"

