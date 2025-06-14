#include "../../include/request/HttpRequest.hpp"
#include "../../include/ClientConnection.hpp"

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
    _query_string_str = src._query_string_str;
    _is_redirected = src._is_redirected;
    _processed = src._processed;
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


// counting a file size
size_t  GetFileSize(std::string &file) 
{
    struct stat _stat_info;

    if (stat(file.c_str(), &_stat_info) != 0)
    {
        std::cerr << "[ ERROR ] : File does not exist: " << file << std::endl;
        return 0;
    }
    return _stat_info.st_size;
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
    this->_query_string_str = query_string_str;
}

void HttpRequest::SetLocation(std::string location)
{
    _location = location;
}

bool HttpRequest::IsRedirected() const
{
    return _is_redirected;
}

bool HttpRequest::IsProcessed() const
{
    return _processed;
}



void HttpRequest::SetIsRedirected(bool is_redirected)
{
    _is_redirected = is_redirected;
}

void HttpRequest::SetProcessed(bool processed)
{
    _processed = processed;
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
    _query_string.clear();
    _query_string_str = "";
    _headers.clear();
    _is_redirected = false;
    _processed = false;
}

bool HttpRequest::IsValidRequest() const
{
    return _status == DONE;
}

HttpRequest::~HttpRequest()
{

}

std::string    HttpRequest::GetRelativePath(const Location * cur_location)
{
    std::string rel_path;

    // in case of no location found, 
    if (!cur_location)
    {
        rel_path = this->GetClientDatat()->_server->getServerConfig().get_root() + this->GetLocation();
        if (rel_path[rel_path.length() - 1] != '/')
            rel_path += '/';
        std::cout << "[ WARNING ] : No matching location found, using server root: " << rel_path << std::endl;
        return rel_path;
    }

    // in case the location is just a default
    /*
    std::cout << "[ DEBUG ] : LOCATION INFOT\n";
    std::cout << "PATH : " << cur_location->get_path() << std::endl;
    std::cout << "ROOT LOCATION : " << cur_location->get_root_location() << std::endl;
    std::cout << "ALIAS : " << cur_location->get_alias() << std::endl;
    std::cout << "RETURN : " << cur_location->get_return().size() << std::endl;
    std::cout << "AUTO INDEX : " << (cur_location->get_autoindex() ? "ON" : "OFF") << std::endl;
    std::cout << "INDEX : " << cur_location->get_index() << std::endl;
    std::cout << "ALLOW METHODS : ";
    for (size_t i = 0; i < cur_location->get_allowMethods().size(); i++)
    {
        if (i > 0)
        std::cout << ", ";
        std::cout <<  cur_location->get_allowMethods()[i];
    }
    for (size_t i = 0; i < cur_location->get_return().size(); i++)
    {
        if (i > 0)
        std::cout << ", ";
        std::cout << "return Redirection  : " << cur_location->get_return()[i];
    }
    std::cout << std::endl;
    std::cout << "CLIENT MAX BODY SIZE : " << cur_location->get_clientMaxBodySize() << std::endl;
    std::cout << "[====================================================]\n";
    */
    if (!cur_location->get_return().empty())
    {        
        SetIsRedirected(true);
        /*
        std::cout << "REDIRECTED TO : " << cur_location->get_return()[0] << std::endl;
        std::cout << "REDIRECTED TO : " << cur_location->get_return()[1] << std::endl;
        std::cout << "REQUEST LOCATION : " << request->GetLocation()  << std::endl;
        */
        rel_path = cur_location->get_return()[1];
        return rel_path;
    }
    else if (!cur_location->get_alias().empty())
    {
        std::cout << "[ DEBUG ] : Using alias : " << cur_location->get_alias() << std::endl;
        rel_path = cur_location->get_alias() + GetLocation();
    }
    else if (!cur_location->get_root_location().empty())
    {
        std::cout << "[ DEBUG ] : Using root location : " << cur_location->get_root_location() << std::endl;
        rel_path = cur_location->get_root_location() + this->GetLocation();
    }
    else if (rel_path.empty())
    {
        std::cerr << "[ WARNING ] : No alias or root location specified, using server root." << std::endl;
        // If no alias or root location is specified, we use the Server's root path
        std::cout << "[ Server Root Path :" << this->GetClientDatat()->_server->getServerConfig().get_root() << " ]\n";
        rel_path = this->GetClientDatat()->_server->getServerConfig().get_root() + this->GetLocation();
    }
    if (rel_path[rel_path.length() - 1] != '/')
    {
        rel_path += '/';
    }
    else
    {
        std::cout << "[ DEBUG ] : Relative Path already ends with a slash : !!!!!!!!!!!" << rel_path << std::endl;
    }
    std::cout << "[------------ FInal rel_path :" << rel_path << " ----------------------]\n";
    return rel_path;
}

std::string  HttpRequest::GetRedirectionMessage(int status_code) const
{

    std::string message;

    switch (status_code)
    {
        case 301:
            message = "Moved Permanently";
            break;
        case 302:
            message = "Found";
            break;
        case 303:
            message = "See Other";
            break;
        case 307:
            message = "Temporary Redirect";
            break;
        case 308:
            message = "Permanent Redirect";
            break;
        default:
            message = "Redirection";
            break;
    }
    return message;
}

void HttpRequest::handleRedirect(const Location *cur_location,  std::string &rel_path)
{
    std::stringstream ss;
    int               status_code;

    ss << cur_location->get_return()[0];
    ss >> status_code;
    std::cout << "[REDIRECTED TO : " << rel_path  << " ]"<< std::endl;
    this->GetClientDatat()->http_response->setStatusCode(status_code);
    this->GetClientDatat()->http_response->setStatusMessage(GetRedirectionMessage(status_code));
    this->GetClientDatat()->http_response->setChunked(false);
    this->GetClientDatat()->http_response->setHeader("Location", rel_path);
    // in case of redirection doesn't contain a body, we set an empty buffer
    if (this->GetBody().empty())
        this->GetClientDatat()->http_response->setBuffer(" ");
    // else
        // @To do by ILyas check if the body size is too large, if so , you need to handle it for performance considerations !!!!!!!!!1
    return ;
}