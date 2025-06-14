#include "../../include/response/HttpResponse.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <cstring>  // for strerror
#include <cerrno>   // for errno
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
        // Convert extension to lowercase
        for (size_t i = 0; i < ext.length(); i++) {
            ext[i] = std::tolower(ext[i]);
        }
        
        for (int i = 0; i < size; i++) {
            if (contentType[i] == ext) {
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
    std::cout << "[INFO ] : [ --- HTTP RESPONSE TO STRING METHOD --- ]\n";
    std::stringstream ss;
    std::map<std::string, std::string>::const_iterator it;

    ss << this->getStatusCode();
    std::string response = "HTTP/1.1 " + ss.str() + " " + this->_status_message + "\r\n";
    for (it = this->_headers.begin(); it != this->_headers.end(); ++it)
        response += it->first + ": " + it->second + "\r\n";
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
    std::cout << "[Debug : ] ---Start of Sending A response--- !!\n";

    std::string response = this->toString();
    
    ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending response: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    std::cout << "Bytes sent: " << bytes_sent << " out of " << response.size() << " bytes" << std::endl;

}
#include <cstdlib>
void HttpResponse::sendChunkedResponse(int socket_fd)
{
    std::cout << "[Debug] ---Start of Sending A Chunked response---\n";
    std::string response;
    
    // Handle buffer-based responses (non-file) -> BUFFER
    if (!this->_buffer.empty())
    {
        std::cout << "[Debug] Sending chunked response from buffer\n";
        response = this->toString();
        ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Error sending chunked response: " << strerror(errno) << std::endl;
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        this->_byte_sent = this->_byte_to_send; 
        std::cout << "Buffer response sent: " << bytes_sent << " bytes" << std::endl;
        return;
    }
    
    // Handle file-based responses
    std::ifstream file(this->_file_path.c_str(), std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << this->_file_path << std::endl;
        throw HttpException(404, "Not Found", NOT_FOUND);
    }
    
    // Send headers only once (when _byte_sent == 0) => FIRST CHUNK
    if (this->_byte_sent == 0)
    {
        std::ostringstream headers;
        headers << "HTTP/1.1 200 OK\r\n";
        headers << "Transfer-Encoding: chunked\r\n";
        headers << "Connection: keep-alive\r\n";
        
        if (!this->_file_path.empty())
            this->_content_type = determineContentType(this->_file_path);
        headers << "Content-Type: " + (this->_content_type.empty() ? "text/plain" : this->_content_type) + "\r\n";
        headers << "\r\n";
        
        std::string header_str = headers.str();
        ssize_t header_bytes = send(socket_fd, header_str.c_str(), header_str.size(), 0);
        if (header_bytes < 0) {
            std::cerr << "Error sending headers: " << strerror(errno) << std::endl;
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        std::cout << "[Debug] Headers sent: " << header_bytes << " bytes\n";
        std::cout << "[Debug] Content type: " << this->_content_type << std::endl;
    }

    // HANDLE FINAL CHUNK
    if (this->_byte_sent >= this->_byte_to_send)
    {
        std::cout << "[Debug] Sending final chunk (size 0)\n";
        std::string final_chunk = "0\r\n\r\n";
        ssize_t final_bytes = send(socket_fd, final_chunk.c_str(), final_chunk.size(), 0);
        if (final_bytes < 0) {
            std::cerr << "Error sending final chunk: " << strerror(errno) << std::endl;
            throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
        }
        std::cout << "[Debug] Final chunk sent: " << final_bytes << " bytes\n";
        return;
    }
    file.seekg(this->_byte_sent);
    size_t remaining_bytes = this->_byte_to_send - this->_byte_sent;
    size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), remaining_bytes);
    // Read chunk from file
    std::vector<char> buffer(chunk_size);
    file.read(buffer.data(), chunk_size);
    if (!file && !file.eof()) {
        std::cerr << "Error reading file chunk" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    // Get actual bytes read (might be less than requested at end of file)
    size_t actual_bytes_read = file.gcount();
    
    // Create chunk response
    std::ostringstream chunk_stream;
    chunk_stream << std::hex << actual_bytes_read << "\r\n";
    std::string chunk_response = chunk_stream.str();
    chunk_response += std::string(buffer.data(), actual_bytes_read);
    chunk_response += "\r\n";
    ssize_t bytes_sent = send(socket_fd, chunk_response.c_str(), chunk_response.size(), 0);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending chunk: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    this->_byte_sent += actual_bytes_read;
    std::cout << "[Debug] Chunk sent: size=" << actual_bytes_read 
              << ", total_sent=" << this->_byte_sent 
              << "/" << this->_byte_to_send << std::endl;
    
    file.close();
}
HttpResponse::~HttpResponse()
{}