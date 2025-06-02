#pragma once
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../config/Location.hpp"
#include "../config/ServerConfig.hpp"

class ClientConnection;
enum RequestStatus
{
    PARSER,
    PROCESSING,
    RESPONDING,
    DONE,
    ERROR
};

enum RequestLineStatus
{
    REQ_PROCESSING,
    REQ_HTTP_VERSION_ERROR,
    REQ_METHOD_ERROR,
    REQ_NOT_IMPLEMENTED,
    REQ_LOCATION_ERROR,
    REQ_DONE
};


class HttpRequest
{
    private:
        ClientConnection *                                  _client;
        std::string                                         _request_line;
        std::string                                         _http_version;
        std::string                                         _method;
        std::string                                         _location;
        std::string                                         _buffer;
        std::string                                         _body;
        std::map<std::string, std::string>                  _headers;
        std::vector<std::pair<std::string, std::string> >   _query_string;
        std::string                                         _query_string_str;
        enum RequestStatus                                  _status;
        bool                                                _is_crlf;
        RequestLineStatus                                   _is_rl; //request line
        bool                                                _are_header_parsed;
        bool                                                _is_redirected; // Flag to indicate if the request is redirected or not


    public:
        HttpRequest();
        HttpRequest(HttpRequest const &);
        bool                                            FindHeader(std::string, std::string);
        std::string                                     GetHeader(std::string )const;
        std::map<std::string, std::string>              GetHeaders()const;
        std::string                                     GetRequestLine() const;
        std::string                                     GetHttpVersion() const;
        std::string                                     GetBody() const;
        std::vector<std::pair<std::string, std::string> > GetQueryString() const;
        bool                                            GetIsCrlf() const;
        RequestLineStatus                               GetIsRl() const;
        std::string                                     GetMethod() const;
        std::string                                     GetLocation() const;
        ClientConnection *                              GetClientDatat() const;
        enum RequestStatus                              GetStatus() const;
        std::string                                     GetQueryStringStr() const;
        bool                                            IsRedirected() const;
        std::string                                     GetRelativePath(const Location * cur_location) const ;

        void                                            SetIsRedirected(bool);
        void                                            SetQueryStringStr(std::string);
        void                                            SetClientData(ClientConnection *);
        void                                            SetMethod(std::string);
        void                                            SetRequestLine(std::string);
        void                                            SetHttpVersion(std::string);
        void                                            SetLocation(std::string);
        void                                            SetHeader(std::string, std::string);
        void                                            SetBody(std::string);
        void                                            SetBuffer(std::string);
        void                                            SetIsCrlf(bool);
        void                                            SetIsRl(RequestLineStatus);
        void                                            SetAreHeaderParsed(bool);
        void                                            SetStatus(enum RequestStatus );
        void                                            SetQueryString(std::vector<std::pair<std::string, std::string> > );
        void                                            ResetRequest();
        std::string                                     GetRelativePath(const Location * cur_location);
        bool                                            IsValidRequest() const;
        void                                            handleRedirect(const Location * cur_location , std::string &rel_path);
        std::string                                     GetRedirectionMessage(int status_code) const;
        ~HttpRequest();
};