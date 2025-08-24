#include "../../include/response/HttpResponse.hpp"
#include <sys/socket.h>
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>  // for strerror
#include <iostream>
#include <fstream>

HttpResponse::HttpResponse(int status_code, std::map<std::string, std::string> headers, std::string content_type, bool is_chunked, bool keep_alive)
    : _status_code(status_code), _content_type(content_type), _is_chunked(is_chunked), _keep_alive(keep_alive)
{
    this->_headers =  headers;
    this->_status_message = this->GetStatusMessage(status_code);
    this->_file_path = "";
    this->_buffer = "";
    this->_byte_sent = 0;
    this->_byte_to_send = 0;
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

void HttpResponse::setByteSent(int byte_sent)
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
    if (!this->_keep_alive )
        this->_keep_alive = false;
    this->_byte_sent = 0;
    this->_content_type.clear();
    this->_byte_to_send = 0;
}

int HttpResponse::getStatusCode() const
{
    return this->_status_code;
}

std::string HttpResponse::getStatusMessage() const
{
    return this->_status_message;
}

int HttpResponse::getByteToSend() const
{
    return this->_byte_to_send;
}

void HttpResponse::setByteToSend(int byte_to_send)
{
    this->_byte_to_send = byte_to_send;
}

std::string HttpResponse::getHeader(std::string key) const
{
    std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
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


std::map<std::string, std::string> HttpResponse::getHeaders() const
{
    return this->_headers;
}

int HttpResponse::getByteSent() const
{
    return this->_byte_sent;
}

std::string HttpResponse::getContentType() const
{
    return this->_content_type;
}

bool HttpResponse::checkAvailablePacket() const
{
    if (!this->_buffer.empty() || !this->_file_path.empty())
        return true;
    return false;
}

bool HttpResponse::isFile() const
{
    return !this->_file_path.empty();
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

std::string HttpResponse::determineContentType(std::string path) 
{
    int size;
    size_t dot_pos;
    
    // Convert extension to lowercase for case-insensitive matching
    std::string contentType[] = { "html", "css", "js", "png", "jpeg", "jpg", "gif", "json", "xml", "pdf", "mp4", "mpeg", "x-www-form-urlencoded", "form-data", "woff2", "woff", "zip", "csv", "txt", "ico"};
    std::string contenetFormat[] = { "text/html", "text/css", "application/javascript", "image/png", "image/jpeg", "image/jpeg", "image/gif", "application/json", "application/xml", "application/pdf", "video/mp4", "audio/mpeg", "application/x-www-form-urlencoded", "multipart/form-data", "font/woff2", "font/woff", "application/zip", "text/csv", "text/plain", "image/x-icon"};
    size = sizeof(contentType) / sizeof(contentType[0]);
    dot_pos = path.rfind('.');
    
    if (dot_pos != std::string::npos)
    {
        std::string ext = path.substr(dot_pos + 1);
        for (size_t i = 0; i < ext.length(); i++)
        {
            ext[i] = std::tolower(ext[i]);
        }
        
        for (int i = 0; i < size; i++)
        {
            if (contentType[i] == ext) 
            {
                return contenetFormat[i];
            }
        }
    }
    return "text/plain";
}

long getFileSize(std::string &file_name)
{
    struct stat file_info;
    if (stat(file_name.c_str(), &file_info) != 0)
    {
        std::cerr << "error while trying to get file size";
        return -1;
    }
    return file_info.st_size;
}
    
std::string HttpResponse::toString() 
{
    std::stringstream ss;
    std::map<std::string, std::string>::const_iterator it;

    ss << this->getStatusCode();
    std::string response = "HTTP/1.1 " + ss.str() + " " + this->_status_message + "\r\n";
    
    std::vector<std::string> set_cookie_headers;
    
    for (it = this->_headers.begin(); it != this->_headers.end(); ++it)
    {
        if (it->first == "Set-Cookie")
        {
            set_cookie_headers.push_back(it->second);
        }
        else {
            response += it->first + ": " + it->second + "\r\n";
        }
    }
    
    for (size_t i = 0; i < set_cookie_headers.size(); ++i)
    {
        response += "Set-Cookie: " + set_cookie_headers[i] + "\r\n";
    }
    
    if (this->_is_chunked)
        response += "Transfer-Encoding: chunked\r\n";
    if (this->_keep_alive)
        response += "Connection: keep-alive\r\n";
    if (!this->_file_path.empty())
        this->_content_type = determineContentType(this->_file_path);
    response += "Content-Type: " + (this->_content_type.empty() ? "text/plain" : this->_content_type) + "\r\n";

    std::string body;
    if (!this->_buffer.empty())
    {
        body = this->_buffer;
        std::stringstream ss;
        ss << body.size();
        response += "Content-Length: " + ss.str() + "\r\n";
    }
    else if (!this->_file_path.empty())
    {
        std::string normalized_path = this->_file_path;
        std::string file_name = normalized_path;
        std::ifstream file(file_name.c_str(), std::ios::binary);
        if (!file)
        {
            std::cerr << "Error opening file: " << file_name << std::endl;
            throw HttpException(404, "Not Found", NOT_FOUND);
        }
        std::ostringstream file_stream;
        file_stream << file.rdbuf();
        body = file_stream.str();
        file.close();
        std::stringstream ss2;
        ss2 << body.size();
        response += "Content-Length: " + ss2.str() + "\r\n";
    }

    // Final CRLF to end headers
    response += "\r\n";
    response += body;
    return response;
}

void HttpResponse::sendResponse(int socket_fd)
{

    std::string response = this->toString();
    ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending response: send failed" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }

}

void HttpResponse::sendLastChunk( int socket_fd)
{
    std::string final_chunk = "0\r\n\r\n";
    ssize_t final_bytes = send(socket_fd, final_chunk.c_str(), final_chunk.size(), MSG_NOSIGNAL);
    if (final_bytes < 0)
    {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    return;
}

void HttpResponse::sendChunkedResponse(int socket_fd)
{
    std::string response;
    
    if (!this->_buffer.empty())
    {
        response = this->toString();
        ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
        if (bytes_sent < 0) {
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        this->_byte_sent = this->_byte_to_send; 
        return;
    }
    std::ifstream file(this->_file_path.c_str(), std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << this->_file_path << std::endl;
        throw HttpException(404, "Not Found", NOT_FOUND);
    }
    if (this->_byte_sent == 0)
    {
        std::ostringstream headers;
        headers << "HTTP/1.1 200 OK\r\n";
        headers << "Transfer-Encoding: chunked\r\n";
        headers << "Connection: keep-alive\r\n";
        if (!this->_file_path.empty())
            this->_content_type = determineContentType(this->_file_path);
        headers << "Content-Type: " + (this->_content_type.empty() ? "text/plain" : this->_content_type) + "\r\n";
        headers << "Cache-Control: public, max-age=3600\r\n";
        headers << "Accept-Ranges: bytes\r\n";
        headers << "\r\n";
        
        std::string header_str = headers.str();
        ssize_t header_bytes = send(socket_fd, header_str.c_str(), header_str.size(), MSG_NOSIGNAL);
        if (header_bytes < 0)
        {
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
    }
    // HANDLE FINAL CHUNK
    file.seekg(this->_byte_sent);
    size_t remaining_bytes = this->_byte_to_send - this->_byte_sent;
    size_t chunk_size = std::min(static_cast<size_t>(CHUNKED_SIZE), remaining_bytes);
    std::vector<char> buffer(chunk_size);
    file.read(buffer.data(), chunk_size);
    if (!file && !file.eof())
    {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    size_t actual_bytes_read = file.gcount();
    std::ostringstream chunk_stream;
    chunk_stream << std::hex << actual_bytes_read << "\r\n";
    std::string chunk_response = chunk_stream.str();
    chunk_response += std::string(buffer.data(), actual_bytes_read);
    chunk_response += "\r\n";
    ssize_t bytes_sent = send(socket_fd, chunk_response.c_str(), chunk_response.size(), MSG_NOSIGNAL);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending chunk: send failed" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    this->_byte_sent += actual_bytes_read;
    file.close();
}

HttpResponse::~HttpResponse()
{

}

void HttpResponse::setHeaders(std::map<std::string, std::string> headers)
{
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
        this->setHeader(it->first, it->second);
    }
}
