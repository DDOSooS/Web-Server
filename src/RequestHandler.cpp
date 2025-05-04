#include "../include/RequestHandler.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include "dirent.h"
#include <stdio.h>
#include <algorithm>

RequestHandler::RequestHandler(WebServer* server) : server(server) {}

void RequestHandler::processRequest(int fd)
{
    ClientData& client = server->getClient(fd);
    
    // Parse request line
    if (!client.http_request->request_line)
    {
        server->sendErrorResponse(fd, 400, "Bad Request");
        return;
    }
    // Handle standard methods
    if (client.http_request->method == "GET") {
        handleGet(fd, client);
    }
    else if (client.http_request->method == "POST") {
        handlePost(fd, client);
    }
    else {
        std::cout << "Methode : " << client.http_request->method << std::endl;
        server->sendErrorResponse(fd, 501, "Not Implemented");
    }
}

/* Functions that validate request Path */

//decode path
std::string urlDecode(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.size())
        {
            int hexValue;
            if (sscanf(str.substr(i + 1, 2).c_str(), "%x", &hexValue) == 1)
            {
                result += static_cast<char>(hexValue);
                i += 2;
            }
            else
                result += str[i]; 
        }
        else if (str[i] == '+')
            result += ' ';
        else
            result += str[i];
    }
    return result;
}

// validate path
// To do check encoding characters within the URL !!!!!!!!
bool isValidPath(std::string &path)
{
    std::string decoded_path;
    struct stat path_stat;
    
    decoded_path = urlDecode(path);
    std::cout << "decoded " << decoded_path << std::endl;
    /*
    // const char* ptr =  getcwd(NULL, 256);
    // std::string string(ptr);
    // std::cout << "absolute path is : " <<  string << std::endl;
    // // decoded_path =  decoded_path.substr(1);
    // string.append(decoded_path);
    // const char* buffer = decoded_path.c_str();
    // std::cout << "PATH : $" << buffer  << "$" << std::endl;
    // assert(stat(buffer, &path_stat) < 0);
    */
    return stat(decoded_path.c_str(), &path_stat) == 0;
}

//check if the path is refer to a directory
bool isDir(std::string &path)
{
    struct stat path_stat;
    std::string decoded_path;
    
    decoded_path = urlDecode(path);
    if(stat(decoded_path.c_str(), &path_stat) != 0)
        return (false);
    return S_ISDIR(path_stat.st_mode);
}

//check if the path refer to a regular file
bool isFile(std::string &path)
{
    std::string decoded_path;
    struct stat path_stat;
    
    decoded_path = urlDecode(path);
    if (stat(decoded_path.c_str(), &path_stat) != 0)
        return (false);
    return S_ISREG(path_stat.st_mode);
}

void RequestHandler::handleGet(int fd, ClientData& client)
{
    std::string full_path;

    std::cout << "Handling Get \n";
    // Security: prevent path traversal
    if (client.http_request->request_path.find("../") != std::string::npos)
    {
        server->sendErrorResponse(fd, 403, "Forbidden");
        return;
    }
    full_path = "./www" + client.http_request->request_path;
    if (!isValidPath(full_path))
    {
        server->sendErrorResponse(fd, 404, "Not Found");
        return ;
    }
    if (isDir(full_path))
    {
        std::string tmp;
        tmp =  full_path + "index.html";
        if (isFile(tmp))
            serveFile(fd,client, tmp);
        else
            listingDir(fd, client, full_path);
    }
    else if (isFile(full_path))
        serveFile(fd, client, full_path);
    else
        server->sendErrorResponse(fd, 403, "Forbidden");
}

void RequestHandler::handlePost(int fd, ClientData& client)
{    
    bool isMultipart;
    bool isChunked;

    isMultipart = client.http_request->findHeaderValue("Content-Type", "ultipart/form-data");
    isChunked = client.http_request->findHeaderValue("Transfer-Encoding", "chunked");
    if (isMultipart)
        processMultipartData(fd, client);
    else if (isChunked)
        processChunkedData(fd, client);
    else
    {
        // Handle regular POST data
        std::string response_body = "POST request received\n";
        response_body += "Content Length: " + std::to_string(client.http_request->request_body.length()) + "\n";
        
        client.responseHeaders = "HTTP/1.1 200 OK\r\n";
        client.responseHeaders += "Content-Type: text/plain\r\n";
        client.responseHeaders += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        client.responseHeaders += "Connection: close\r\n\r\n";
        
        client.sendBuffer = client.responseHeaders + response_body;
        server->updatePollEvents(fd, POLLOUT);
    }
}

//Listing File
void RequestHandler::listingDir(int fd, ClientData &client, const std::string &full_path)
{

    std::cout << "Serving a Listing Directory\n";
    DIR *dir_ptr;
    struct dirent *entry;
    std::stringstream response;

    dir_ptr = opendir(full_path.c_str());
    if (!dir_ptr)
    {
        server->sendErrorResponse(fd, 500 , "Internal Server Error");
        return;
    }
    response << "<!DOCTYPE html>\n"
            << "<html>\n"
            << "<head>\n"
            << "    <title>Directory listing for " << full_path << "</title>\n"
            << "    <style>\n"
            << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
            << "        h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n"
            << "        ul { list-style-type: none; padding: 0; }\n"
            << "        li { margin: 5px 0; }\n"
            << "        li a { text-decoration: none; }\n"
            << "        li a:hover { text-decoration: underline; }\n"
            << "        .folder { font-weight: bold; }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Index of  " << full_path << "</h1>\n"
            << "    <ul>\n";
    while ((entry = readdir(dir_ptr)) != NULL)
    {
        std::string name;
        std::string full_name;

        name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        full_name =  full_path + "/" + name;
        response << "<li><a href=\"" << full_name.substr(5);
        if (entry->d_type == DT_DIR)
        {
            response << "/\" class=\"folder\">" << name << "/";
        }
        else if (entry->d_type == DT_REG)
        {
            response << "\">" << name;  
        }
        response << "</a> " << "</li> \n";
    }
    response << "</ul>\n"
    << "</body>\n"
    << "</html>";
    closedir(dir_ptr);

    std::string content = response.str();
    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
    client.responseHeaders += "Content-Type: text/html\r\n";
    client.responseHeaders += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    if (client.http_request->findHeaderValue("Connection", "keep-alive"))
        client.responseHeaders += "Connection: keep-alive\r\n";
    else
        client.responseHeaders += "Connection: close\r\n";
    client.responseHeaders += "\r\n";
    client.sendBuffer = client.responseHeaders + content;
    server->updatePollEvents(fd, POLLOUT);
}

//serving File
void RequestHandler::serveFile(int fd, ClientData& client, const std::string& full_path) 
{
     size_t CHUNK_SIZE = 8192; // 8KB chunks
    
    // First call - initialize transfer
    if (!client.http_request->request_status) {
        // Open file and set up metadata
        client.http_request->ressources.open(full_path, std::ios::binary | std::ios::ate);
        if (!client.http_request->ressources.is_open()) {
            server->sendErrorResponse(fd, 404, "Not Found");
            return;
        }

        client.http_request->remaine_bytes = client.http_request->ressources.tellg();
        client.http_request->ressources.seekg(0, std::ios::beg);
        client.http_request->full_path = full_path;

        // Build headers
        std::string content_type = determineContentType(full_path);
        bool keepAlive = client.http_request->findHeaderValue("Connection", "keep-alive");
        
        std::stringstream headers;
        headers << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: " << content_type << "\r\n"
                << "Transfer-Encoding: chunked\r\n"
                << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n"
                << "\r\n";

        client.sendBuffer = headers.str();
        client.http_request->request_status = true;
    }

    // Generate data chunks
    if (client.http_request->remaine_bytes > 0)
    {
        std::vector<char> buffer(CHUNK_SIZE);
        size_t bytes_to_read = std::min(CHUNK_SIZE, static_cast<size_t>(client.http_request->remaine_bytes));
        
        client.http_request->ressources.read(buffer.data(), bytes_to_read);
        size_t bytes_read = client.http_request->ressources.gcount();

        if (bytes_read > 0) {
            std::stringstream chunk;
            chunk << std::hex << bytes_read << "\r\n";
            chunk.write(buffer.data(), bytes_read);
            chunk << "\r\n";
            
            client.sendBuffer += chunk.str();
            client.http_request->remaine_bytes -= bytes_read;
        } else {
            server->sendErrorResponse(fd, 500, "Internal Server Error");
            return;
        }
    }
    
    // Final chunk when done
    if (client.http_request->remaine_bytes == 0 && 
        client.sendBuffer.find("0\r\n\r\n") == std::string::npos)
    {
        client.sendBuffer += "0\r\n\r\n";
        client.http_request->ressources.close();
    }

    server->updatePollEvents(fd, POLLOUT);
}

std::string RequestHandler::determineContentType(const std::string& path)
{
    int size;
    size_t dot_pos;
    
    std::string contentType[] = { "html", "css", "js", "png", "jpeg", "gif", "json", "xml" , "pdf", "mp4", "mpeg", "x-www-form-urlencoded", "form-data", "woff2", "woff", "zip", "csv"};
    std::string contenetFormat[] = { "text/html", "text/css", "application/javascript", "image/png", "image/jpeg", "image/gif", "application/json", "application/xml", "application/pdf", "video/mp4", "audio/mpeg", "application/x-www-form-urlencoded", "multipart/form-data", "font/woff2", "font/woff", "application/zip", "text/csv"};
    size = sizeof(contentType) / sizeof(contentType[0]);
    dot_pos = path.rfind('.');
    if (dot_pos != std::string::npos)
    {
        std::string ext = path.substr(dot_pos + 1);
        for (int i = 0; i < size; i++)
            if (contentType[i] == ext)
                return contenetFormat[i];
    }
    return "text/plain";
}

void RequestHandler::processMultipartData(int fd, ClientData& client)
{
    std::string body = client.http_request->request_body;
    std::string headers = client.http_request->getHeader("Content-Type");

    
    size_t boundary_pos = headers.find("boundary=");
    if (boundary_pos == std::string::npos) {
        server->sendErrorResponse(fd, 400, "Bad Request: No boundary found");
        return;
    }
    
    std::string boundary = "--" + headers.substr(boundary_pos + 9);
    boundary = boundary.substr(0, boundary.find("\r\n"));
    
    std::vector<std::string> parts;
    size_t pos = 0;
    while ((pos = body.find(boundary)) != std::string::npos) {
        std::string part = body.substr(0, pos);
        if (!part.empty() && part.find("Content-Disposition") != std::string::npos) {
            parts.push_back(part);
        }
        body.erase(0, pos + boundary.length());
    }
    //
    for (const auto& part : parts) {
        size_t filename_pos = part.find("filename=\"");
        if (filename_pos != std::string::npos) {
            size_t filename_start = filename_pos + 10;
            size_t filename_end = part.find("\"", filename_start);
            if (filename_end != std::string::npos) {
                std::string filename = part.substr(filename_start, filename_end - filename_start);
                size_t data_start = part.find("\r\n\r\n");
                if (data_start != std::string::npos) {
                    data_start += 4;
                    std::string file_data = part.substr(data_start);
                    
                    std::ofstream out("uploads/" + filename, std::ios::binary);
                    out << file_data;
                    out.close();
                    
                    std::string response = "File uploaded successfully: " + filename;
                    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
                    client.responseHeaders += "Content-Type: text/plain\r\n";
                    client.responseHeaders += "Content-Length: " + std::to_string(response.length()) + "\r\n";
                    client.responseHeaders += "Connection: close\r\n\r\n";
                    client.sendBuffer = client.responseHeaders + response;
                    server->updatePollEvents(fd, POLLOUT);
                    return;
                }
            }
        }
    }
    
    server->sendErrorResponse(fd, 400, "Bad Request: No file found in multipart data");
}

void RequestHandler::processChunkedData(int fd, ClientData& client)
{
    std::string body = client.http_request->request_body;
    std::string decoded;
    size_t pos = 0;
    
    while (pos < body.length()) {
        size_t crlf = body.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        
        std::string chunk_size_str = body.substr(pos, crlf - pos);
        int chunk_size = std::stoi(chunk_size_str, nullptr, 16);
        if (chunk_size == 0) break;
        
        pos = crlf + 2;
        decoded += body.substr(pos, chunk_size);
        pos += chunk_size + 2;
    }
    
    std::ofstream out("uploads/chunked_upload.txt", std::ios::binary);
    out << decoded;
    out.close();
    
    std::string response = "Chunked data received and saved";
    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
    client.responseHeaders += "Content-Type: text/plain\r\n";
    client.responseHeaders += "Content-Length: " + std::to_string(response.length()) + "\r\n";
    client.responseHeaders += "Connection: close\r\n\r\n";
    client.sendBuffer = client.responseHeaders + response;
    server->updatePollEvents(fd, POLLOUT);
}

void RequestData::reset()
{
    this->content_length = 0;
    this->crlf_flag =  false;
    this->request_status =  false;
    this->remaine_bytes = 0;
    this->keep_alive = false;
    this->request_line =  false;
    this->headers.clear() ;
    this->method.clear();
    this->request_body.clear();
    this->content_length = 0;
    this->ressources.clear();
}