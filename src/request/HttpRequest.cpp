#include "../../include/request/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
    ResetRequest();
}

HttpRequest::HttpRequest(HttpRequest const &src)
{
    _request_line = src._request_line;
    _http_version = src._http_version;
    _method = src._method;
    _location = src._location;
    _buffer = src._buffer;
    _body = src._body;
    _headers = src._headers;
    _query_string = src._query_string;
    _status = src._status;
    _is_crlf = src._is_crlf;
    _is_rl = src._is_rl;
    _are_header_parsed = src._are_header_parsed;
}

bool HttpRequest::FindHeader(std::string key, std::string value)
{
    std::map<std::string, std::string>::iterator it = _headers.find(key);
    if (it != _headers.end())
    {
        return it->second == value;
    }
    return false;
}

std::string HttpRequest::GetHeader(std::string key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
    {
        return it->second;
    }
    return "";
}

std::map<std::string, std::string> HttpRequest::GetHeaders() const
{
    return _headers;
}

std::string HttpRequest::GetRequestLine() const
{
    return _request_line;
}

std::string HttpRequest::GetQueryStringStr() const
{
    return _query_string_str;
}

std::string HttpRequest::GetHttpVersion() const
{
    return _http_version;
}

std::string    HttpRequest::GetMethod() const
{
    return this->_method;
}

std::string HttpRequest::GetBody() const
{
    return _body;
}

std::string HttpRequest::GetLocation() const
{
    return _location;
}

void HttpRequest::SetMethod(std::string method)
{
    _method = method;
}

void HttpRequest::SetStatus(enum RequestStatus status)
{
    _status = status;
}

void HttpRequest::SetRequestLine(std::string request_line)
{
    _request_line = request_line;
}

void HttpRequest::SetHttpVersion(std::string http_version)
{
    _http_version = http_version;
}

void HttpRequest::SetQueryStringStr(std::string query_string_str)
{
    _query_string_str = query_string_str;
}

void HttpRequest::SetLocation(std::string location)
{
    _location = location;
}

ClientConnection *  HttpRequest::GetClientDatat() const
{
    return _client;
}

enum RequestStatus HttpRequest::GetStatus() const
{
    return _status;
}

void    HttpRequest::SetClientData(ClientConnection *client)
{
    this->_client =  client;
}

void HttpRequest::SetHeader(std::string key, std::string value)
{
    _headers[key] = value;
}

void HttpRequest::SetBody(std::string body)
{
    _body = body;
}

void HttpRequest::SetQueryString(std::vector<std::pair<std::string, std::string> > query)
{
    _query_string =  query;
}

void HttpRequest::SetBuffer(std::string buffer)
{
    _buffer = buffer;
}

void HttpRequest::SetIsCrlf(bool is_crlf)
{
    _is_crlf = is_crlf;
}

void HttpRequest::SetIsRl(RequestLineStatus is_rl)
{
    _is_rl = is_rl;
}

bool HttpRequest::GetIsCrlf() const
{
    return _is_crlf;
}

RequestLineStatus HttpRequest::GetIsRl() const
{
    return _is_rl;
}

std::vector<std::pair<std::string, std::string> > HttpRequest::GetQueryString() const
{
    return _query_string;
}

void HttpRequest::SetAreHeaderParsed(bool are_header_parsed)
{
    _are_header_parsed = are_header_parsed;
}

void HttpRequest::ResetRequest()
{
    _request_line = "";
    _http_version = "";
    _method = "";
    _location = "";
    _buffer = "";
    _body = "";
    _status = PARSER;
    _is_crlf = false;
    _is_rl = REQ_PROCESSING;
    _are_header_parsed = false;
}

bool HttpRequest::IsValidRequest() const
{
    return _status == DONE;
}

HttpRequest::~HttpRequest()
{

}