#include "../include/request/Error.hpp"

Error::Error(int code = 0, std::string &message, ERROR_TYPE error): _code_error(code), _message(message), _error_type(errno)
{}

int Error::GetCodeError() const
{
    return _code_error;
}

std::string Error::GetErroeMessage() const
{
    return _message;
}

ERROR_TYPE Error::GetErrorType()
{
    return _error_type;
}

void    Error::SetCodeError(int code)
{
    this->_code_error =  code;
}

void    Error::SetMessage(std::string & message)
{
    this->_message =  message;
}

void   Error::SetErrorType(ERROR_TYPE type)
{
    this->_error_type = type;
}