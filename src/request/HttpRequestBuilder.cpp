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

    std::cout << "============================================\n";
    std::cout << "============================================\n";
    std::cout << "----> method " << method << std::endl;
    std::cout << "----> path " << path << std::endl;
    std::cout << "----> http_version " << http_version << std::endl;
    std::cout << "----> request_line " << request_line << std::endl;
    std::cout << "============================================\n";
    std::cout << "============================================\n";

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
    // const Location *cur_location = serverConfig.findMatchingLocation(path);
    
    // if (cur_location && !cur_location->is_method_allowed(method))
    // {
    //     _http_request.SetIsRl(REQ_METHOD_ERROR);
    //     std::cout << "METHOD NOT ALLOWED: " << method << std::endl;
    //     std::cout << "METHOD NOT ALLOWED: " << method << std::endl;
    //     std::cout << "METHOD NOT ALLOWED: " << method << std::endl;
    //     std::cout << "METHOD NOT ALLOWED: " << method << std::endl;
    //     std::cout << "METHOD NOT ALLOWED: " << method << std::endl;
    //     exit(0);
    //     return;
    // }
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
        std::cout << "REQUEST HEADER LINE: [" << line  << std::endl;
        std::string key;
        std::string value;
        if (line.empty() || line == "\r") break;
        size_t pos = line.find(":");
        if (pos == std::string::npos)
        {
            std::cerr << "Malformed Header: Missing ':'" << std::endl;
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
        if (!value.empty())
        {
            std::cout << "Header Key: [" << key << "] (" << key.length() << "), Value: [" << value << "] (" << value.length() << ")" << std::endl;
        }

        _http_request.SetHeader(key, value);
    }
}

void HttpRequestBuilder::ParseRequestBody(std::string &body)
{
    //handling the body reading part including chunked transfer encoding and multipart form data
    _http_request.SetBodyAsStr(body);
}



void HttpRequestBuilder::ParseRequest(std::string &rawRequest,const ServerConfig &serverConfig, int socketFd)
{
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Parsing the Request !!!!!!!!!\n";
    std::cout << "Raw Request size:" << rawRequest.size() << std::endl;
    std::cout << "Raw Request Content: [" << rawRequest << "]" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    // exit(0);
    // if (rawRequest.find("\r\n") == std::string::npos && rawRequest.find("\n") == std::string::npos)
    // {
    //     std::cerr << "Invalid request format NO CRLF EXIST " << std::endl;
    //     throw HttpException(400, "Bad Request", BAD_REQUEST);
    // }
    std::cout << "Crlf Test is BEING PASSED WELL!!!\n";
    
    std::istringstream iss(rawRequest);
    std::string line;

    // Parse request line
    std::getline(iss, line);
    ParseRequestLine(line, serverConfig);

    //i should find more optimization for error handling here
    if (_http_request.GetIsRl() != REQ_DONE)
    {
        std::cerr << "Invalid request line" << std::endl;
        std::cerr << "Debug - Request status: " << _http_request.GetIsRl() << std::endl;
        std::cerr << "Method: '" << _http_request.GetMethod() << "', Location: '" << _http_request.GetLocation() << "'" << std::endl;
        
        if (_http_request.GetIsRl() == REQ_HTTP_VERSION_ERROR)
            throw HttpException(404, "HTTP Version Not Supported", NOT_FOUND);
        else if (_http_request.GetIsRl() == METHOD_NOT_ALLOWED || _http_request.GetIsRl() == REQ_METHOD_ERROR)
        {
            std::cerr << "Method error: '" << _http_request.GetMethod() << "'" << std::endl;
            throw HttpException(405, "Bad Request - Method Not Allowed", METHOD_NOT_ALLOWED);
        }
        else if (_http_request.GetIsRl() == REQ_LOCATION_ERROR)
        {
            std::cerr << "Location error: '" << _http_request.GetLocation() << "'" << std::endl;
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
            std::cerr << "Failed to read more headers from socket" << std::endl;
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
    // std::cout << "bytesRead: " << rawRequest.size() << std::endl;
    // exit(0);

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
    // std::cout << "Headers part size: " << headersPart.size() << std::endl;
    // std::cout << "Headers part content: [" << headersPart << "]" << std::endl;
    // std::cout << "-- END OF HEADERS --" << std::endl << std::endl;
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
    std::cout << "END OF HEADERS PARSING !!!!!!!!!!!!\n";

    /*
        Remplimeinting Post Method
        Body parsing
        - If Content-Length is present, read the body up to that length
        - if the transfer encoding is chunked, read chunks until the end
        - if it's multipart/form-data, parse the body accordingly
        - if no body is present, just set the body return response with 200 status code .
    */

    if (_http_request.GetMethod() == "POST")
    {
        if (_http_request.HasHeader("Content-Length"))
        {
            std::string content_length_str = _http_request.GetHeader("Content-Length");
            size_t content_length = std::stoul(content_length_str);
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
                std::cerr << "Failed to read body from socket" << std::endl;
                throw HttpException(500, "Internal Server Error - Failed to read body", INTERNAL_SERVER_ERROR);
            }
            if (bytesRead == 0)
            {
                _http_request.SetBodyAsStr("");
                std::cout << "No body received, setting empty body" << std::endl;
            }
            else
            {
                buffer[bytesRead] = '\0';
                _http_request.SetBodyAsStr(std::string(buffer));
            }
        }
        // debugging the request details
        // std::cout << "---- [REQUEST DETAILS] ----" << std::endl;
        // std::cout << "Method: " << _http_request.GetMethod() << std::endl ;
        // std::cout << "Location: " << _http_request.GetLocation() << std::endl;
        // std::cout << "Body Size : " << _http_request.GetBodySize() << std::endl;
        // std::cout << "Body Content: " << _http_request.GetBodyAsStr() << std::endl;
        // std::cout << "Headers: " << std::endl;
        // exit(0);
    }

    // Set the socket file descriptor
    std::cout << "===== Raw Request Parsing Completed Successfully! =====" << std::endl;
    std::cout << "rawRequest size: " << rawRequest.size() << std::endl;
    _http_request.SetSocketFd(socketFd);
    std::cout << "Request parsing completed successfully!" << std::endl << std::endl;
}

/* build the http request   */
HttpRequest& HttpRequestBuilder::GetHttpRequest()
{
    return _http_request;
}
