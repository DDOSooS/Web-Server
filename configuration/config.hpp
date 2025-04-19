#include <string>
#include <vector>
#include <map>
#include <optional>

struct Directive {
    std::string name;                  // Directive name (e.g., "listen", "root")
    std::vector<std::string> parameters;   // List of values (e.g., {"80"} or {"example.com", "www.example.com"})

    Directive(const std::string& n, const std::vector<std::string>& v = {}) : name(n), parameters(v) {}
} ;

// Represents a generic block (e.g., "server {...}", "location / {...}")
struct Block {
    std::string name;                          // Block name (e.g., "server", "location")
    std::vector<std::string> parameters;       // Block parameters (e.g., "/" for "location /")
    std::vector<Directive> directives;         // List of directives inside the block
    std::vector<Block> nested_blocks;          // List of nested blocks

    Block(const std::string& name, const std::vector<std::string>& params = {})
        : name(name), parameters(params) {}
};

 struct Listen {
        std::string address;           // e.g., "127.0.0.1" or "0.0.0.0"
        int port;                      // e.g., 80
        std::vector<std::string> flags; // e.g., {"ssl", "http2"}
        bool default_server;            // true if "default_server" is set
    };