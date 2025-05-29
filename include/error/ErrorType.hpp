#pragma once

// Enum defining all possible error types
enum ERROR_TYPE
{  
    //4xx Client Errors
    BAD_REQUEST,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    CONTENT_TOO_LARGE,
    REQUEST_TIME_OUT,
    FORBIDDEN,
    // UNAUTHORIZED,
    // FORBIDDEN,
    // LENGTH_REQUIREDM,

    //5xx  SERVER ERRORS
    INTERNAL_SERVER_ERROR,
    NOT_IMPLEMENTED
    // SERVICE_UNAVAILABLE,
    // GATEWAY_TIMEOUT,
    // BAD_GATEWAY
};