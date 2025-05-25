#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctime>

class CgiHandler {
public:
    struct CgiProcess {
        pid_t pid;
        int stdout_fd;
        int client_fd;
        time_t start_time;
        std::string output;
        bool completed;
        
        CgiProcess() : pid(-1), stdout_fd(-1), client_fd(-1), start_time(0), completed(false) {}
    };

private:
    std::map<std::string, std::string> cgi_extensions; // .py -> /usr/bin/python3
    std::map<int, CgiProcess> active_processes;        // stdout_fd -> process
    std::string cgi_path;
    int timeout_seconds;

public:
    CgiHandler();
    ~CgiHandler();
    
    // Config methods (for backward compatibility with WebServer.cpp)
    void setCgiPath(const std::string& path) { cgi_path = path; }
    void addCgiExtension(const std::string& ext, const std::string& interpreter) { 
        cgi_extensions[ext] = interpreter; 
    }
    
    // Main interface
    bool isCgiRequest(const std::string& path) const;
    int startCgi(const std::string& method, const std::string& path, const std::string& query, 
                const std::string& body, int client_fd);
    bool handleCgiOutput(int cgi_fd, int& client_fd_out, std::string& response_out);
    
    // Poll integration
    std::vector<int> getCgiFds() const;
    bool isCgiFd(int fd) const { return active_processes.find(fd) != active_processes.end(); }
    void cleanup(int cgi_fd);
    void killTimeouts();
    
    // Status info
    size_t getActiveCount() const { return active_processes.size(); }

private:
    std::string getExt(const std::string& path) const;
    bool fileExists(const std::string& path) const;
    void setNonBlock(int fd);
    std::string readCgi(int fd);
    std::string makeResponse(const std::string& cgi_output) const;
};

#endif