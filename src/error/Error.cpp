#include "../../include/error/Error.hpp"

Error::Error(ClientConnection& client, int code, const std::string& message, ERROR_TYPE type)
: _code_error(code), _message(message), _error_type(type), _client(client) {}
// Error::Error(ClientConnection& client, int code, const std::string& message, ERROR_TYPE errorType)
//     : _client(client), _code_error(code), _message(message), _error_type(errorType)
// {
// }
ClientConnection& Error::GetClientData() const
{
    return _client;
}

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

void Error::SetClientData(ClientConnection &client)
{
    this->_client =  client;
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

Error::~Error()
{
    
}