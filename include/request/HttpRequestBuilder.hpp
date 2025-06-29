#pragma once

#include "./HttpRequest.hpp"
#include "./HttpException.hpp"
#include "../config/ServerConfig.hpp"
class HttpRequestBuilder
{
    private:
    HttpRequest            _http_request;

    public:
        HttpRequestBuilder();
        // void Reset();
        void            SetRequestLine(std::string );
        void            SetHttpVersion(std::string );
        void            SetLocation(std::string );
        void            SetBody(std::string );
        void            addHeader(std::string &, std::string &);
        void            ParseRequestLine(std::string & /* request Line*/,const ServerConfig & /* server config */);
        void            ParseRequsetHeaders(std::istringstream & /* Headers*/);
        void            ParseRequestBody(std::string & /* Body*/);
        void            ParseRequest(std::string & /* request Line*/,const ServerConfig & /* server config */);
        void            ParseQueryString(std::string & /* query string*/);
        std::string     UrlDecode(const std::string &);
        void            TrimPath(std::string &path);
        HttpRequest&    GetHttpRequest();
};