#include "../include/request/HttpRequestBuilder.hpp"

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
    _http_request.SetBody(body);
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

    while (std::getline(iss, query, '&'))
    {
        size_t pos =  query.find("=");
        if (pos != std::string::npos)
        {
            this->_http_request.GetQueryString().emplace_back(query.substr(0,pos), query.substr(pos + 1));
        }
        else
            this->_http_request.GetQueryString().emplace_back(query, "");
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

void HttpRequestBuilder::ParseRequestLine(std::string &request_line)
{

    // decode the request line
    std::string decoded_request_line = UrlDecode(request_line);
    std::istringstream iss(decoded_request_line);
    std::string method, path, http_version;

    iss >> method >> path >> http_version;
    // check if the request line is a query string ?
    if (path.find("?") != std::string::npos)
    {
        size_t pos;

        pos  = path.find("?");
        std::string query_string = path.substr(pos + 1);
        path = path.substr(0, pos);
        ParseQueryString(query_string);
    }

    _http_request.SetRequestLine(request_line);
    //check crlf of the request line
    if (_http_request.GetRequestLine().find("/r/n") == std::string::npos)
    {
        _http_request.SetIsRl(METHOD_ERROR);
        return ;
    }

    // check if the request line is valid
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
    {
        _http_request.SetIsRl(HTTP_VERSION_ERROR);
        return;
    }
    _http_request.SetHttpVersion(http_version);

    // check if the method is valid
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE")
    {
        _http_request.SetIsRl(METHOD_ERROR);
        return;
    }
    _http_request.SetMethod(method);

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
    if (rawRequest.find("\r\n") == std::string::npos || rawRequest.find("/r/n/r/n") == std::string::npos)
    {
        // Invalid request format exception or error handling
        //std::cerr << "Invalid request format" << std::endl;
        return ;
    }
    std::istringstream iss(rawRequest);
    std::string line;
    
    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line);
    if (_http_request.GetIsRl() != PROCESSING)
    {
        std::cerr << "Invalid request line" << std::endl;
        //throw an exception or handle error
        return;
    }
    // Parse headers
    ParseRequsetHeaders(iss);
    if (_http_request.GetIsRl() != PROCESSING)
    {
        std::cerr << "Invalid headers" << std::endl;
        //throw an exception or handle error
        return;
    }
    
    
    // Parse body if present
    // if (iss.peek() != EOF)
    // {
    //     std::string body;
    //     std::getline(iss, body);
    //     ParseRequestBody(body);
    // }
}
    

/* build the http request   */
HttpRequest& HttpRequestBuilder::GetHttpRequest()
{
    return _http_request;
}