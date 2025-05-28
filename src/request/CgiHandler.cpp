#include "../../include/request/CgiHandler.hpp"
#include "../../include/request/RequestHandler.hpp"
#include "../../include/WebServer.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/request/RequestHandler.hpp"
#include <iostream>
#include <stdexcept>
#include "../../include/config/ServerConfig.hpp"
#include "../../include/config/Location.hpp"
#include <sys/types.h>
#include <sys/wait.h>

CgiHandler::CgiHandler(ClientConnection* client) : _client(client) {}
CgiHandler::~CgiHandler() {}

bool CgiHandler::isCgiRequest(HttpRequest *request) const {
    
    std::string request_path = request->GetLocation();
    std::cout << "=== CGI Detection Debug ===" << std::endl;
    std::cout << "Request path: " << request_path << std::endl;
    
    const Location* matching_location = _client->_server->getServerConfig().findMatchingLocation(request_path);
    
    if (!matching_location) {
        std::cout << "No matching location found" << std::endl;
        return false;
    }
    
    std::cout << "Found matching location: " << matching_location->get_path() << std::endl;
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        std::cout << "Configured extension[" << i << "]: '" << cgi_extensions[i] << "' -> (" << cgi_paths[i] << ")"<< std::endl;
    }
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        std::cout << "❌No CGI configured for this location" << std::endl;
        return false;
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        std::cout << "❌No file extension found" << std::endl;
        return false;
    }
    
    std::string extension = request_path.substr(dot_pos);
    std::cout << "Request extension: '" << extension << "'" << std::endl;
    
    for (std::vector<std::string>::const_iterator it = cgi_extensions.begin(); 
         it != cgi_extensions.end(); ++it) {
        std::cout << "Comparing '" << extension << "' with '" << *it << "'" << std::endl;
        if (*it == extension) {
            std::cout << "✅CGI match found for extension: " << extension << std::endl;
            return true;
        }
    }
    
    std::cout << "❌No CGI extension match found" << std::endl;
    return false;
}

std::string CgiHandler::getCgiPath(HttpRequest *request) const {
    std::string request_path = request->GetLocation();
    const Location* matching_location = _client->_server->getServerConfig().findMatchingLocation(request_path);
    
    if (!matching_location) {
        throw std::runtime_error("No matching location found for CGI path");
    }
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    
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

void CgiHandler::ProccessRequest(HttpRequest *request) {
    if (!isCgiRequest(request)) {
        std::cout << "Not a CGI request, passing to next handler" << std::endl;
        if (this->GetNext()) {
            this->GetNext()->HandleRequest(request);
        }
        return;
    }
    try {
            std::cout << "CGI process started successfully - streaming output directly" << std::endl;
            std::string response = executeCgiScript(request);
            request->GetClientDatat()->http_response->setStatusCode(200);
            request->GetClientDatat()->http_response->setStatusMessage("OK");
            request->GetClientDatat()->http_response->setChunked(false);
            request->GetClientDatat()->http_response->setBuffer(response);
            request->GetClientDatat()->http_response->setContentType("text/html");
    } catch (const std::exception &e) {
        std::cerr << "Error in CGI processing: " << e.what() << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
}
// this fucntion will provide exceve with environment variables
char ** CgiHandler::setGgiEnv(HttpRequest *request) {
    std::vector<std::string> env_vars;
    env_vars.push_back("REQUEST_METHOD=" + request->GetMethod());
    env_vars.push_back("SCRIPT_NAME=" + request->GetLocation());
    env_vars.push_back("QUERY_STRING=name=ayoub&age=26&city=benguerir"); // Example query string, modify as needed TODO: get query string from request
    env_vars.push_back("SERVER_NAME=webserv");
    env_vars.push_back("SERVER_PORT=" + std::to_string(_client->_server->getServerConfig().get_port()));
    env_vars.push_back("SERVER_PROTOCOL=" + request->GetHttpVersion());
    env_vars.push_back("REMOTE_ADDR=" + _client->ipAddress);
    env_vars.push_back("REMOTE_PORT=" + std::to_string(_client->port));
    env_vars.push_back("CONTENT_TYPE=" + request->GetHeader("Content-Type"));
    env_vars.push_back("CONTENT_LENGTH=" + std::to_string(request->GetBody().length()));
    env_vars.push_back("HTTP_HOST=" + request->GetHeader("Host"));
    env_vars.push_back("HTTP_USER_AGENT=" + request->GetHeader("User-Agent"));
    env_vars.push_back("HTTP_ACCEPT=" + request->GetHeader("Accept"));
    env_vars.push_back("HTTP_ACCEPT_LANGUAGE=" + request->GetHeader("Accept-Language"));
    env_vars.push_back("HTTP_ACCEPT_ENCODING=" + request->GetHeader("Accept-Encoding"));
    env_vars.push_back("HTTP_CONNECTION=" + request->GetHeader("Connection"));
    env_vars.push_back("HTTP_COOKIE=" + request->GetHeader("Cookie"));
    env_vars.push_back("HTTP_REFERER=" + request->GetHeader("Referer"));
    env_vars.push_back("HTTP_UPGRADE_INSECURE_REQUESTS=" + request->GetHeader("Upgrade-Insecure-Requests"));
    env_vars.push_back("HTTP_X_FORWARDED_FOR=" + request->GetHeader("X-Forwarded-For"));
    env_vars.push_back("HTTP_X_FORWARDED_PROTO=" + request->GetHeader("X-Forwarded-Proto"));
    env_vars.push_back("HTTP_X_FORWARDED_HOST=" + request->GetHeader("X-Forwarded-Host"));
    env_vars.push_back("HTTP_X_FORWARDED_SERVER=" + request->GetHeader("X-Forwarded-Server"));
    char **env = new char*[env_vars.size() + 1];
    for (size_t i = 0; i < env_vars.size(); ++i) {
        env[i] = strdup(env_vars[i].c_str());
    }
    env[env_vars.size()] = NULL; // Null-terminate the array
    return env;
}

std::string CgiHandler::executeCgiScript(HttpRequest *request) {
    int pipe_out[2];
    if(pipe(pipe_out) == -1) {
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    pid_t cgi_pid = fork();
    if (cgi_pid < 0) {
        close(pipe_out[0]);
        close(pipe_out[1]);
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    if (cgi_pid == 0) {
        // Child process
        close(pipe_out[0]); // Close read end
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_out[1], STDERR_FILENO);
        close(pipe_out[1]);
        
        char **env = setGgiEnv(request);
        if (!env) {
            std::cerr << "Error setting CGI environment variables" << std::endl;
            exit(1);
        }
        std::string request_path = request->GetLocation();
        std::string script_name = _client->_server->getServerConfig().get_root() + request_path;
        
        char* interpreter = strdup(getCgiPath(request).c_str());
        if (!interpreter) {
            std::cerr << "Error allocating memory for interpreter path" << std::endl;
            exit(1);
        }
        char* script_path_cstr = strdup(script_name.c_str());
        if (!script_path_cstr) {
            std::cerr << "Error allocating memory for script path" << std::endl;
            free(interpreter);
            exit(1);
        }
        char* argv[] = {interpreter, script_path_cstr, NULL};
        char** envp = env; // Use the environment variables set earlier
        
        execve(interpreter, argv, env);
        // If execve fails
        std::cerr << "Error executing CGI script: " << strerror(errno) << std::endl;
        std::cout << "Script path: " << script_name << std::endl;

        free(interpreter);
        free(script_path_cstr);
        close(pipe_out[1]);
        // Exit child process with error code
        exit(1);
    }
    
    // Parent process
    close(pipe_out[1]); // Close write end
    
    std::string result;
    char buffer[4097]; // +1 for null terminator
    ssize_t bytes_read;
    
    while((bytes_read = read(pipe_out[0], buffer, 4096)) > 0) {
        buffer[bytes_read] = '\0'; // Null terminate
        result += buffer;
    }
    
    close(pipe_out[0]);
    
    // Wait for child to prevent zombies
    int status;
    waitpid(cgi_pid, &status, 0);
    
    std::ofstream outputFile1("cgi_out.txt");
    if (outputFile1.is_open()) {
        outputFile1 << result << std::endl;
        outputFile1.close();
    } else {
        std::cerr << "Error opening file!" << std::endl;
    }
    return result;
}

bool CgiHandler::cgiExec(){
    // setup env
    // create pipe
    // execute script
    return (true);
}
