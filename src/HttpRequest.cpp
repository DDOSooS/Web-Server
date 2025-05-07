#include "../include/request/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
    ResetRequest();
}

bool HttpRequest::FindHeader(std::string key, std::string value)
{
    std::unordered_map<std::string, std::string>::iterator it = _headers.find(key);
    if (it != _headers.end())
    {
        return it->second == value;
    }
    return false;
}

std::string HttpRequest::GetHeader(std::string key) const
{
    std::unordered_map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
    {
        return it->second;
    }
    return "";
}

std::unordered_map<std::string, std::string> HttpRequest::GetHeaders() const
{
    return _headers;
}

std::string HttpRequest::GetRequestLine() const
{
    return _request_line;
}


std::string HttpRequest::GetHttpVersion() const
{
    return _http_version;
}

std::string HttpRequest::GetBody() const
{
    return _body;
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
void HttpRequest::SetLocation(std::string location)
{
    _locatoin = location;
}

void HttpRequest::SetHeader(std::string key, std::string value)
{
    _headers[key] = value;
}

void HttpRequest::SetBody(std::string body)
{
    _body = body;
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

bool HttpRequest::GetIsRl() const
{
    return _is_rl;
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
    _locatoin = "";
    _buffer = "";
    _body = "";
    _status = PARSER;
    _is_crlf = false;
    _is_rl = PROCESSING;
    _are_header_parsed = false;
}

bool HttpRequest::IsValidRequest() const
{
    return _status == DONE;
}

HttpRequest::~HttpRequest()
{

}