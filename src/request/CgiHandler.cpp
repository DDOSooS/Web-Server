// ========================================
// 2. FIXED CgiHandler.cpp
// ========================================

#include "../include/request/CgiHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <errno.h>
#include <cstring>

CgiHandler::CgiHandler() : cgi_path("./www"), timeout_seconds(30) {}

CgiHandler::~CgiHandler() {
    for (auto& pair : active_processes) {
        cleanup(pair.first);
    }
}

bool CgiHandler::isCgiRequest(const std::string& path) const {
    std::string ext = getExt(path);
    return cgi_extensions.find(ext) != cgi_extensions.end();
}

int CgiHandler::startCgi(const std::string& method, const std::string& path, 
                        const std::string& query, const std::string& body, int client_fd) {
    
    // Build full path
    std::string full_path = cgi_path + path;
    if (!fileExists(full_path)) {
        std::cerr << "CGI not found: " << full_path << std::endl;
        return -1;
    }
    
    // Get interpreter
    std::string ext = getExt(path);
    auto it = cgi_extensions.find(ext);
    if (it == cgi_extensions.end()) return -1;
    std::string interpreter = it->second;
    
    // Create pipes
    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        perror("pipe");
        return -1;
    }
    
    // Fork
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return -1;
    }
    
    if (pid == 0) {
        // Child: setup and exec
        dup2(stdin_pipe[0], 0);
        dup2(stdout_pipe[1], 1);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        
        chdir(cgi_path.c_str());
        
        setenv("REQUEST_METHOD", method.c_str(), 1);
        setenv("QUERY_STRING", query.c_str(), 1);
        setenv("CONTENT_LENGTH", std::to_string(body.size()).c_str(), 1);
        setenv("SCRIPT_NAME", path.c_str(), 1);
        
        execl(interpreter.c_str(), interpreter.c_str(), full_path.c_str(), NULL);
        perror("execl");
        exit(1);
    }
    
    // Parent: setup process tracking
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    
    // Write POST data
    if (method == "POST" && !body.empty()) {
        write(stdin_pipe[1], body.c_str(), body.size());
    }
    close(stdin_pipe[1]);
    
    setNonBlock(stdout_pipe[0]);
    
    CgiProcess proc;
    proc.pid = pid;
    proc.stdout_fd = stdout_pipe[0];
    proc.client_fd = client_fd;
    proc.start_time = time(NULL);
    
    active_processes[stdout_pipe[0]] = proc;
    
    std::cout << "CGI started: PID=" << pid << " fd=" << stdout_pipe[0] << std::endl;
    return stdout_pipe[0];
}

bool CgiHandler::handleCgiOutput(int cgi_fd, int& client_fd_out, std::string& response_out) {
    auto it = active_processes.find(cgi_fd);
    if (it == active_processes.end()) return false;
    
    CgiProcess& proc = it->second;
    client_fd_out = proc.client_fd;
    
    // Read data
    std::string data = readCgi(cgi_fd);
    if (!data.empty()) {
        proc.output += data;
    }
    
    // Check if done
    int status;
    if (waitpid(proc.pid, &status, WNOHANG) > 0) {
        proc.completed = true;
        response_out = makeResponse(proc.output);
        cleanup(cgi_fd);
        return true;
    }
    
    return false;
}

std::vector<int> CgiHandler::getCgiFds() const {
    std::vector<int> fds;
    for (const auto& pair : active_processes) {
        if (!pair.second.completed) {
            fds.push_back(pair.second.stdout_fd);
        }
    }
    return fds;
}

void CgiHandler::cleanup(int cgi_fd) {
    auto it = active_processes.find(cgi_fd);
    if (it != active_processes.end()) {
        if (it->second.pid > 0) {
            kill(it->second.pid, SIGKILL);
            waitpid(it->second.pid, NULL, 0);
        }
        close(cgi_fd);
        active_processes.erase(it);
    }
}

void CgiHandler::killTimeouts() {
    time_t now = time(NULL);
    std::vector<int> to_kill;
    
    for (const auto& pair : active_processes) {
        if (now - pair.second.start_time > timeout_seconds) {
            to_kill.push_back(pair.first);
        }
    }
    
    for (int fd : to_kill) {
        cleanup(fd);
    }
}

// Private utilities
std::string CgiHandler::getExt(const std::string& path) const {
    size_t dot = path.find_last_of('.');
    return (dot == std::string::npos) ? "" : path.substr(dot);
}

bool CgiHandler::fileExists(const std::string& path) const {
    std::ifstream f(path);
    return f.good();
}

void CgiHandler::setNonBlock(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

std::string CgiHandler::readCgi(int fd) {
    std::string result;
    char buf[1024];
    ssize_t n;
    
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        result.append(buf, n);
    }
    
    return result;
}

std::string CgiHandler::makeResponse(const std::string& output) const {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    size_t split = output.find("\r\n\r\n");
    if (split == std::string::npos) split = output.find("\n\n");
    
    if (split != std::string::npos) {
        // Has headers
        std::string headers = output.substr(0, split);
        std::string body = output.substr(split + (output[split+1] == '\n' ? 2 : 4));
        
        response += headers + "\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
        response += body;
    } else {
        // No headers
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + std::to_string(output.size()) + "\r\n\r\n";
        response += output;
    }
    
    return response;
}
