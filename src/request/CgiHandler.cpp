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

CgiHandler::CgiHandler(ClientConnection* client) : _client(client) {}

CgiHandler::~CgiHandler() {}

bool CgiHandler::isCgiRequest(HttpRequest *request) const {
    if (!request) {
        std::cout << "❌Request is null" << std::endl;
        return false;
    }
    std::cout << "Querying String: " << this->_client->http_request->GetQueryStringStr() << std::endl;
    std::string request_path = request->GetLocation();
    if (request_path.empty()) {
        std::cout << "❌Empty request path" << std::endl;
        return false;
    }
    
    if (request_path[0] != '/') {
        std::cout << "❌Request path does not start with '/'" << std::endl;
        return false;
    }
    
    // Remove query string for location matching
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    const Location* matching_location = _client->_server->getServerConfig().findBestMatchingLocation(request_path);
    
    if (!matching_location) {
        std::cout << "❌No matching location found for: " << request_path << std::endl;
        return false;
    }
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        std::cout << "❌No CGI extensions or paths configured for location" << std::endl;
        return false;
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        std::cout << "❌No file extension found in path: " << request_path << std::endl;
        return false;
    }
    
    std::string extension = request_path.substr(dot_pos);
    for (std::vector<std::string>::const_iterator it = cgi_extensions.begin(); 
         it != cgi_extensions.end(); ++it) {
        if (*it == extension) {
            std::cout << "✅CGI extension match found: " << extension << std::endl;
            return true;
        }
    }
    
    std::cout << "❌No CGI extension match found for: " << extension << std::endl;
    return false;
}

std::string CgiHandler::getCgiPath(HttpRequest *request) const {
    std::string request_path = request->GetLocation();
    
    // Remove query string
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    
    // FIXED: Use the same method as isCgiRequest()
    const Location* matching_location = _client->_server->getServerConfig().findBestMatchingLocation(request_path);
    
    if (!matching_location) {
        std::cerr << "❌No matching location found for CGI path: " << request_path << std::endl;
        throw std::runtime_error("No matching location found for CGI path");
    }
    
    std::vector<std::string> cgi_extensions = matching_location->get_cgiExt();
    std::vector<std::string> cgi_paths = matching_location->get_cgiPath();
    
    if (cgi_extensions.empty() || cgi_paths.empty()) {
        std::cerr << "❌No CGI extensions or paths configured" << std::endl;
        throw std::runtime_error("No CGI extensions or paths configured");
    }
    
    size_t dot_pos = request_path.rfind('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "❌No file extension found in request path: " << request_path << std::endl;
        throw std::runtime_error("No file extension found in request path");
    }
    
    std::string extension = request_path.substr(dot_pos);
    
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        if (cgi_extensions[i] == extension) {
            std::cout << "✅Found interpreter for " << extension << ": " << cgi_paths[i] << std::endl;
            return cgi_paths[i];
        }
    }
    
    std::cerr << "❌No CGI path found for extension: " << extension << std::endl;
    throw std::runtime_error("No CGI path found for the given extension: " + extension);
}

bool CgiHandler::CanHandle(std::string method) {
    return ((method == "POST" || method == "GET") && isCgiRequest(_client->http_request));
}

void CgiHandler::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig) {
    if (!isCgiRequest(request)) {
        std::cout << "Not a CGI request, passing to next handler" << std::endl;
        if (this->GetNext()) {
            this->GetNext()->HandleRequest(request, serverConfig);
        }
        return;
    }
    
    try {
        std::cout << "CGI process started successfully" << std::endl;
        std::string response = executeCgiScript(request);
        
        // Parse CGI output for headers and body
        std::string headers, body;
        parseHttpHeaders(response, headers, body);
        
        // Set response headers from CGI output
        setCgiResponseHeaders(request, headers);
        
        // Set response body
        request->GetClientDatat()->http_response->setBuffer(body);
        
    } catch (const std::exception &e) {
        std::cerr << "Error in CGI processing: " << e.what() << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
}

char** CgiHandler::setGgiEnv(HttpRequest *request) {
    std::vector<std::string> env_vars;
    
    // Required CGI environment variables
    env_vars.push_back("REQUEST_METHOD=" + request->GetMethod());
    
    // FIX: Clean script name (remove query string and normalize path)
    std::string request_path = request->GetLocation();
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    env_vars.push_back("SCRIPT_NAME=" + request_path);
    
    // FIX: Ensure query string is properly extracted and set
    std::string query_string = request->GetQueryStringStr();
    if (query_string.empty()) {
        // Try to extract from location if not already set
        std::string full_location = request->GetLocation();
        size_t q_pos = full_location.find('?');
        if (q_pos != std::string::npos) {
            query_string = full_location.substr(q_pos + 1);
        }
    }
    std::cout << "🔍 Final Query String for CGI: '" << query_string << "'" << std::endl;
    env_vars.push_back("QUERY_STRING=" + query_string);
    
    // Server information
    env_vars.push_back("SERVER_NAME=webserv");
    env_vars.push_back("SERVER_PORT=" + this->to_string(_client->_server->getServerConfig().get_port()));
    env_vars.push_back("SERVER_PROTOCOL=" + request->GetHttpVersion());
    env_vars.push_back("SERVER_SOFTWARE=webserv/1.0");
    env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    
    // Client information
    env_vars.push_back("REMOTE_ADDR=" + _client->ipAddress);
    env_vars.push_back("REMOTE_PORT=" + this->to_string(_client->port));
    
    // Script and document information
    std::string script_path = _client->_server->getServerConfig().get_root() + request_path;
    env_vars.push_back("SCRIPT_FILENAME=" + script_path);
    env_vars.push_back("DOCUMENT_ROOT=" + _client->_server->getServerConfig().get_root());
    
    // PATH_INFO and PATH_TRANSLATED
    std::string path_info = extractPathInfo(request->GetLocation());
    if (!path_info.empty()) {
        env_vars.push_back("PATH_INFO=" + path_info);
        env_vars.push_back("PATH_TRANSLATED=" + _client->_server->getServerConfig().get_root() + path_info);
    }
    
    // Content information (critical for POST)
    if (request->GetMethod() == "POST") {
        std::string content_type = request->GetHeader("Content-Type");
        if (!content_type.empty()) {
            env_vars.push_back("CONTENT_TYPE=" + content_type);
        }
        env_vars.push_back("CONTENT_LENGTH=" + this->to_string(request->GetBody().length()));
    }
    
    // FIX: Enhanced HTTP headers (add all headers as HTTP_* variables)
    std::map<std::string, std::string> headers = request->GetHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        if (!it->second.empty()) {
            std::string header_name = "HTTP_" + it->first;
            // Convert to uppercase and replace hyphens with underscores
            for (size_t i = 5; i < header_name.length(); ++i) {
                if (header_name[i] == '-') {
                    header_name[i] = '_';
                }
                header_name[i] = std::toupper(header_name[i]);
            }
            env_vars.push_back(header_name + "=" + it->second);
            
            // Special handling for important headers
            if (it->first == "Cookie") {
                std::cout << "🍪 Setting HTTP_COOKIE: " << it->second << std::endl;
            }
        }
    }
    
    // Create environment array
    char **env = new char*[env_vars.size() + 1];
    for (size_t i = 0; i < env_vars.size(); ++i) {
        env[i] = strdup(env_vars[i].c_str());
        std::cout << "ENV[" << i << "]: " << env_vars[i] << std::endl;
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

std::string CgiHandler::executeCgiScript(HttpRequest *request) {
    int pipe_out[2];
    int pipe_in[2];
    
    // Create pipes for communication
    std::cout << "Creating pipes for CGI execution" << std::endl;
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        std::cerr << "Failed to create pipes: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    std::cout << "Pipes created successfully" << std::endl;
    
    // Validate script and interpreter paths
    std::string request_path = request->GetLocation();
    size_t question_pos = request_path.find('?');
    if (question_pos != std::string::npos) {
        request_path = request_path.substr(0, question_pos);
    }
    
    // FIXED: Don't add trailing slash for CGI scripts
    std::string script_path = _client->_server->getServerConfig().get_root() + request_path;
    // Remove trailing slash if it exists and path is not root
    if (script_path.length() > 1 && script_path[script_path.length() - 1] == '/') {
        script_path = script_path.substr(0, script_path.length() - 1);
    }
    
    std::cout << "FIXED Script path: " << script_path << std::endl;
    
    // Validate script exists and is executable
    if (!isValidScriptPath(script_path)) {
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        throw HttpException(404, "Script Not Found or Not Executable", NOT_FOUND);
    }
    
    // Validate interpreter exists and is executable
    std::string interpreter_path;
    try {
        interpreter_path = getCgiPath(request);
        std::cout << "Interpreter path: " << interpreter_path << std::endl;
        
        if (!isValidInterpreterPath(interpreter_path)) {
            close(pipe_in[0]); close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            throw HttpException(500, "CGI Interpreter Not Found", INTERNAL_SERVER_ERROR);
        }
    } catch (const std::exception& e) {
        std::cerr << "❌Exception in getCgiPath: " << e.what() << std::endl;
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        throw HttpException(500, "CGI Configuration Error: " + std::string(e.what()), INTERNAL_SERVER_ERROR);
    }
    
    pid_t cgi_pid = fork();
    if (cgi_pid < 0) {
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_in[0]); close(pipe_in[1]);
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    if (cgi_pid == 0) {
        // Child process
        close(pipe_out[0]); // Close read end
        close(pipe_in[1]);  // Close write end
        
        // Redirect stdout/stderr to pipe
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_out[1], STDERR_FILENO);
        close(pipe_out[1]);
        
        // Redirect stdin from pipe (for POST data)
        dup2(pipe_in[0], STDIN_FILENO);
        close(pipe_in[0]);
        
        // Change to script's directory
        std::string script_dir = getDirectoryFromPath(script_path);
        if (chdir(script_dir.c_str()) != 0) {
            std::cerr << "Failed to change to script directory: " << script_dir 
                      << " - " << strerror(errno) << std::endl;
            exit(1);
        }
        
        // Get script filename
        std::string script_filename = getFilenameFromPath(script_path);
        
        // Set up environment
        char **env = setGgiEnv(request);
        if (!env) {
            std::cerr << "Error setting CGI environment variables" << std::endl;
            exit(1);
        }
        
        // Prepare execution arguments
        char* argv[] = {
            strdup(interpreter_path.c_str()),
            strdup(script_filename.c_str()),
            NULL
        };
        if (!argv[0] || !argv[1]) {
            std::cerr << "Error allocating memory for CGI arguments" << std::endl;
            cleanupEnvironment(env);
            exit(1);
        }
        // Execute the CGI script
        execve(interpreter_path.c_str(), argv, env);
        
        // If we reach here, execve failed
        std::cerr << "CGI exec failed: " << strerror(errno) << std::endl;
        std::cerr << "Interpreter: " << interpreter_path << std::endl;
        std::cerr << "Script: " << script_filename << std::endl;
        std::cerr << "Working dir: " << script_dir << std::endl;
        
        // Cleanup and exit
        free(argv[0]);
        free(argv[1]);
        cleanupEnvironment(env);
        exit(1);
    }
    
    // Parent process
    close(pipe_out[1]); // Close write end
    close(pipe_in[0]);  // Close read end
    
    // Send POST data if present
    if (request->GetMethod() == "POST" && !request->GetBody().empty()) {
        std::cout << "Handling POST request in CGI ..." << std::endl;
        const std::string& body = request->GetBody();
        ssize_t written = write(pipe_in[1], body.c_str(), body.length());
        if (written != static_cast<ssize_t>(body.length())) {
            std::cerr << "Warning: Could not write all POST data to CGI script" << std::endl;
        }
    }
    close(pipe_in[1]);
    
    // Read output from CGI script
    std::string result;
    char buffer[4096];
    ssize_t bytes_read;
    
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        result += buffer;
    }
    
    close(pipe_out[0]);
    
    // Wait for child process and check exit status
    int status;
    waitpid(cgi_pid, &status, 0);
    
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            std::cerr << "CGI script exited with code: " << exit_code << std::endl;
            throw HttpException(500, "CGI Script Error", INTERNAL_SERVER_ERROR);
        }
    } else if (WIFSIGNALED(status)) {
        int signal_num = WTERMSIG(status);
        std::cerr << "CGI script terminated by signal: " << signal_num << std::endl;
        throw HttpException(500, "CGI Script Timeout or Error", INTERNAL_SERVER_ERROR);
    }
    
    // Debug: write output to file for debugging
    std::ofstream outputFile("cgi_out.txt");
    if (outputFile.is_open()) {
        outputFile << result;
        outputFile.close();
    }
    
    return result;
}


// Helper function to extract PATH_INFO
std::string CgiHandler::extractPathInfo(const std::string& url) {
    // Remove query string first
    size_t question_pos = url.find('?');
    std::string path = (question_pos != std::string::npos) ? url.substr(0, question_pos) : url;
    
    // Find script name and extract remaining path
    size_t script_end = path.rfind('.');
    if (script_end != std::string::npos) {
        // Find the end of the script name (next slash or end of string)
        size_t next_slash = path.find('/', script_end);
        if (next_slash != std::string::npos) {
            return path.substr(next_slash);
        }
    }
    return "";
}

// Helper function to extract directory from full path
std::string CgiHandler::getDirectoryFromPath(const std::string& full_path) {
    size_t last_slash = full_path.rfind('/');
    if (last_slash != std::string::npos) {
        return full_path.substr(0, last_slash);
    }
    return "."; // Current directory if no slash found
}

// Helper function to extract filename from full path
std::string CgiHandler::getFilenameFromPath(const std::string& full_path) {
    size_t last_slash = full_path.rfind('/');
    if (last_slash != std::string::npos) {
        return full_path.substr(last_slash + 1);
    }
    return full_path; // Return as-is if no slash found
}

// Check if file exists
bool CgiHandler::fileExists(const std::string& file_path) {
    struct stat file_stat;
    return (stat(file_path.c_str(), &file_stat) == 0);
}

// Check if file is executable
bool CgiHandler::isFileExecutable(const std::string& file_path) {
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        return false;
    }
    
    // Check if it's a regular file and has execute permission
    return S_ISREG(file_stat.st_mode) && (file_stat.st_mode & S_IXUSR);
}

// Validate script path
bool CgiHandler::isValidScriptPath(const std::string& script_path) {
    // Check if file exists
    if (!fileExists(script_path)) {
        std::cerr << "Script not found: " << script_path << std::endl;
        return false;
    }
    
    // Check if file is readable
    if (access(script_path.c_str(), R_OK) != 0) {
        std::cerr << "Script not readable: " << script_path << std::endl;
        return false;
    }
    
    std::cout << "Script validation passed: " << script_path << std::endl;
    return true;
}

// Validate interpreter path
bool CgiHandler::isValidInterpreterPath(const std::string& interpreter_path) {
    // Check if interpreter exists
    if (!fileExists(interpreter_path)) {
        std::cerr << "Interpreter not found: " << interpreter_path << std::endl;
        return false;
    }
    
    // Check if interpreter is executable
    if (!isFileExecutable(interpreter_path)) {
        std::cerr << "Interpreter not executable: " << interpreter_path << std::endl;
        return false;
    }
    
    std::cout << "Interpreter validation passed: " << interpreter_path << std::endl;
    return true;
}

// Security: check for path traversal attempts
bool CgiHandler::isPathTraversalSafe(const std::string& path) {
    // Check for dangerous patterns
    if (path.find("../") != std::string::npos ||
        path.find("..\\") != std::string::npos ||
        path.find("..") == 0) {
        return false;
    }
    
    return true;
}

// Parse HTTP headers from CGI output
void CgiHandler::parseHttpHeaders(const std::string& cgi_output, std::string& headers, std::string& body) {
    size_t header_end = cgi_output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = cgi_output.find("\n\n");
        if (header_end != std::string::npos) {
            headers = cgi_output.substr(0, header_end);
            body = cgi_output.substr(header_end + 2);
        } else {
            // No headers found, treat entire output as body
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
        // Remove carriage return if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        // Skip empty lines
        if (line.empty()) continue;
        
        // Find colon separator
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string header_name = line.substr(0, colon_pos);
        std::string header_value = line.substr(colon_pos + 1);
        
        // Trim whitespace from header value
        while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t')) {
            header_value.erase(0, 1);
        }
        while (!header_value.empty() && (header_value[header_value.length()-1] == ' ' || header_value[header_value.length()-1] == '\t')) {
            header_value.erase(header_value.length()-1, 1);
        }
        
        std::cout << "🔐 Processing CGI header: " << header_name << ": " << header_value << std::endl;
        
        // Handle special CGI headers
        if (header_name == "Status") {
            // Parse status code and message
            size_t space_pos = header_value.find(' ');
            if (space_pos != std::string::npos) {
                status_code = std::atoi(header_value.substr(0, space_pos).c_str());
                status_message = header_value.substr(space_pos + 1);
            } else {
                status_code = std::atoi(header_value.c_str());
            }
            std::cout << "🔐 Set status: " << status_code << " " << status_message << std::endl;
        } 
        else if (header_name == "Content-Type" || header_name == "Content-type") {
            content_type = header_value;
            std::cout << "🔐 Set content-type: " << content_type << std::endl;
        } 
        else if (header_name == "Location") {
            // Handle redirect
            status_code = 302;
            status_message = "Found";
            request->GetClientDatat()->http_response->setHeader("Location", header_value);
            std::cout << "🔐 Set redirect location: " << header_value << std::endl;
        }
        // CRITICAL FIX: Enhanced Set-Cookie handling
        else if (header_name == "Set-Cookie") {
            std::cout << "🍪 [CRITICAL] Processing Set-Cookie from CGI: " << header_value << std::endl;
            request->GetClientDatat()->http_response->setHeader("Set-Cookie", header_value);
            std::cout << "🍪 [CRITICAL] Set-Cookie header added to response object" << std::endl;
        }
        // Handle other headers
        else {
            request->GetClientDatat()->http_response->setHeader(header_name, header_value);
            std::cout << "🔐 Set header: " << header_name << ": " << header_value << std::endl;
        }
    }
    
    // Set response status and content type
    request->GetClientDatat()->http_response->setStatusCode(status_code);
    request->GetClientDatat()->http_response->setStatusMessage(status_message);
    request->GetClientDatat()->http_response->setContentType(content_type);
    request->GetClientDatat()->http_response->setChunked(false);
    
    std::cout << "🔐 Final CGI response setup - Status: " << status_code << " " << status_message << std::endl;
    std::cout << "🔐 Final CGI response setup - Content-Type: " << content_type << std::endl;
    
    // Debug: Check if Set-Cookie header is in the response object
    std::string set_cookie_check = request->GetClientDatat()->http_response->getHeader("Set-Cookie");
    if (!set_cookie_check.empty()) {
        std::cout << "🍪 [VERIFICATION] Set-Cookie header in response object: " << set_cookie_check << std::endl;
    } else {
        std::cout << "🍪 [ERROR] No Set-Cookie header found in response object!" << std::endl;
    }
}

// to_string to be c++ 98 compatible
template <typename T>
std::string CgiHandler::to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}