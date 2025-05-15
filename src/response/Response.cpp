#include "../../include/response/HttpResponse.hpp"

HttpResponse::HttpResponse(int status_code, std::unordered_map<std::string, std::string> headers, std::string content_type, bool is_chunked, bool keep_alive)
    : _status_code(status_code), _content_type(content_type), _is_chunked(is_chunked), _keep_alive(keep_alive)
{
    this->_headers =  headers;
    this->_status_message = this->GetStatusMessage(status_code);
    this->_file_path = "";
    this->_buffer = "";
    this->_byte_sent = 0;
}

void HttpResponse::setStatusCode(int code)
{
    this->_status_code = code;
}

void HttpResponse::setStatusMessage(std::string message)
{
    this->_status_message = message;
}

void HttpResponse::setHeader(std::string key, std::string value)
{
    this->_headers[key] = value;
}

void HttpResponse::setFilePath(std::string path)
{
    this->_file_path = path;
}
void HttpResponse::setChunked(bool chunked)
{
    this->_is_chunked = chunked;
}

void HttpResponse::setKeepAlive(bool keep_alive)
{
    this->_keep_alive = keep_alive;
}

void HttpResponse::setBuffer(std::string buffer)
{
    this->_buffer = buffer;
}

void HttpResponse::setByteSent(size_t byte_sent)
{
    this->_byte_sent = byte_sent;
}

void HttpResponse::setContentType(std::string content_type)
{
    this->_content_type = content_type;
}

void HttpResponse::clear()
{
    this->_status_code = 0;
    this->_status_message.clear();
    this->_headers.clear();
    this->_file_path.clear();
    this->_buffer.clear();
    this->_is_chunked = false;
    this->_keep_alive = false;
    this->_byte_sent = 0;
    this->_content_type.clear();

}

int HttpResponse::getStatusCode() const
{
    return this->_status_code;
}

std::string HttpResponse::getStatusMessage() const
{
    return this->_status_message;
}

std::string HttpResponse::getHeader(std::string key) const
{
    auto it = this->_headers.find(key);
    if (it != this->_headers.end())
        return it->second;
    return "";
}

std::string HttpResponse::getFilePath() const
{
    return this->_file_path;
}

bool HttpResponse::isChunked() const
{
    return this->_is_chunked;
}

bool HttpResponse::isKeepAlive() const
{
    return this->_keep_alive;
}

std::string HttpResponse::getBuffer() const
{
    return this->_buffer;
}

std::unordered_map<std::string, std::string> HttpResponse::getHeaders() const
{
    return this->_headers;
}

size_t HttpResponse::getByteSent() const
{
    return this->_byte_sent;
}

std::string HttpResponse::getContentType() const
{
    return this->_content_type;
}

bool HttpResponse::checkAvailablePacket() const
{
    if (this->_buffer.empty() || this->_file_path.empty())
        return false;
    return true;
}

std::string HttpResponse::GetStatusMessage(int code) const 
{
    switch (code)
    {
        case 200:
            return "OK";
        case 404:
            return "Not Found";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 405:
            return "Method Not Allowed";
        case 408:
            return "Request Timeout";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 503:
            return "Service Unavailable";
        default:
            return "Unknown Status";
    }
}

std::string HttpResponse::toString() const
{
    std::string response = "HTTP/1.1 " + std::to_string(this->getStatusCode()) + " " + this->_status_message + "\r\n";
    for (const auto& header : this->_headers)
    {
        response += header.first + ": " + header.second + "\r\n";
    }
    if (this->_is_chunked)
        response += "Transfer-Encoding: chunked\r\n";
    if (this->_keep_alive)
        response += "Connection: keep-alive\r\n";
    if (!this->_content_type.empty())
        response += "Content-Type: " + this->_content_type + "\r\n";
    else
        response += "Content-Type: text/plain\r\n";
    
    // handle if the buffer is the paquet that we need to send
    if (!this->_buffer.empty())
    {
        response += std::string("Content-Length: ") + std::to_string(this->_buffer.size()) + "\r\n";  
        response += "\r\n";
        response += this->_buffer;
    }
    // handle if the paquet that we need to send is a file
    if (!this->_file_path.empty())
    {
        std::string file_content;
        std::string file_name = "../www/" + this->_file_path;
        std::ifstream file(file_name, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error opening file: " << this->_file_path << std::endl;
            throw HttpException(400, "Not Found", ERROR_TYPE::NOT_FOUND);
        }
        if (file)
        {
            response += std::string("Content-Length: ") + std::to_string(file.tellg()) + "\r\n";
            while (std::getline(file, file_content))
                response += file_content;
            file.close();
            response += "\r\n";
        }
    }
    return response;
}


void HttpResponse::sendResponse(int socket_fd)
{
    std::string response = this->toString();
    ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending response: " << std::endl;
        throw HttpException(500, "Internal Server Error", ERROR_TYPE::INTERNAL_SERVER_ERROR);
    }
}

void HttpResponse::sendChunkedResponse(int socket_fd)
{
    size_t  byte_sent = 0;

    byte_sent =  send(socket_fd, _buffer.c_str(), _buffer.size(), 0);
    if (byte_sent == 0)
    {
        std::cerr << "Error sending chunked response" << std::endl;
        throw HttpException(500, "Internal Server Error", ERROR_TYPE::INTERNAL_SERVER_ERROR);
    }
}


HttpResponse::~HttpResponse()
{}