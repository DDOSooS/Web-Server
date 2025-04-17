#include "RequestHandler.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include "dirent.h"


RequestHandler::RequestHandler(WebServer* server) : server(server) {}

void RequestHandler::processRequest(int fd)
{
    ClientData& client = server->getClient(fd);
    
    // Parse request line
    size_t line_end = client.requestHeaders.find("\r\n");
    if (line_end == std::string::npos) {
        server->sendErrorResponse(fd, 400, "Bad Request");
        return;
    }
    
    std::string request_line = client.requestHeaders.substr(0, line_end);
    std::istringstream iss(request_line);
    std::string method, path;
    iss >> method >> path;

    // Handle standard methods
    if (method == "GET") {
        handleGet(fd, client);
    }
    else if (method == "POST") {
        handlePost(fd, client);
    }
    else {
        server->sendErrorResponse(fd, 501, "Not Implemented");
    }
}

/* Functions that validate request Path */

// validate path
bool isValidPath(std::string &path)
{
    struct stat path_stat;

    return stat(path.c_str(), &path_stat) == 0;
}

//check if the path is refer to a directory
bool isDir(std::string &path)
{
    struct stat path_stat;
    
    if(stat(path.c_str(), &path_stat) != 0)
        return (false);
    return S_ISDIR(path_stat.st_mode);
}

//check if the path refer to a regular file
bool isFile(std::string &path)
{
    struct stat path_stat;

    if (stat(path.c_str(), &path_stat) != 0)
        return (false);
    return S_ISREG(path_stat.st_mode);
}



void RequestHandler::handleGet(int fd, ClientData& client)
{
    std::string full_path;
    
    // Security: prevent path traversal
    if (client.requestPath.find("../") != std::string::npos)
    {
        server->sendErrorResponse(fd, 403, "Forbidden");
        return;
    }
    
    full_path = "./www" + client.requestPath;
    if (!isValidPath(full_path))
    {
        server->sendErrorResponse(fd, 404, "Not Found");
        return ;
    }
    if (isDir(full_path))
    {
        std::cout << "path is refering to a DIR11111\n";
        std::string tmp;
        tmp =  full_path + "index.html";
        std::cout << "FULL PATH: " << full_path << " TMP:" << tmp << std::endl; 
        if (isFile(tmp))
        {
            std::cout << "listing a file 11  << \n";
            serveFile(fd,client, tmp);
        }
        else
        {
            std::cout << "listing a Dir 222<< \n";
            listingDir(fd, client, full_path);
        }
    }
    else if (isFile(full_path))
    {
        std::cout << "path is refering to a File33\n";
        serveFile(fd, client, full_path);
    }
    else
        server->sendErrorResponse(fd, 403, "Forbidden");
}

void RequestHandler::handlePost(int fd, ClientData& client) {
    std::string headers = client.requestHeaders;
    
    bool isMultipart = headers.find("Content-Type: multipart/form-data") != std::string::npos;
    bool isChunked = headers.find("Transfer-Encoding: chunked") != std::string::npos;

    if (isMultipart) {
        processMultipartData(fd, client);
    }
    else if (isChunked) {
        processChunkedData(fd, client);
    }
    else {
        // Handle regular POST data
        std::string response_body = "POST request received\n";
        response_body += "Content Length: " + std::to_string(client.requestBody.length()) + "\n";
        
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
    DIR *dir_ptr;
    struct dirent *entry;
    std::stringstream response;

    std::cout << full_path << "====\n";
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
            // << "    <li> <a href="" > Name</a> </i>"
    int counter = 0;
    while ((entry = readdir(dir_ptr)) != NULL)
    {
        std::cout << "Counter << " << counter << std::endl;
        counter++;
        std::string name;
        std::string full_name;

        name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        full_name =  full_path + "/" + name;
        response << "<li><a href=\"" << full_name.substr(5);
        if (entry->d_type == DT_DIR)
        {
            response << "/\" class=\"folder\">" << name << "/";  // Add trailing slash in display too
        }
        else if (entry->d_type == DT_REG)
        {
            response << "\">" << name;  // Add ">" after the closing quote
        }
        response << "</a> " << "</li> \n";
    }
    response << "</ul>\n"
    << "</body>\n"
    << "</html>";
    
    closedir(dir_ptr);
    
    // Send the directory listing
    std::string content = response.str();
    
    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
    client.responseHeaders += "Content-Type: text/html\r\n";
    client.responseHeaders += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    
    bool keepAlive = (client.requestHeaders.find("Connection: keep-alive") != std::string::npos);
    if (keepAlive)
    {
        client.responseHeaders += "Connection: keep-alive\r\n";
    } 
    else
    {
        client.responseHeaders += "Connection: close\r\n";
    }
    client.responseHeaders += "\r\n";
    client.sendBuffer = client.responseHeaders + content;
    server->updatePollEvents(fd, POLLOUT);
    std::cout << "end Of Listing \n";
}

//serving File
void RequestHandler::serveFile(int fd, ClientData& client, const std::string& full_path)
{
    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        server->sendErrorResponse(fd, 404, "Not Found");
        return;
    }
    
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> file_data(file_size);
    if (!file.read(file_data.data(), file_size)) {
        server->sendErrorResponse(fd, 500, "Internal Server Error");
        return;
    }
    
    std::string content_type = determineContentType(full_path);
    client.responseHeaders = "HTTP/1.1 200 OK\r\n";
    client.responseHeaders += "Content-Type: " + content_type + "\r\n";
    client.responseHeaders += "Content-Length: " + std::to_string(file_size) + "\r\n";
    
    bool keepAlive = (client.requestHeaders.find("Connection: keep-alive") != std::string::npos);
    if (keepAlive)
    {
        client.responseHeaders += "Connection: keep-alive\r\n";
    }
    else
    {
        client.responseHeaders += "Connection: close\r\n";
    }
    client.responseHeaders += "\r\n";
    
    client.sendBuffer = client.responseHeaders + std::string(file_data.data(), file_size);
    server->updatePollEvents(fd, POLLOUT);
}

std::string RequestHandler::determineContentType(const std::string& path)
{
    size_t dot_pos = path.rfind('.');
    if (dot_pos != std::string::npos) {
        std::string ext = path.substr(dot_pos + 1);
        if (ext == "html") return "text/html";
        else if (ext == "css") return "text/css";
        else if (ext == "js") return "application/javascript";
        else if (ext == "png") return "image/png";
        else if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        else if (ext == "gif") return "image/gif";
    }
    return "text/plain";
}

void RequestHandler::processMultipartData(int fd, ClientData& client) {
    std::string headers = client.requestHeaders;
    std::string body = client.requestBody;
    
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

void RequestHandler::processChunkedData(int fd, ClientData& client) {
    std::string body = client.requestBody;
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