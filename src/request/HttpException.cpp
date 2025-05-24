#include "../../include/request/HttpException.hpp"

HttpException::HttpException(int code, std::string message, ERROR_TYPE error_type):std::runtime_error(message), _code(code), _message(message), _error_type(error_type)
{
}

std::string HttpException::GetMessage() const
{
    return this->_message;
}

int HttpException::GetCode() const
{
    return  this->_code;
}

ERROR_TYPE HttpException::GetErrorType() const
{
    return this->_error_type;
}

HttpException::~HttpException() throw()
{
}