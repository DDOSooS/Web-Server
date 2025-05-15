#pragma once

#include <stdexcept>

#include "./Error.hpp"
enum class ERROR_TYPE;

class HttpException : public std::runtime_error
{
    private:
        int             _code;
        std::string     _message;
        ERROR_TYPE      _error_type;
    public:
        HttpException(int , std::string , ERROR_TYPE);
        int         GetCode() const;
        std::string GetMessage() const;
        ERROR_TYPE  GetErrorType() const;
        ~HttpException();
};

