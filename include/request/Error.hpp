#pragma once
#include <string>

enum class ERROR_TYPE
{  
    BAD_RQUEST,
    UNAUTHORIZED,
    FORBIDDEN,
    NOTFOUND,
    METHOD_NOT_ALLOWED,
    LENGTH_REQUIREDM,
    CONTENT_TOO_LARGE,
    INTERNAL_SERVER_ERROR,
    NOT_IMPLEMENTED,
    SERVICE_UNAVAILABLE,
    GATEWAY_TIMEOUT
} ;

class Error
{
    private:
        int            _code_error;
        std::string    _message;
        ERROR_TYPE     _error_type;

    public:
        Error(int , std::string &, ERROR_TYPE);
        ~Error();
        std::string    GetErroeMessage() const;
        int            GetCodeError() const;
        ERROR_TYPE     GetErrorType() ;

        void           SetCodeError(int );
        void           SetMessage(std::string &);
        void           SetErrorType(ERROR_TYPE);
};
