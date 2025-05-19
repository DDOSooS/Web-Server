#pragma once

#include <string>
#include "../ClientConnection.hpp"


class ClientConnection;
enum class ERROR_TYPE
{  
    //4xx Client Errors
    BAD_REQUEST,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    CONTENT_TOO_LARGE,
    REQUEST_TIME_OUT,
    // UNAUTHORIZED,
    // FORBIDDEN,
    // LENGTH_REQUIREDM,

    //5xx  SERVER ERRORS
    INTERNAL_SERVER_ERROR,
    NOT_IMPLEMENTED
    // SERVICE_UNAVAILABLE,
    // GATEWAY_TIMEOUT,
    // BAD_GATEWAY

//   // 4xx Client Errors
//         BadRequest = 400,
//         Unauthorized = 401,
//         Forbidden = 403,
//         NotFound = 404,
//         MethodNotAllowed = 405,
//         RequestTimeout = 408,
//         Conflict = 409,

//         // 5xx Server Errors
//         InternalServerError = 500,
//         NotImplemented = 501,
//         BadGateway = 502,
//         ServiceUnavailable = 503,
//         GatewayTimeout = 504
} ;

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
