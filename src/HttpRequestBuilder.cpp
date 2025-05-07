#include "../include/request/HttpRequestBuilder.hpp"

HttpRequestBuilder::HttpRequestBuilder()
{
    _http_request.ResetRequest();
}

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

void HttpRequestBuilder::addHeader(std::string &key, std::string &value)
{
    _http_request.SetHeader(key, value);
}

void HttpRequestBuilder::ParseRequestLine(std::string &request_line)
{
    std::istringstream iss(request_line);
    std::string method, path, http_version;
    
    iss >> method >> path >> http_version;
    
    _http_request.SetRequestLine(request_line);
    
    //check crlf of the request line
    if (_http_request.GetRequestLine().find("/r/n") == std::string::npos)
    {
        _http_request.SetIsRl(METHOD_ERROR);
        return ;
    }
    // check if the request line is valid
    _http_request.SetHttpVersion(http_version);
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
    {
        _http_request.SetIsRl(HTTP_VERSION_ERROR);
        return;
    }

    // check if the method is valid
    _http_request.SetMethod(method);
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE")
    {
        _http_request.SetIsRl(METHOD_ERROR);
        return;
    }
    // check if the path is valid
    if (path.empty() || path[0] != '/')
    {
        _http_request.SetIsRl(LOCATION_ERROR);
        return;
    }
    _http_request.SetLocation(path);

    _http_request.SetIsRl(DONE);
}

void HttpRequestBuilder::ParseRequsetHeaders(std::istringstream &iss)
{
    std::string line;


    std::getline(iss, line);
    while (std::getline(iss, line) && line != "\r\n")
    {
        std::string key, value;
        size_t pos = line.find(":");
        if (pos != std::string::npos)
        {
            key = line.substr(0, pos);
            value = line.substr(pos + 1);
            // Remove leading whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            _http_request.SetHeader(key, value);
        }
    }
}

void HttpRequestBuilder::ParseRequestBody(std::string &body)
{
    //handling the body reading part including chunked transfer encoding and multipart form data
    _http_request.SetBody(body);
}


void HttpRequestBuilder::ParseRequest(std::string &rawRequest)
{
    // Split the raw request into lines
    std::istringstream iss(rawRequest);
    std::string line;
    
    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line);
    
    // Parse headers
    ParseRequsetHeaders(iss);
    
    
    // Parse body if present
    // if (iss.peek() != EOF)
    // {
    //     std::string body;
    //     std::getline(iss, body);
    //     ParseRequestBody(body);
    // }
}
    