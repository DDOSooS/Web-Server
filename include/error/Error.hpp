#pragma once

#include <string>
#include "../ClientConnection.hpp"
#include "ErrorType.hpp"


class ClientConnection;

class Error
{
    private:
        int                 _code_error;
        std::string         _message;
        ERROR_TYPE          _error_type;
        ClientConnection&   _client;

    public:
        // Error( ClientConnection &, int, std::string &, ERROR_TYPE);
        Error(ClientConnection& client, int code, const std::string& message, ERROR_TYPE type);
        ~Error();
        std::string    GetErroeMessage() const;
        int            GetCodeError() const;
        ERROR_TYPE     GetErrorType() ;
        ClientConnection& GetClientData() const;

        void           SetCodeError(int );
        void           SetMessage(std::string &);
        void           SetErrorType(ERROR_TYPE);
        void           SetClientData(ClientConnection &);
};
