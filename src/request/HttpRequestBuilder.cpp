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

void HttpRequestBuilder::ParseRequestLine(std::string &request_line,const ServerConfig &serverConfig)
{
    std::cout << "[INFO] : PARSING REQ LINE !!!!!!!!!!!!!!\n";
    // decode the request line
    std::string         decoded_request_line = UrlDecode(request_line);
    std::istringstream  iss(decoded_request_line);
    std::string         method, path, http_version;

    iss >> method >> path >> http_version;
    TrimPath(path);
    
    // Debug the raw request line and parsed components
    std::cout << "Raw request line: '" << request_line << "'" << std::endl;
    std::cout << "Decoded request line: '" << decoded_request_line << "'" << std::endl;
    std::cout << "Parsed method: '" << method << "', path: '" << path << "', version: '" << http_version << "'" << std::endl;
    
    // check if the request line is a query string ?
    if (path.find("?") != std::string::npos)
    {
        size_t pos;

        pos  = path.find("?");
        std::string query_string = path.substr(pos + 1);
        path = path.substr(0, pos);
        this->_http_request.SetQueryStringStr(query_string);
        std::cout << "Extracted query string: '" << query_string << "'" << std::endl;
        std::cout << "Updated path: '" << path << "'" << std::endl;
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
        // exit(2);
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
    std::cout << "HTTP LOCATION TEST passed!!\n";
    _http_request.SetLocation(path);
    // check if the method is valid

    /*
    // check if the method is valid
        check for location -> default locatoin -> error page 404

        std::cout << "PATH (( " << path << "  ))\n";  
        if (cur_location)
            std::cout << "CURRENT LOCATION EXIST !!!!!!!!!!!!!---" << cur_location->get_path() << "==" << cur_location->get_allowMethods().size() << "===" << cur_location->get_root_location() << std::endl;
        else
            std::cout << "CURRENT LOCATION DOESN'T EXIST !!!!!!!!!!!!!\n";
    */
    // need to intergate the conf file congiguration!!!
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
    // check if the method is valid
    const Location *cur_location = serverConfig.findMatchingLocation(path);
    if (cur_location && !cur_location->is_method_allowed(method))
    {
        _http_request.SetIsRl(REQ_METHOD_ERROR);
        return;
    }
    std::cout << "Http METHOD TEST passed!!\n";

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
        // Check for end of headers (empty line or \r)
        if (line.empty() || line == "\r") break;
        size_t pos = line.find(":");
        if (pos == std::string::npos)
        {
            std::cerr << "Malformed Header: Missing ':'" << std::endl;
            throw HttpException(400, "Malformed Header: Missing ':'", BAD_REQUEST);
        }
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        _http_request.SetHeader(key, value);
    }
}

void HttpRequestBuilder::ParseRequestBody(std::string &body)
{
    //handling the body reading part including chunked transfer encoding and multipart form data
    _http_request.SetBody(body);
}


void HttpRequestBuilder::ParseRequest(std::string &rawRequest,const ServerConfig &serverConfig)
{
    std::cout << "Parsing the Request !!!!!!!!!\n";
    
    // Split the raw request into lines
    if (rawRequest.find("\r\n") == std::string::npos && rawRequest.find("/r/n/r/n") == std::string::npos)
    {
        std::cerr << "Invalid request format" << std::endl;
        throw HttpException(400, "Bad Request", BAD_REQUEST);
    }
    std::cout << "Crlf Test is BEING PASSED WELL!!!\n";
    
    // Find the separation between headers and body
    size_t bodyStart = rawRequest.find("\r\n\r\n");
    if (bodyStart == std::string::npos)
    {
        bodyStart = rawRequest.find("\n\n");
        if (bodyStart != std::string::npos)
        {
            bodyStart += 2;
        }
    }
    else
    {
        bodyStart += 4;
    }
    
    std::string headersPart;
    std::string bodyPart;
    
    if (bodyStart != std::string::npos)
    {
        headersPart = rawRequest.substr(0, bodyStart - 4);
        bodyPart = rawRequest.substr(bodyStart);
    }
    else
    {
        headersPart = rawRequest;
        bodyPart = "";
    }
    
    std::istringstream iss(headersPart);
    std::string line;

    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line, serverConfig);
    if (_http_request.GetIsRl() != REQ_DONE)
    {
        std::cerr << "Invalid request line" << std::endl;
        std::cerr << "Debug - Request status: " << _http_request.GetIsRl() << std::endl;
        std::cerr << "Method: '" << _http_request.GetMethod() << "', Location: '" << _http_request.GetLocation() << "'" << std::endl;
        
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
        {
            throw HttpException(404, "HTTP Version Not Supported", NOT_FOUND);
        }
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR)
        {
            std::cerr << "Method error: '" << _http_request.GetMethod() << "'" << std::endl;
            throw HttpException(400, "Bad Request - Invalid Method", BAD_REQUEST);
        }
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
        {
            std::cerr << "Location error: '" << _http_request.GetLocation() << "'" << std::endl;
            throw HttpException(404, "Not Found - Invalid Location", NOT_FOUND);
        }
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
        {
            throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
        }
        else
        {
            throw HttpException(400, "Bad Request - Unknown Error", BAD_REQUEST);
        }
        return;
    }
    
    // Parse headers
    ParseRequsetHeaders(iss);
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

    // FIXED: Handle body parsing correctly
    std::string contentLength = _http_request.GetHeader("Content-Length");
    if (!contentLength.empty())
    {
        size_t expectedLength;
        std::stringstream ss(contentLength);
        ss >> expectedLength;
        
        if (!bodyPart.empty())
        {
            if (bodyPart.size() >= expectedLength)
            {
                // We have the complete body in the initial read
                bodyPart = bodyPart.substr(0, expectedLength);
                ParseRequestBody(bodyPart);
                std::cout << "Complete body received: " << bodyPart.size() << " bytes" << std::endl;
            }
            else
            {
                // CRITICAL FIX: Don't set incomplete body!
                // Let ClientConnection handle reading the rest
                std::cout << "Body incomplete: got " << bodyPart.size() 
                         << " bytes, expected " << expectedLength 
                         << " - will read remaining data" << std::endl;
                // Don't call ParseRequestBody here - let ClientConnection handle it
            }
        }
        else if (expectedLength > 0)
        {
            std::cout << "No body in initial read, expecting " << expectedLength << " bytes" << std::endl;
            // Don't set body - let ClientConnection handle reading it
        }
    }
    else if (!bodyPart.empty())
    {
        // No Content-Length but we have body data - set it
        ParseRequestBody(bodyPart);
        std::cout << "Body set without Content-Length: " << bodyPart.size() << " bytes" << std::endl;
    }
    else
    {
        std::cout << "No body found in request" << std::endl;
    }
}

/* build the http request   */
HttpRequest& HttpRequestBuilder::GetHttpRequest()
{
    return _http_request;
}
