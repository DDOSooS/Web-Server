#pragma once

#include "./HttpRequest.hpp"

class HttpRequestBuilder
{
    private:
        HttpRequest            _http_request;

    public:
        HttpRequestBuilder();
        // void Reset();
        void SetRequestLine(std::string );
        void SetHttpVersion(std::string );
        void SetLocation(std::string );
        void addHeader(std::string &, std::string &);
        void ParseRequestLine(std::string & /* request Line*/);
        void ParseRequsetHeaders(std::istringstream & /* Headers*/);
        void ParseRequestBody(std::string & /* Body*/);
        void ParseRequest(std::string & rawRequest);

};