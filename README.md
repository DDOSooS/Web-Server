# 🌐 WebServ

A high-performance, multi-server HTTP/1.1 web server implementation written in C++98, compliant with 42 Network standards. This project demonstrates advanced system programming concepts including socket programming, event-driven I/O, HTTP protocol handling, and CGI execution.

![42](https://img.shields.io/badge/42-Network-black?style=flat-square&logo=42)
![C++](https://img.shields.io/badge/C++-98-blue?style=flat-square&logo=c%2B%2B)
![HTTP](https://img.shields.io/badge/HTTP-1.1-green?style=flat-square)
![CGI](https://img.shields.io/badge/CGI-Enabled-orange?style=flat-square)

## ✨ Features

### 🔧 Core Functionality
- **Multi-server support**: Host multiple virtual servers on different ports
- **Event-driven I/O**: Non-blocking architecture using `poll()` for optimal performance
- **HTTP/1.1 compliance**: Full support for HTTP methods (GET, POST, DELETE)
- **Configuration-based**: Nginx-style configuration file parsing
- **Error handling**: Comprehensive HTTP status code handling (4xx, 5xx)

### 🚀 Advanced Features
- **CGI execution**: Support for Python and shell script CGI programs
- **File upload handling**: Configurable upload directory and size limits
- **Directory listing**: Automatic index generation (autoindex)
- **URL redirection**: HTTP 301/302 redirects support
- **Multiple location blocks**: Fine-grained path-based configuration
- **Custom error pages**: Configurable error page templates

## 📁 Project Structure

```
Web-Server/
├── include/                    # Header files
│   ├── WebServer.hpp          # Main server class
│   ├── ClientConnection.hpp   # Client connection handling
│   ├── config/                # Configuration parsing
│   │   ├── ConfigParser.hpp
│   │   ├── ServerConfig.hpp
│   │   └── Location.hpp
│   ├── request/               # HTTP request handling
│   │   ├── HttpRequest.hpp
│   │   ├── RequestHandler.hpp
│   │   └── CgiHandler.hpp
│   ├── response/              # HTTP response generation
│   │   └── HttpResponse.hpp
│   └── error/                 # Error handling classes
├── src/                       # Source implementation
│   ├── config/                # Configuration parsing logic
│   ├── request/               # Request processing
│   ├── response/              # Response generation
│   └── error/                 # Error handling
├── www/                       # Web document root
│   ├── index.html            # Default index page
│   ├── cgi-bin/              # CGI scripts
│   │   └── main.py           # Python CGI example
│   └── error pages (404.html, 500.html, etc.)
├── Makefile                   # Build configuration
├── default.config            # Default server configuration
└── servers.config            # Multi-server configuration
```

## 🛠️ Installation & Usage

### Prerequisites
- C++ compiler with C++98 support
- Make build system
- Python3 (for CGI scripts)

### Building
```bash
# Clone the repository
git clone <repository-url>
cd Web-Server

# Compile the project
make

# Clean build files
make clean      # Remove object files
make fclean     # Remove all generated files
make re         # Full rebuild
```

### Configuration

The server uses Nginx-style configuration files. Example configuration:

```nginx
server {
    listen 127.0.0.1:8080;
    server_name example.com;
    root www;
    index index.html;
    
    # Error pages
    error_page 404 /404.html;
    error_page 500 502 503 504 /50x.html;
    
    # Default location
    location / {
        allow_methods GET POST;
        autoindex on;
    }
    
    # CGI handling
    location /cgi-bin/ {
        cgi_path /usr/bin/python3 /bin/bash;
        cgi_extension .py .cgi;
        allow_methods GET POST;
    }
    
    # File uploads
    location /upload {
        allow_methods GET POST DELETE;
        client_max_body_size 1M;
        upload_store www/uploads;
    }
}
```

### Running the Server

```bash
# Use default configuration
./webserver

# Specify custom configuration file
./webserver custom.config

# Test configuration without starting server
./webserver -t default.config
./webserver --test servers.config
```

## 🔧 Configuration Directives

### Server Block
| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | IP address and port | `listen 127.0.0.1:8080;` |
| `server_name` | Virtual host name | `server_name example.com;` |
| `root` | Document root directory | `root www;` |
| `index` | Default index files | `index index.html index.htm;` |
| `error_page` | Custom error pages | `error_page 404 /404.html;` |

### Location Block
| Directive | Description | Example |
|-----------|-------------|---------|
| `allow_methods` | Allowed HTTP methods | `allow_methods GET POST DELETE;` |
| `autoindex` | Enable directory listing | `autoindex on;` |
| `return` | HTTP redirect | `return 301 /new-page;` |
| `cgi_path` | CGI interpreter paths | `cgi_path /usr/bin/python3;` |
| `cgi_extension` | CGI file extensions | `cgi_extension .py .cgi;` |
| `client_max_body_size` | Max request body size | `client_max_body_size 1M;` |
| `upload_store` | Upload directory | `upload_store www/uploads;` |

## 🌐 HTTP Features

### Supported Methods
- **GET**: Retrieve resources, directory listings
- **POST**: Form submissions, CGI execution, file uploads
- **DELETE**: File deletion (when configured)

### Status Code Handling
- **2xx Success**: 200 OK, 201 Created
- **3xx Redirection**: 301 Moved Permanently, 302 Found
- **4xx Client Error**: 400 Bad Request, 403 Forbidden, 404 Not Found, 405 Method Not Allowed, 413 Request Entity Too Large
- **5xx Server Error**: 500 Internal Server Error, 501 Not Implemented, 502 Bad Gateway

### CGI Support
- Environment variable passing (REQUEST_METHOD, QUERY_STRING, etc.)
- POST data handling through stdin
- Response header parsing
- Timeout management
- Support for Python, shell scripts, and other interpreters

## 🧪 Testing

### Manual Testing
```bash
# Basic GET request
curl http://localhost:8080/

# POST request with data
curl -X POST -d "name=value" http://localhost:8080/cgi-bin/main.py

# File upload
curl -X POST -F "file=@test.txt" http://localhost:8080/upload/

# Directory listing
curl http://localhost:8080/files/

# Custom error page
curl http://localhost:8080/nonexistent
```

### Configuration Testing
```bash
# Test configuration syntax
./webserver --test default.config

# Validate all configurations
./webserver -t servers.config
```

## 📚 Technical Implementation

### Architecture Overview
- **Event-driven**: Uses `poll()` system call for non-blocking I/O
- **Multi-server**: Single process handles multiple virtual servers
- **Modular design**: Separate classes for request/response handling
- **Memory safe**: RAII principles and proper resource management

### Key Classes
- `WebServer`: Main server orchestrator and connection manager
- `ConfigParser`: Nginx-style configuration file parser with validation
- `ServerConfig`: Server configuration container with location blocks
- `ClientConnection`: Individual client connection state management
- `RequestHandler`: HTTP request parsing and routing logic
- `CgiHandler`: CGI script execution and communication
- `HttpResponse`: HTTP response generation and formatting

### Performance Features
- Non-blocking socket operations
- Efficient poll-based event loop
- Connection pooling and reuse
- Optimized buffer management
- Configurable timeout handling

## 🐛 Error Handling

The server implements comprehensive error handling:
- **Configuration errors**: Invalid syntax, missing files, conflicting directives
- **Runtime errors**: Socket failures, permission issues, resource exhaustion
- **HTTP errors**: Malformed requests, unsupported methods, timeout conditions
- **CGI errors**: Script failures, execution timeouts, invalid responses

Custom error pages can be configured for different HTTP status codes, providing a better user experience.

## 🔒 Security Considerations

- Input validation for all HTTP headers and data
- Path traversal protection (prevents `../` attacks)
- Configurable request size limits
- CGI execution sandboxing
- Error message sanitization
- Resource limit enforcement

## 👥 Author

**42 Network Student Project**

This project is part of the 42 Network curriculum, designed to teach advanced C++ programming, network programming, and web server architecture.

---