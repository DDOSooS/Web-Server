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
    _client = src._client;
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
    _client = NULL;
}

bool HttpRequest::IsValidRequest() const
{
    return _status == DONE;
}

HttpRequest::~HttpRequest()
{

}

std::string join_path(const std::string& a, const std::string& b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    std::string result = a;
    if (!result.empty() && result[result.size() - 1] == '/')
        result.erase(result.size() - 1);
    if (!b.empty() && b[0] == '/')
        return result + b;
    else
        return result + "/" + b;
}

std::string ensure_trailing_slash(const std::string& s)
{
    if (!s.empty() && s[s.size() - 1] != '/')
        return s + '/';
    return s;
}
std::string HttpRequest::GetRelativePath(const Location *cur_location, ClientConnection *client)
{
    std::string rel_path;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        std::cerr << "[ ERROR ] : Failed to get current working directory." << std::endl;
        cwd[0] = '\0';
    }
    if (!cur_location)
    {
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), this->GetLocation());
        rel_path = ensure_trailing_slash(rel_path);
        std::cout << "[ WARNING ] : No matching location found, using server root: " << rel_path << std::endl;
        std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
        return rel_path;
    }
    std::cout << "[ DEBUG ] : Current location path: RETUN------------------" << cur_location->get_return().empty() << std::endl;
    if (cur_location->get_return().empty())
    {
        std::cerr << "[ ERROR ] : Location return is empty." << std::endl;
        // return "";
    }
    else
    {
        std::cout << "[ DEBUG ] : Location return isn't empty "  << std::endl;
    }
    if (!cur_location->get_return().empty())
    {
        // std::cout << ""
        SetIsRedirected(true);
        std::cout << "\n\n\n-------------------------[ DEBUG ] : [ORIGIN ]Redirecting to : " 
                  << cur_location->get_path() << "------" << cur_location->get_return()[1] 
                  << "------------------\n\n" << std::endl;
        client->redirect_counter++;
        rel_path = cur_location->get_return()[1];
        std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
        return rel_path;
    }
    else if (!cur_location->get_alias().empty())
    {
        std::cout << "[ DEBUG ] : Using alias : " << cur_location->get_alias() << std::endl;
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), cur_location->get_alias());
    }
    else if (!cur_location->get_root_location().empty())
    {
        std::cout << "[ DEBUG ] : Using root location : " << cur_location->get_root_location() << std::endl;
        rel_path = join_path(join_path(join_path(cwd, client->getServerConfig().get_root()), cur_location->get_root_location()), this->GetLocation());
    }
    else if (rel_path.empty())
    {
        std::cerr << "[ WARNING ] : No alias or root location specified, using server root." << std::endl;
        std::cout << "[ Server Root Path :" << client->getServerConfig().get_root() << " ]\n";
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), this->GetLocation());
    }
    rel_path = ensure_trailing_slash(rel_path);

    std::cout << "[------------ FInal rel_path :" << rel_path << " ----------------------]\n";
    std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
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

void HttpRequest::handleRedirect(const Location *cur_location, std::string &rel_path)
{
    std::stringstream ss;
    int status_code;

    ss << cur_location->get_return()[0];
    ss >> status_code;
    std::cout << "[REDIRECTED TO : " << rel_path << " ]" << std::endl;
    
    this->GetClientDatat()->http_response->setStatusCode(status_code);
    this->GetClientDatat()->http_response->setStatusMessage(GetRedirectionMessage(status_code));
    this->GetClientDatat()->http_response->setChunked(false);
    this->GetClientDatat()->http_response->setHeader("Location", rel_path);
    
    if (status_code == 307 || status_code == 308)
    {
        std::cout << "[ INFO ] : Handling " << status_code << " redirect with method preservation" << std::endl;
        
        this->GetClientDatat()->http_response->setHeader("X-Original-Method", this->GetMethod());
        
        if (!this->GetBody().empty())
        {
            std::string content_type = this->GetHeader("Content-Type");
            size_t body_size = this->GetBody().size();
            
            // Performance consideration: Define thresholds based on server capacity
            const size_t MAX_BODY_SIZE_FOR_REDIRECT = 1024 * 1024;      // 1MB - moderate threshold
            const size_t CRITICAL_BODY_SIZE = 10 * 1024 * 1024;         // 10MB - critical threshold
            const size_t MAX_ALLOWED_BODY_SIZE = 50 * 1024 * 1024;      // 50MB - absolute limit
            
            if (!content_type.empty())
            {
                this->GetClientDatat()->http_response->setHeader("Content-Type", content_type);
                std::cout << "[ INFO ] : Setting Content-Type: " << content_type << std::endl;
            }
            
            std::stringstream ss_content_length;
            ss_content_length << body_size;
            this->GetClientDatat()->http_response->setHeader("Content-Length", ss_content_length.str());
            
            // Performance handling based on body size
            if (body_size > MAX_ALLOWED_BODY_SIZE)
            {
                // For extremely large bodies, reject the redirect to prevent server overload
                std::cerr << "[ ERROR ] : Body size (" << body_size << " bytes) exceeds maximum allowed size for redirect" << std::endl;
                
                this->GetClientDatat()->http_response->setStatusCode(413); // Payload Too Large
                this->GetClientDatat()->http_response->setStatusMessage("Payload Too Large");
                this->GetClientDatat()->http_response->setBuffer("Request body too large for redirect operation");
                
                // Log the rejection for monitoring
                std::cerr << "[ PERFORMANCE ] : Rejected redirect due to excessive body size: " 
                          << body_size << " bytes (limit: " << MAX_ALLOWED_BODY_SIZE << ")" << std::endl;
                return;
            }
            else if (body_size > CRITICAL_BODY_SIZE)
            {
                // For very large bodies, use streaming approach or chunked transfer
                std::cout << "[ WARNING ] : Critical body size (" << body_size << " bytes) in " 
                          << status_code << " redirection - using optimized handling" << std::endl;
                
                // Set chunked transfer for large bodies to avoid memory issues
                this->GetClientDatat()->http_response->setChunked(true);
                this->GetClientDatat()->http_response->setHeader("Transfer-Encoding", "chunked");
                
                // Don't include the full body in response to save memory
                // Instead, provide metadata about the body
                this->GetClientDatat()->http_response->setHeader("X-Large-Content", "true");
                this->GetClientDatat()->http_response->setHeader("X-Body-Size", ss_content_length.str());
                
                // Provide a minimal response body
                std::string minimal_body = "Large content redirect - body preserved for target location";
                this->GetClientDatat()->http_response->setBuffer(minimal_body);
                
                // Log performance impact
                std::cout << "[ PERFORMANCE ] : Using chunked transfer for large body redirect: " 
                          << body_size << " bytes" << std::endl;
            }
            else if (body_size > MAX_BODY_SIZE_FOR_REDIRECT)
            {
                // For moderately large bodies, include metadata but optimize memory usage
                std::cout << "[ WARNING ] : Large body size (" << body_size << " bytes) in " 
                          << status_code << " redirection - applying performance optimizations" << std::endl;
                
                this->GetClientDatat()->http_response->setHeader("X-Large-Content", "true");
                
                // For form data, provide additional metadata
                if (content_type.find("multipart/form-data") != std::string::npos || 
                    content_type.find("application/x-www-form-urlencoded") != std::string::npos)
                {
                    std::stringstream ss_form_size;
                    ss_form_size << body_size;
                    this->GetClientDatat()->http_response->setHeader("X-Form-Data-Size", ss_form_size.str());
                    
                    // Consider truncating form data preview for performance
                    const size_t PREVIEW_SIZE = 1024; // 1KB preview
                    if (body_size > PREVIEW_SIZE)
                    {
                        std::string preview = this->GetBody().substr(0, PREVIEW_SIZE) + "... [truncated]";
                        this->GetClientDatat()->http_response->setBuffer(preview);
                        this->GetClientDatat()->http_response->setHeader("X-Body-Truncated", "true");
                        std::cout << "[ PERFORMANCE ] : Body truncated for redirect response (preview: " 
                                  << PREVIEW_SIZE << " bytes)" << std::endl;
                    }
                    else
                    {
                        this->GetClientDatat()->http_response->setBuffer(this->GetBody());
                    }
                }
                else
                {
                    // For non-form data, be more conservative about including full body
                    this->GetClientDatat()->http_response->setBuffer(this->GetBody());
                }
                
                // Log performance metrics
                std::cout << "[ PERFORMANCE ] : Handling large body redirect: " 
                          << body_size << " bytes (threshold: " << MAX_BODY_SIZE_FOR_REDIRECT << ")" << std::endl;
            }
            else
            {
                // For small bodies, include normally
                this->GetClientDatat()->http_response->setBuffer(this->GetBody());
                std::cout << "[ INFO ] : Including original body in redirect response (" 
                          << body_size << " bytes)" << std::endl;
            }
            
            // Additional performance headers for client optimization
            if (body_size > MAX_BODY_SIZE_FOR_REDIRECT)
            {
                this->GetClientDatat()->http_response->setHeader("X-Redirect-Performance", "optimized");
                
                // Suggest client behavior for large redirects
                if (status_code == 307 || status_code == 308)
                {
                    this->GetClientDatat()->http_response->setHeader("X-Client-Hint", "consider-connection-reuse");
                }
            }
        }
        else
        {
            this->GetClientDatat()->http_response->setBuffer(" ");
            std::cout << "[ INFO ] : Empty body in redirect" << std::endl;
        }
    }
    else
    {
        // For non-method-preserving redirects (301, 302, 303), don't include body
        std::cout << "[ INFO ] : Standard redirect " << status_code 
                  << ", not preserving body (performance optimized)" << std::endl;
        this->GetClientDatat()->http_response->setBuffer(" ");
        
        // Still log if original request had large body for monitoring
        if (!this->GetBody().empty())
        {
            size_t original_body_size = this->GetBody().size();
            if (original_body_size > 1024 * 1024) // 1MB
            {
                std::cout << "[ PERFORMANCE ] : Discarded large body (" << original_body_size 
                          << " bytes) in standard redirect for performance" << std::endl;
            }
        }
    }
    
    return;
}