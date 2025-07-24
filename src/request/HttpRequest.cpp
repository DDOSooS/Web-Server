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
    _remaine_bytes = src._remaine_bytes;
    _recv_bytes = src._recv_bytes;
    _body_start = src._body_start;
    _body_size = src._body_size;
    _socket_fd = src._socket_fd;
    _upload_file_path = src._upload_file_path;
    _is_streaming_upload = src._is_streaming_upload;
    _upload_file_type = src._upload_file_type;
    _boundry = src._boundry;
    _uploading_status = src._uploading_status;
    // _upload_file_stream is not copyable; do not copy it
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

bool HttpRequest::HasHeader(std::string key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    return it != _headers.end();
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

std::string HttpRequest::GetBodyAsStr() const
{
    return _body;
}

std::vector<char> HttpRequest::GetBodyAsBin() const
{
    return _body_vec;
}

int HttpRequest::GetSocketFd() const
{
    return _socket_fd;
}
std::string HttpRequest::GetLocation() const
{
    return _location;
}

void HttpRequest::SetMethod(std::string method)
{
    _method = method;
}

void HttpRequest::SetBoundary(std::string boundry)
{
    _boundry = boundry;
}

void HttpRequest::SetUploadFilePath(std::string upload_file_path)
{
    _upload_file_path = upload_file_path;
}

void HttpRequest::SetStatus(enum RequestStatus status)
{
    _status = status;
}

size_t HttpRequest::GetRemaineBytes() const
{
    return _remaine_bytes;
}

void HttpRequest::SetRemaineBytes(size_t remaine_bytes)
{
    _remaine_bytes = remaine_bytes;
}

size_t HttpRequest::GetRecvBytes() const
{
    return _recv_bytes;
}

void HttpRequest::SetRecvBytes(size_t recv_bytes)
{
    _recv_bytes += recv_bytes;
}


std::string HttpRequest::GetBoundary() const
{
    return _boundry;
}

size_t HttpRequest::GetBodyStart() const
{
    return _body_start;
}

void HttpRequest::SetBodyStart(size_t body_start)
{
    _body_start = body_start;
}

size_t HttpRequest::GetBodySize() const
{
    return _body_size;
}

void HttpRequest::SetBodySize(size_t body_size)
{
    _body_size = body_size;
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

void HttpRequest::SetSocketFd(int socket_fd)
{
    _socket_fd = socket_fd;
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

bool HttpRequest::IsStreamingUpload() const
{
    return _is_streaming_upload;
}


void    HttpRequest::SetClientData(ClientConnection *client)
{
    this->_client =  client;
}

void HttpRequest::SetHeader(std::string key, std::string value)
{
    _headers[key] = value;
}

void HttpRequest::SetBodyAsBin(std::vector<char> body)
{
    // Convert vector<char> to std::string
    _body_vec = body;
    _body.assign(body.begin(), body.end());
}

void HttpRequest::SetBodyAsStr(std::string body)
{
    _body = body;
    _body_vec.assign(body.begin(), body.end());
}

void HttpRequest::SetUploadStatus(enum UploadingStatus status)
{
    _uploading_status = status;
}

enum UploadingStatus HttpRequest::GetUploadingStatus() const
{
    return _uploading_status;
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
    _remaine_bytes = 0;
    _recv_bytes = 0;
    _body_start = 0;
    _body_size = 0;
    _socket_fd = -1;
    _upload_file_path = "";
    _is_streaming_upload = false;
    _upload_file_type = -1;
    _boundry = "";
    _uploading_status = UPLOAD_START;
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

void HttpRequest::SetIsStreamingUpload(bool is_streaming_upload)
{
    _is_streaming_upload = is_streaming_upload;
}

std::string HttpRequest::GetUploadFilePath() const
{
    return _upload_file_path;
}

std::string HttpRequest::GetRelativePath(const Location *cur_location, ClientConnection *client)
{
    std::string rel_path;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        // std::cerr << "[ ERROR ] : Failed to get current working directory." << std::endl;
        cwd[0] = '\0';
    }
    if (!cur_location)
    {
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), this->GetLocation());
        rel_path = ensure_trailing_slash(rel_path);
        // std::cout << "[ WARNING ] : No matching location found, using server root: " << rel_path << std::endl;
        // std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
        return rel_path;
    }
    // std::cout << "[ DEBUG ] : Current location path: RETUN------------------" << cur_location->get_return().empty() << std::endl;
    if (!cur_location->get_return().empty())
    {
        // std::cout << ""
        SetIsRedirected(true);
        // std::cout << "\n\n\n-------------------------[ DEBUG ] : [ORIGIN ]Redirecting to : " 
        //           << cur_location->get_path() << "------" << cur_location->get_return()[1] 
        //           << "------------------\n\n" << std::endl;
        client->redirect_counter++;
        rel_path = cur_location->get_return()[1];
        // std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
        return rel_path;
    }
    else if (!cur_location->get_alias().empty())
    {
        // std::cout << "[ DEBUG ] : Using alias : " << cur_location->get_alias() << std::endl;
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), cur_location->get_alias());
    }
    else if (!cur_location->get_root_location().empty())
    {
        // std::cout << "[ DEBUG ] : Using root location : " << cur_location->get_root_location() << std::endl;
        rel_path = join_path(join_path(join_path(cwd, client->getServerConfig().get_root()), cur_location->get_root_location()), this->GetLocation());
    }
    else if (rel_path.empty())
    {
        // std::cerr << "[ WARNING ] : No alias or root location specified, using server root." << std::endl;
        // std::cout << "[ Server Root Path :" << client->getServerConfig().get_root() << " ]\n";
        rel_path = join_path(join_path(cwd, client->getServerConfig().get_root()), this->GetLocation());
    }
    rel_path = ensure_trailing_slash(rel_path);

    // std::cout << "[------------ FInal rel_path :" << rel_path << " ----------------------]\n";
    // std::cout << "[ INFO ] : Current working directory: " << cwd << std::endl;
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

