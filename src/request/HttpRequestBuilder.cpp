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

void HttpRequestBuilder::ParseRequestLine(std::string &request_line,const ServerConfig &serverConfig)
{
    std::cout << "[INFO] : PARSING REQ LINE !!!!!!!!!!!!!!\n";
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
        this->_http_request.SetQueryStringStr(query_string);
        // std::cout << "<><><><><><><><><><<><><><><><><><><> Query String : " << this->_http_request.GetQueryStringStr() << "\n";
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
        {
            std::cerr << "Malformed Header: Missing ':'" << std::endl;
            throw HttpException(400, "Malformed Header: Missing ':'", BAD_REQUEST);
        }
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        /*
            if (key.empty() || value.empty())
            {
                
            throw HttpException(400, "Empty Header Key/Value", ERROR_TYPE::BAD_REQUEST);
            }
            Check for invalid characters in key
            if (key.find_first_of("()<>@,;:\\/[]?={} \t") != std::string::npos) {
                throw HttpException(400, "Invalid Header Key: " + key, ERROR_TYPE::BAD_REQUEST);
            }
            
            if (_http_request.GetHeader(key).empty()){
                throw HttpException(400, "Duplicate Header: " + key, ERROR_TYPE::BAD_REQUEST);
            }
        */
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
    std::cout << "Req Line::" << rawRequest << "==========|" << (rawRequest.find("\r\n\r\n")== std::string::npos) << "====" << (rawRequest.find("\r\n") == std::string::npos) << ":";
    // Split the raw request into lines
    if (rawRequest.find("\r\n") == std::string::npos && rawRequest.find("/r/n/r/n") == std::string::npos)
    {
        // Invalid request format exception or error handling
        std::cerr << "Invalid request format==================================????" << std::endl;
        throw HttpException(400, "Bad Request", BAD_REQUEST);
    }
    std::cout << "Crlf Test is BEING PASSED WELL!!!\n";
    
    // Find the separation between headers and body
    size_t bodyStart = rawRequest.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        bodyStart = rawRequest.find("\n\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 2;
        }
    } else {
        bodyStart += 4;
    }
    
    std::string headersPart;
    std::string bodyPart;
    
    if (bodyStart != std::string::npos) {
        headersPart = rawRequest.substr(0, bodyStart - 4); // Remove the \r\n\r\n
        bodyPart = rawRequest.substr(bodyStart);
    } else {
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
        //throw an exception or handle error
        std::cerr << "Invalid request line===========================================\n" << std::endl;
        // exit(1);
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
        {
            // std::cout << "HTTP VERSION ERROR>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
            throw HttpException(404, "HTTP Version Not Supported", NOT_FOUND);
        }
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(500, "Bad Request", BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(404, "Not Found", NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
        return;
    }
    // Parse headers
    ParseRequsetHeaders(iss);
    if (_http_request.GetStatus() != PARSER)
    {
        std::cerr << "Invalid headers" << std::endl;
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(505, "HTTP Version Not Supported", BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_METHOD_ERROR || _http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(500, "Bad Request", BAD_REQUEST);
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
            throw HttpException(404, "Not Found", NOT_FOUND);
        else if (_http_request.GetIsRl() == REQ_NOT_IMPLEMENTED)
            throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
        return;
    }

    // Parse body if present - FIXED SECTION
    if (!bodyPart.empty()) {
        std::cout << "Content-Length header found: " << _http_request.GetHeader("Content-Length") << " bytes" << std::endl;
        std::cout << "Actual body size: " << bodyPart.size() << " bytes" << std::endl;
        std::cout << "Body content: " << bodyPart << std::endl;
        
        // Check if we have a Content-Length header
        std::string contentLength = _http_request.GetHeader("Content-Length");
        if (!contentLength.empty()) {
            size_t expectedLength;
            std::stringstream ss(contentLength);
            ss >> expectedLength;
            
            // Check if we have the complete body
            if (bodyPart.size() >= expectedLength) {
                // Take only the expected amount
                bodyPart = bodyPart.substr(0, expectedLength);
                std::cout << "Already read " << bodyPart.size() << " bytes of body" << std::endl;
            } else {
                std::cout << "Body incomplete: got " << bodyPart.size() << " bytes, expected " << expectedLength << std::endl;
            }
        }
        
        // Set the body
        ParseRequestBody(bodyPart);
        std::cout << "Body set in request object: " << _http_request.GetBody().size() << " bytes" << std::endl;
    } else {
        std::cout << "No body found in request" << std::endl;
    }
}


/* build the http request   */
HttpRequest& HttpRequestBuilder::GetHttpRequest()
{
    return _http_request;
}
