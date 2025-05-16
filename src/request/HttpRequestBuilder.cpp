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
    std::cout << "PARSING REQ LINE !!!!!!!!!!!!!!\n";
    // decode the request line
    std::string         decoded_request_line = UrlDecode(request_line);
    std::istringstream  iss(decoded_request_line);
    std::string         method, path, http_version;

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
    std::cout << "<< RL :: " << _http_request.GetRequestLine() << ";;   >>>\n"; 
    std::cout << "Methode :" << method << "\n";
    std::cout << "Location :" << path << "\n";
    std::cout << "Http Version:" << http_version << "\n";
   
    // check if the request line is valid
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0")
    {
        std::cerr << "HTTP VERSION ERROR\n";
        exit(2);
        _http_request.SetIsRl(REQ_HTTP_VERSION_ERROR);
        return;
    }
    _http_request.SetHttpVersion(http_version);
    // check if the method is valid
    std::cout << "Http TEST passed!!\n";
    // need to intergate the conf file congiguration!!!
    std::vector<std::string> valid_methods = {"GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS", "HEAD", "TRACE", "CONNECT"};
    if (std::find(valid_methods.begin(), valid_methods.end(), method) == valid_methods.end())
    {
        _http_request.SetIsRl(REQ_METHOD_ERROR);
        return;
    }
    std::cout << "METOHD SELECTED IS A VALID METHOD !!\n";
    // check if the method is valid
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE")
    {
        _http_request.SetIsRl(REQ_NOT_IMPLEMENTED);
        return;
    }
    std::cout << "Http METHOD TEST passed!!\n";

    _http_request.SetMethod(method);
    // check if the path is valid
    if (path.empty() || path[0] != '/')
    {
        _http_request.SetIsRl(REQ_LOCATION_ERROR);
        return;
    }
    std::cout << "HTTP LOCATION TEST passed!!\n";
    _http_request.SetLocation(path);
    _http_request.SetIsRl(REQ_DONE);
}

/*
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
*/

void HttpRequestBuilder::ParseRequsetHeaders(std::istringstream &iss)
{
    std::string line;

    while (std::getline(iss, line))
    {
        std::string key; 
        std::string value;
        // Check for end of headers (empty line or \r)
        if (line.empty() || line == "\r") break;
        size_t pos = line.find(":");
        if (pos == std::string::npos)
            throw HttpException(400, "Malformed Header: Missing ':'", ERROR_TYPE::BAD_REQUEST);
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        if (key.empty() || value.empty())
            throw HttpException(400, "Empty Header Key/Value", ERROR_TYPE::BAD_REQUEST);

        // Check for invalid characters in key
        // if (key.find_first_of("()<>@,;:\\/[]?={} \t") != std::string::npos) {
        //     throw HttpException(400, "Invalid Header Key: " + key, ERROR_TYPE::BAD_REQUEST);
        // }

        // if (_http_request.GetHeader(key).empty()){
        //     throw HttpException(400, "Duplicate Header: " + key, ERROR_TYPE::BAD_REQUEST);
        // }
        _http_request.SetHeader(key, value);
    }
}

void HttpRequestBuilder::ParseRequestBody(std::string &body)
{
    //handling the body reading part including chunked transfer encoding and multipart form data
    _http_request.SetBody(body);
}


void HttpRequestBuilder::ParseRequest(std::string &rawRequest)
{
    std::cout << "Parsing the Request !!!!!!!!!\n";
    std::cout << "Req Line::" << rawRequest << "==========|" << (rawRequest.find("\r\n\r\n")== std::string::npos) << "====" << (rawRequest.find("\r\n") == std::string::npos) << ":"; 
    // Split the raw request into lines
    if (rawRequest.find("\r\n") == std::string::npos && rawRequest.find("/r/n/r/n") == std::string::npos)
    {
        // Invalid request format exception or error handling
        std::cerr << "Invalid request format==================================????" << std::endl;
        exit(1);
        throw HttpException(404, "Bad Request", ERROR_TYPE::BAD_REQUEST);
    }
    std::cout << "Crlf Test is BEING PASSED WELL!!!\n";
    std::istringstream iss(rawRequest);
    std::string line;
    
    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line);
    if (_http_request.GetIsRl() != REQ_DONE)
    {
        //throw an exception or handle error
        std::cerr << "Invalid request line===========================================\n" << std::endl;
        exit(1);
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(505, "HTTP Version Not Supported", ERROR_TYPE::BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(500, "Bad Request", ERROR_TYPE::BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(404, "Not Found", ERROR_TYPE::NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", ERROR_TYPE::NOT_IMPLEMENTED);
        return;
    }
    // Parse headers
    ParseRequsetHeaders(iss);
    if (_http_request.GetIsRl() != REQ_PROCESSING)
    {
        std::cerr << "Invalid headers" << std::endl;
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(505, "HTTP Version Not Supported", ERROR_TYPE::BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(500, "Bad Request", ERROR_TYPE::BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(404, "Not Found", ERROR_TYPE::NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", ERROR_TYPE::NOT_IMPLEMENTED);
        
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