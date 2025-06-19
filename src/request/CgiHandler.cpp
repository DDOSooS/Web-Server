#include "../../include/request/CgiHandler.hpp"
#include "../../include/request/RequestHandler.hpp"
#include "../../include/WebServer.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/config/ServerConfig.hpp"
#include "../../include/config/Location.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

std::map<int, CgiHandler::CgiProcess> CgiHandler::active_cgis;

CgiHandler::CgiHandler(ClientConnection* client) : _client(client) {}

CgiHandler::~CgiHandler() {}

bool CgiHandler::isCgiRequest(HttpRequest *request) const {
    if (!request) {
        return false;
    }
    std::string request_path = request->GetLocation();
    if (request_path.empty()) {
        return false;
    }
    
    if (request_path[0] != '/') {
        return false;
    }
    
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    const Location* matching_location = _client->_server->getConfigForClient(_client->GetFd()).findBestMatchingLocation(request_path);
    
    if (!matching_location) {
        return false;
    }
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        return false;
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string extension = request_path.substr(dot_pos);
    for (std::vector<std::string>::const_iterator it = cgi_extensions.begin(); 
         it != cgi_extensions.end(); ++it) {
        if (*it == extension) {
            return true;
        }
    }
    
    return false;
}

std::string CgiHandler::getCgiPath(HttpRequest *request) const {
    std::string request_path = request->GetLocation();
    
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    
    const Location* matching_location = _client->_server->getConfigForClient(_client->GetFd()).findBestMatchingLocation(request_path);
    
    if (!matching_location) {
        throw std::runtime_error("No matching location found for CGI path");
    }
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        throw std::runtime_error("No CGI extensions or paths configured");
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        throw std::runtime_error("No file extension found in request path");
    }
    
    std::string extension = request_path.substr(dot_pos);
    
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        if (cgi_extensions[i] == extension) {
            return cgi_paths[i];
        }
    }
    
    throw std::runtime_error("No CGI path found for the given extension: " + extension);
}

bool CgiHandler::CanHandle(std::string method) {
    return ((method == "POST" || method == "GET") && isCgiRequest(_client->http_request));
}

void CgiHandler::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) {
    if (!isCgiRequest(request)) {
        if (this->GetNext()) {
            this->GetNext()->HandleRequest(request, serverConfig, clientConfig);
        }
        return;
    }
    
    try {
        if (startCgiProcess(request)) {
        } else {
            throw HttpException(500, "Failed to start CGI process", INTERNAL_SERVER_ERROR);
        }
    } catch (const std::exception &e) {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
}


char** CgiHandler::setGgiEnv(HttpRequest *request) {
    std::vector<std::string> env_vars;
    
    env_vars.push_back("REQUEST_METHOD=" + request->GetMethod());
    
    std::string request_path = request->GetLocation();
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    env_vars.push_back("SCRIPT_NAME=" + request_path);
    
    std::string query_string = request->GetQueryStringStr();
    if (query_string.empty()) {
        std::string full_location = request->GetLocation();
        size_t q_pos = full_location.find('?');
        if (q_pos != std::string::npos) {
            query_string = full_location.substr(q_pos + 1);
        }
    }
    env_vars.push_back("QUERY_STRING=" + query_string);
    
    env_vars.push_back("SERVER_NAME=webserv");
    env_vars.push_back("SERVER_PORT=" + this->to_string(_client->_server->getConfigForClient(_client->GetFd()).get_port()));
    env_vars.push_back("SERVER_PROTOCOL=" + request->GetHttpVersion());
    env_vars.push_back("SERVER_SOFTWARE=webserv/1.0");
    env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    
    env_vars.push_back("REMOTE_ADDR=" + _client->ipAddress);
    env_vars.push_back("REMOTE_PORT=" + this->to_string(_client->port));
    
    std::string script_path = _client->_server->getConfigForClient(_client->GetFd()).get_root() + request_path;
    env_vars.push_back("SCRIPT_FILENAME=" + script_path);
    env_vars.push_back("DOCUMENT_ROOT=" + _client->_server->getConfigForClient(_client->GetFd()).get_root());
    
    std::string path_info = extractPathInfo(request->GetLocation());
    if (!path_info.empty()) {
        env_vars.push_back("PATH_INFO=" + path_info);
        env_vars.push_back("PATH_TRANSLATED=" + _client->_server->getConfigForClient(_client->GetFd()).get_root() + path_info);
    }
    
    if (request->GetMethod() == "POST") {
        std::string content_type = request->GetHeader("Content-Type");
        if (!content_type.empty()) {
            env_vars.push_back("CONTENT_TYPE=" + content_type);
        }
        env_vars.push_back("CONTENT_LENGTH=" + this->to_string(request->GetBody().length()));
    }
    
    std::map<std::string, std::string> headers = request->GetHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        if (!it->second.empty()) {
            std::string header_name = "HTTP_" + it->first;
            for (size_t i = 5; i < header_name.length(); ++i) {
                if (header_name[i] == '-') {
                    header_name[i] = '_';
                }
                header_name[i] = std::toupper(header_name[i]);
            }
            env_vars.push_back(header_name + "=" + it->second);
        }
    }
    
    char **env = new char*[env_vars.size() + 1];
    for (size_t i = 0; i < env_vars.size(); ++i) {
        env[i] = strdup(env_vars[i].c_str());
    }
    env[env_vars.size()] = NULL;
    
    return env;
}

void CgiHandler::cleanupEnvironment(char** env) {
    if (!env) return;
    
    for (int i = 0; env[i] != NULL; ++i) {
        free(env[i]);
    }
    delete[] env;
}

bool CgiHandler::startCgiProcess(HttpRequest *request) {
    int pipe_out[2];
    int pipe_in[2];
    
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        return false;
    }
    
    std::string request_path = request->GetLocation();
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    
    std::string script_path = _client->_server->getConfigForClient(_client->GetFd()).get_root() + request_path;
    if (script_path.length() > 1 && script_path[script_path.length() - 1] == '/') {
        script_path = script_path.substr(0, script_path.length() - 1);
    }
    
    
    if (!isValidScriptPath(script_path)) {
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return false;
    }
    
    std::string interpreter_path;
    try {
        interpreter_path = getCgiPath(request);
        
        if (!isValidInterpreterPath(interpreter_path)) {
            close(pipe_in[0]); close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            return false;
        }
    } catch (const std::exception& e) {
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return false;
    }
    
    pid_t cgi_pid = fork();
    if (cgi_pid < 0) {
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_in[0]); close(pipe_in[1]);
        return false;
    }
    
    if (cgi_pid == 0) {
        close(pipe_out[0]);
        close(pipe_in[1]);
        
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_out[1], STDERR_FILENO);
        close(pipe_out[1]);
        
        dup2(pipe_in[0], STDIN_FILENO);
        close(pipe_in[0]);
        
        std::string script_dir = getDirectoryFromPath(script_path);
        if (chdir(script_dir.c_str()) != 0) {
            exit(1);
        }
        
        std::string script_filename = getFilenameFromPath(script_path);
        char **env = setGgiEnv(request);
        if (!env) {
            exit(1);
        }
        
        char* argv[] = {
            strdup(interpreter_path.c_str()),
            strdup(script_filename.c_str()),
            NULL
        };
        if (!argv[0] || !argv[1]) {
            cleanupEnvironment(env);
            exit(1);
        }
        
        execve(interpreter_path.c_str(), argv, env);
        
        free(argv[0]);
        free(argv[1]);
        cleanupEnvironment(env);
        exit(1);
    }
    
    close(pipe_out[1]);
    close(pipe_in[0]);
    
    if (request->GetMethod() == "POST" && !request->GetBody().empty()) {
        const std::string& body = request->GetBody();
        ssize_t written = write(pipe_in[1], body.c_str(), body.length());
        if (written != static_cast<ssize_t>(body.length())) {
        }
    }
    close(pipe_in[1]);
    
    fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);
    
    CgiProcess cgi;
    cgi.pid = cgi_pid;
    cgi.pipe_fd = pipe_out[0];
    cgi.start_time = time(NULL);
    cgi.output = "";
    cgi.client = _client;
    cgi.request = request;
    
    active_cgis[pipe_out[0]] = cgi;
    
    _client->_server->addCgiToPoll(pipe_out[0]);
    
    return true;
}

std::string CgiHandler::extractPathInfo(const std::string& url) {
    size_t question_pos = url.find('?');
    std::string path = (question_pos != std::string::npos) ? url.substr(0, question_pos) : url;
    
    size_t script_end = path.rfind('.');
    if (script_end != std::string::npos) {
        size_t next_slash = path.find('/', script_end);
        if (next_slash != std::string::npos) {
            return path.substr(next_slash);
        }
    }
    return "";
}

std::string CgiHandler::getDirectoryFromPath(const std::string& full_path) {
    size_t last_slash = full_path.rfind('/');
    if (last_slash != std::string::npos) {
        return full_path.substr(0, last_slash);
    }
}

std::string CgiHandler::getFilenameFromPath(const std::string& full_path) {
    size_t last_slash = full_path.rfind('/');
    if (last_slash != std::string::npos) {
        return full_path.substr(last_slash + 1);
    }
}

bool CgiHandler::fileExists(const std::string& file_path) {
    struct stat file_stat;
    return (stat(file_path.c_str(), &file_stat) == 0);
}

bool CgiHandler::isFileExecutable(const std::string& file_path) {
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        return false;
    }
    
    return S_ISREG(file_stat.st_mode) && (file_stat.st_mode & S_IXUSR);
}

bool CgiHandler::isValidScriptPath(const std::string& script_path) {
    if (!fileExists(script_path)) {
        return false;
    }
    
    if (access(script_path.c_str(), R_OK) != 0) {
        return false;
    }
    
    return true;
}

bool CgiHandler::isValidInterpreterPath(const std::string& interpreter_path) {
    if (!fileExists(interpreter_path)) {
        return false;
    }
    
    if (!isFileExecutable(interpreter_path)) {
        return false;
    }
    
    return true;
}

bool CgiHandler::isPathTraversalSafe(const std::string& path) {
    if (path.find("../") != std::string::npos ||
        path.find("..\\") != std::string::npos ||
        path.find("..") == 0) {
        return false;
    }
    
    return true;
}

void CgiHandler::parseHttpHeaders(const std::string& cgi_output, std::string& headers, std::string& body) {
    size_t header_end = cgi_output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = cgi_output.find("\n\n");
        if (header_end != std::string::npos) {
            headers = cgi_output.substr(0, header_end);
            body = cgi_output.substr(header_end + 2);
        } else {
            headers = "";
            body = cgi_output;
        }
    } else {
        headers = cgi_output.substr(0, header_end);
        body = cgi_output.substr(header_end + 4);
    }
}
void CgiHandler::setCgiResponseHeaders(HttpRequest* request, const std::string& headers) {
    std::istringstream header_stream(headers);
    std::string line;
    
    int status_code = 200;
    std::string status_message = "OK";
    std::string content_type = "text/html";
    
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (line.empty()) continue;
        
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string header_name = line.substr(0, colon_pos);
        std::string header_value = line.substr(colon_pos + 1);
        
        while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t')) {
            header_value.erase(0, 1);
        }
        while (!header_value.empty() && (header_value[header_value.length()-1] == ' ' || header_value[header_value.length()-1] == '\t')) {
            header_value.erase(header_value.length()-1, 1);
        }
        
        
        if (header_name == "Status") {
            size_t space_pos = header_value.find(' ');
            if (space_pos != std::string::npos) {
                status_code = std::atoi(header_value.substr(0, space_pos).c_str());
                status_message = header_value.substr(space_pos + 1);
            } else {
                status_code = std::atoi(header_value.c_str());
            }
        } 
        else if (header_name == "Content-Type" || header_name == "Content-type") {
            content_type = header_value;
        } 
        else if (header_name == "Location") {
            status_code = 302;
            status_message = "Found";
            request->GetClientDatat()->http_response->setHeader("Location", header_value);
        }
        else if (header_name == "Set-Cookie") {
            request->GetClientDatat()->http_response->setHeader("Set-Cookie", header_value);
        }
        else {
            request->GetClientDatat()->http_response->setHeader(header_name, header_value);
        }
    }
    
    request->GetClientDatat()->http_response->setStatusCode(status_code);
    request->GetClientDatat()->http_response->setStatusMessage(status_message);
    request->GetClientDatat()->http_response->setContentType(content_type);
    request->GetClientDatat()->http_response->setChunked(false);
    
    
    std::string set_cookie_check = request->GetClientDatat()->http_response->getHeader("Set-Cookie");
    if (!set_cookie_check.empty()) {
    } else {
    }
}

template <typename T>
std::string CgiHandler::to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}