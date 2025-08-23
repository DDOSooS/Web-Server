#include <string>
#include "../../include/request/HttpRequestBuilder.hpp"
#include "../../include/config/Location.hpp"

HttpRequestBuilder::HttpRequestBuilder()
{
    _http_request.ResetRequest();
}

/* setter of the builder objec*/
void HttpRequestBuilder::SetRequestLine(std::string request_line)
{
    _http_request.SetRequestLine(request_line);
}

void HttpRequestBuilder::SetHttpVersion(std::string http_version)
{
    _http_request.SetHttpVersion(http_version);
}

void HttpRequestBuilder::SetLocation(std::string location)
{
    _http_request.SetLocation(location);
}

void HttpRequestBuilder::SetBody(std::string body)
{
    _http_request.SetBodyAsStr(body);
}

void HttpRequestBuilder::addHeader(std::string &key, std::string &value)
{
    _http_request.SetHeader(key, value);
}


void HttpRequestBuilder::ParseQueryString(std::string &query_string)
{
    std::istringstream                      iss(query_string);
    std::pair<std::string, std::string>     pair;
    std::string                             query;

    // this->_http_request.SetQueryStringStr(query_string);
    while (std::getline(iss, query, '&'))
    {
        size_t pos =  query.find("=");
        if (pos != std::string::npos)
        {
            this->_http_request.GetQueryString().push_back(std::make_pair(query.substr(0,pos), query.substr(pos + 1)));
        }
        else
            this->_http_request.GetQueryString().push_back(std::make_pair(query, ""));
    }
}

std::string HttpRequestBuilder::UrlDecode(const std::string &req_line)
{
    std::string decoded;

    for (size_t i = 0; i < req_line.size(); i++)
    {
        if (req_line[i] == '%' && i + 2 < req_line.size())
        {
            int hexChar;
            if (sscanf(req_line.substr(i+1,3).c_str(), "%x", &hexChar) == 1)
            {
                decoded += static_cast<char>(hexChar);
                i += 2;
            }
            else
                decoded += req_line[i];
        }
        else if (req_line[i] == '+')
            decoded += ' ';
        else
            decoded += req_line[i];
    }
    return decoded;
}

void    HttpRequestBuilder::TrimPath(std::string &path)
{
    for (size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] == '/' && i < path.size() - 1 && path[i + 1] == '/')
        {
            path.erase(i, 1);
            --i;
        }
    }
    if (path.empty() || path[0] != '/')
    {
        path = "/" + path;
        return;
    }
}

std::string trim(const std::string& str) 
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

void SetRequestLineFields(std::string &rl, std::string &method, std::string &path, std::string &http_version)
{
    size_t first_wp = rl.find_first_of(" \t");
    method = rl.substr(0, first_wp);
    size_t last_wp = rl.find_last_of(" \t");
    http_version = rl.substr(last_wp + 1);
    path = rl.substr(first_wp + 1, last_wp - first_wp - 1);
    
    method = trim(method);
    path = trim(path);
    http_version = trim(http_version);
}

void HttpRequestBuilder::ParseRequestLine(std::string &request_line,const ServerConfig &serverConfig)
{
    (void) serverConfig;
    std::string         decoded_request_line = UrlDecode(request_line);
    std::istringstream  iss(decoded_request_line);
    std::string         method, path, http_version;

    SetRequestLineFields(decoded_request_line, method, path, http_version);
    if (path.find("?") != std::string::npos)
    {
        size_t pos;

        pos  = path.find("?");
        std::string query_string = path.substr(pos + 1);
        path = path.substr(0, pos);
        this->_http_request.SetQueryStringStr(query_string);
        ParseQueryString(query_string);
    }
    _http_request.SetRequestLine(decoded_request_line);

    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
    {
        _http_request.SetIsRl(REQ_HTTP_VERSION_ERROR);
        return;
    }
    _http_request.SetHttpVersion(http_version);
    // check if the path is valid
    if (path.empty() || path[0] != '/')
    {
        _http_request.SetIsRl(REQ_LOCATION_ERROR);
        return;
    }
    _http_request.SetLocation(path);

    std::vector<std::string> valid_methods;
    valid_methods.push_back("GET");
    valid_methods.push_back("POST");
    valid_methods.push_back("PUT");
    valid_methods.push_back("DELETE");
    valid_methods.push_back("PATCH");
    valid_methods.push_back("OPTIONS");
    valid_methods.push_back("HEAD");
    valid_methods.push_back("TRACE");
    valid_methods.push_back("CONNECT");
    if (std::find(valid_methods.begin(), valid_methods.end(), method) == valid_methods.end())
    {
        _http_request.SetIsRl(REQ_METHOD_ERROR);
        return;
    }
    _http_request.SetMethod(method);
    _http_request.SetIsRl(REQ_DONE);
    _http_request.SetStatus(PARSER);
}



void HttpRequestBuilder::ParseRequsetHeaders(std::istringstream &iss)
{
    std::string line;

    while (std::getline(iss, line))
    {
        std::string key;
        std::string value;
        if (line.empty() || line == "\r") break;
        size_t pos = line.find(":");
        if (pos == std::string::npos)
        {
            throw HttpException(400, "Malformed Header: Missing ':'", BAD_REQUEST);
        }
        key = line.substr(0, pos);
        // Trim leading whitespace from value
        value = line.substr(pos + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        // // Trim trailing whitespace (safe)
        size_t endpos = value.find_last_not_of(" \t\r\n");
        if (endpos != std::string::npos)
            value.erase(endpos + 1);
        else
            value.clear();
        _http_request.SetHeader(key, value);
    }
}

void HttpRequestBuilder::ParseRequestBody(std::string &body)
{
    _http_request.SetBodyAsStr(body);
}

void HttpRequestBuilder::ParseRequest(std::string &rawRequest,const ServerConfig &serverConfig, int socketFd)
{

    std::istringstream iss(rawRequest);
    std::string line;

    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line, serverConfig);

    //i should find more optimization for error handling here
    if (_http_request.GetIsRl() != REQ_DONE)
    {
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(404, "HTTP Version Not Supported", NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_METHOD_ERROR)
        {
            throw HttpException(405, "Bad Request - Method Not Allowed", METHOD_NOT_ALLOWED);
        }
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
        {
            throw HttpException(404, "Not Found - Invalid Location", NOT_FOUND);
        }
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
        else
            throw HttpException(400, "Bad Request - Unknown Error", BAD_REQUEST);
        return;
    }
    
    // in case if we didn't find the end of the request's headers
    // we should read until we reach the end of the headers
    size_t header_start;
    size_t first_line_end;
    
    first_line_end = rawRequest.find("\r\n");
    if (first_line_end != std::string::npos)
        header_start = first_line_end + 2;
    else
    {
        first_line_end = rawRequest.find("\n");
        header_start = first_line_end + 1;
    }
    while(rawRequest.find("\r\n\r\n") == std::string::npos && rawRequest.find("\n\n") == std::string::npos)
    {
        char buffer[REQUSET_BUFFER];
        ssize_t bytesRead = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0)
        {
            throw HttpException(400, "Bad Request - Failed to read headers", BAD_REQUEST);
        }
        if (bytesRead == 0)
        {
            std::cerr << "No more data to read from socket" << std::endl;
            break; 
        }
        buffer[bytesRead] = '\0';
        rawRequest += std::string(buffer);
    }

    size_t header_end;
    size_t body_start;
    if (rawRequest.find("\r\n\r\n") != std::string::npos)
    {
        header_end = rawRequest.find("\r\n\r\n");
        body_start = header_end + 4;
    }
    else
    {
        header_end = rawRequest.find("\n\n");
        body_start = header_end + 2;
    }
    std::string headersPart = rawRequest.substr(header_start , header_end - header_start);

    std::istringstream is(headersPart);

    // Parse headers
    ParseRequsetHeaders(is);
    //i should find more optimization for error handling here
    if (_http_request.GetStatus() != PARSER)
    {
        std::cerr << "Invalid headers" << std::endl;
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(400, "HTTP Version Not Supported", BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(400, "Bad Request", BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(404, "Not Found", NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
        return;
    }

    if (_http_request.GetMethod() == "POST")
    {
        if (_http_request.HasHeader("Content-Length"))
        {
            std::string content_length_str = _http_request.GetHeader("Content-Length");
            size_t content_length = atol(content_length_str.c_str());
            _http_request.SetBodySize(content_length);
            _http_request.SetRemaineBytes(content_length);
            _http_request.SetRecvBytes(0);
        }
        else if (!_http_request.HasHeader("Transfer-Encoding") )
            throw HttpException(400 , "Bad Request - Content-Length or Transfer-Encoding header missing", BAD_REQUEST);
        if (body_start < rawRequest.size())
        {
            _http_request.SetBodyAsStr(rawRequest.substr(body_start));
        }
        else if (body_start >= rawRequest.size())
        {
            char buffer[REQUSET_BUFFER];
            ssize_t bytesRead = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead < 0)
            {
                throw HttpException(500, "Internal Server Error - Failed to read body", INTERNAL_SERVER_ERROR);
            }
            if (bytesRead == 0)
            {
                _http_request.SetBodyAsStr("");
            }
            else
            {
                buffer[bytesRead] = '\0';
                _http_request.SetBodyAsStr(std::string(buffer));
            }
        }
    }
    // Settng the socket file descriptor
    _http_request.SetSocketFd(socketFd);
}

/* build the http request   */
HttpRequest& HttpRequestBuilder::GetHttpRequest()
{
    return _http_request;
}
