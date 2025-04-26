#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Location.hpp"



// ConfigParser implementation
ConfigParser::ConfigParser(const std::string& file_name) : file_name_(file_name) {
    root_block_ = Block();
}
const std::vector<Block>& ConfigParser::get_servers() const{
    return (root_block_.nested_blocks);
}
bool ConfigParser::parse() {
    std::string content;
    if (!read_file_content(content)) {
        return false;
    }
    
    // Tokenize content
    content = "servers { "+ content + "}";
    std::vector<std::string> tokens = tokenize(content);
    process_tokens(tokens);
    
    std::vector<std::string>::iterator it = tokens.begin();
    root_block_ = parse_block(it, tokens.end());
    servers_ = get_servers();
    return true;
}

void ConfigParser::print_config() const {
    for (size_t i = 0; i < servers_.size(); ++i){
        std::cout << "--------------------SERVER [" << (i) << "]-----------------------------" << std::endl;
        print_block(servers_[i], 0);
    }
}

const Block& ConfigParser::get_root_block() const {
    return root_block_;
}

bool ConfigParser::read_file_content(std::string& content) {
    std::ifstream file(file_name_.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << file_name_ << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    
    return true;
}

std::vector<std::string> ConfigParser::tokenize(const std::string& content) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_comment = false;
    bool in_quotes = false;
    char c;
    
   for(size_t i = 0; i < content.length(); ++i){
        c = content[i];
        // Handle comments
        if (c == '#' && !in_quotes) { // start of comment #
            in_comment = true;
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
        if (in_comment && c == '\n') { // end of comment \n
            in_comment = false;
            continue;
        }
        if (in_comment) { // while still in comment move to next char
            continue;
        }
        
        // Handle quotes
        if (c == '"') {
            in_quotes = !in_quotes;
            current_token += c;
            continue;
        }

         // Handle special characters
        if (!in_quotes && (c == '{' || c == '}' || c == ';')) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            tokens.push_back(std::string(1, c));
            continue;
        }
        
        // Handle whitespace
        if (!in_quotes && std::isspace(c)) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
        
        // Add character to current token
        current_token += c;
    }
    
    // Add last token if exists
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    return tokens;
}


void ConfigParser::process_tokens(std::vector<std::string>& tokens) {
    // Validate balanced braces
    int brace_count = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "{") brace_count++;
        else if (tokens[i] == "}") brace_count--;
        
        if (brace_count < 0) {
            std::cerr << "Error: Unbalanced braces - closing brace without matching opening brace" << std::endl;
            tokens.clear();
            return;
        }
    }
    
    if (brace_count != 0) {
        std::cerr << "Error: Unbalanced braces - missing " << brace_count << " closing braces" << std::endl;
        tokens.clear();
        return;
    }
}

bool ConfigParser::check_if_directive(std::vector<std::string>::iterator it, 
                              const std::vector<std::string>::iterator& end) {
    std::vector<std::string>::iterator temp_it = it;

    while (temp_it != end) {
        if (*temp_it == ";") {
            return true;
        }
        if (*temp_it == "{") {
            return false;
        }
        ++temp_it;
    }
    return false;
}

Block ConfigParser::parse_block(std::vector<std::string>::iterator& it, 
                              const std::vector<std::string>::iterator& end) {
    std::string name;
    std::vector<std::string> parameters;
    
    // Parse block name and parameters
    if (it != end) {
        name = *it++;
    } else {
        Block b("", std::vector<std::string>());
        return b;
    }
    
    // Parse parameters until opening brace
    while (it != end && *it != "{") {
        parameters.push_back(*it++);
    }
    
    Block block(name, parameters);
    
    // Skip opening brace
    if (it != end && *it == "{") {
        ++it;
    } else {
        std::cerr << "Error: Expected '{' after block header" << std::endl;
        return block;
    }
    
    // Parse block contents
    while (it != end && *it != "}") {
        if (check_if_directive(it, end)) {
            block.directives.push_back(parse_directive(it, end));
        }
        else {
            block.nested_blocks.push_back(parse_block(it, end));
        }
    }

    // Skip closing brace
    if (it != end && *it == "}") {
        ++it;
    } else {
        std::cerr << "Error: Expected '}' at end of block" << std::endl;
    }
    
    return block;
}

Directive ConfigParser::parse_directive(std::vector<std::string>::iterator& it, 
                                     const std::vector<std::string>::iterator& end) {
    std::string name;
    std::vector<std::string> parameters;
    
    // Parse directive name
    if (it != end) {
        name = *it++;
    } else {
        Directive d("", std::vector<std::string>());
        return d;
    }
    
    // Parse parameters until semicolon
    while (it != end && *it != ";") {
        parameters.push_back(*it++);
    }
    
    // Skip semicolon
    if (it != end && *it == ";") {
        ++it;
    } else {
        std::cerr << "Error: Expected ';' at end of directive" << std::endl;
    }
    
    return Directive(name, parameters);
}




void print_block(const Block& block, int indent_level) {
    std::string indent(indent_level * 4, ' ');
    
    std::cout << indent << block.name;
    for (size_t i = 0; i < block.parameters.size(); ++i) {
        std::cout << " " << block.parameters[i];
    }
    std::cout << " {" << std::endl;
    
    for (size_t i = 0; i < block.directives.size(); ++i) {
        std::cout << indent << "  " << block.directives[i].name;
        for (size_t j = 0; j < block.directives[i].parameters.size(); ++j) {
            std::cout << " " << block.directives[i].parameters[j];
        }
        std::cout << ";" << std::endl;
    }
    
    for (size_t i = 0; i < block.nested_blocks.size(); ++i) {
        print_block(block.nested_blocks[i], indent_level + 1);
    }
    std::cout << indent << "}" << std::endl;
}




std::vector<Server> ConfigParser::create_servers() {
    std::vector<Server> servers;
    
    // Process each server block
    for (size_t i = 0; i < servers_.size(); ++i) {
        Server server;
        
        // Process server directives
        for (size_t j = 0; j < servers_[i].directives.size(); ++j) {
            const Directive& directive = servers_[i].directives[j];
            
            if (directive.name == "listen") {
                if (!directive.parameters.empty()) {
                    // Check if parameter contains a colon (host:port format)
                    std::string param = directive.parameters[0];
                    size_t colon_pos = param.find(':');
                    
                    if (colon_pos != std::string::npos) {
                        // Split into host and port
                        std::string host = param.substr(0, colon_pos);
                        std::string port = param.substr(colon_pos + 1);
                        server.set_host(host);
                        server.set_port(port);
                    } else {
                        // Just port, use default host
                        server.set_port(param);
                    }
                }
            }
            else if (directive.name == "host") {
                if (!directive.parameters.empty()) {
                    server.set_host(directive.parameters[0]);
                }
            }
            else if (directive.name == "server_name") {
                if (!directive.parameters.empty()) {
                    server.set_server_name(directive.parameters[0]);
                }
            }
            else if (directive.name == "root") {
                if (!directive.parameters.empty()) {
                    server.set_root(directive.parameters[0]);
                }
            }
            else if (directive.name == "index") {
                if (!directive.parameters.empty()) {
                    server.set_index(directive.parameters[0]);
                }
            }
            else if (directive.name == "autoindex") {
                if (!directive.parameters.empty()) {
                    server.set_autoindex(directive.parameters[0]);
                }
            }
            else if (directive.name == "client_max_body_size") {
                if (!directive.parameters.empty()) {
                    server.set_client_max_body_size(directive.parameters[0]);
                }
            }
            else if (directive.name == "error_page") {
                if (directive.parameters.size() >= 2) {
                    server.set_error_pages(directive.parameters[0], directive.parameters[1]);
                }
            }
        }
        
        // Process location blocks
        for (size_t j = 0; j < servers_[i].nested_blocks.size(); ++j) {
            if (servers_[i].nested_blocks[j].name == "location") {
                Location location(servers_[i].nested_blocks[j]);
                server.add_location(location);
            }
        }
        
        // Setup server address
        server.set_server_address();
        
        servers.push_back(server);
    }
    
    return servers;
}

bool ConfigParser::validate_config() {
    // For each server block
    for (size_t i = 0; i < servers_.size(); ++i) {
        // Check required directives for server
        if (!validate_server_block(servers_[i])) {
            return false;
        }
        
        // Check each location block
        for (size_t j = 0; j < servers_[i].nested_blocks.size(); ++j) {
            if (servers_[i].nested_blocks[j].name == "location") {
                if (!validate_location_block(servers_[i].nested_blocks[j])) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ConfigParser::validate_server_block(const Block& server) {
    bool has_listen = false;
    
    // Check all directives in server block
    for (size_t i = 0; i < server.directives.size(); ++i) {
        const Directive& directive = server.directives[i];
        
        if (directive.name == "listen") {
            has_listen = true;
            // Validate listen directive parameters
            if (directive.parameters.empty()) {
                std::cerr << "Error: listen directive requires parameters" << std::endl;
                return false;
            }
            
            // Check if parameter contains a valid port number
            std::string param = directive.parameters[0];
            size_t colon_pos = param.find(':');
            std::string port_str;
            
            if (colon_pos != std::string::npos) {
                // Split into host and port
                port_str = param.substr(colon_pos + 1);
            } else {
                // Just port
                port_str = param;
            }
            
            // Validate port number
            const char* cstr = port_str.c_str();
            char* endptr;
            unsigned long port = strtoul(cstr, &endptr, 10);
            if (*endptr != '\0' || port < 1 || port > 65535) {
                std::cerr << "Error: Invalid port number: " << port_str << std::endl;
                return false;
            }
        } 
        else if (directive.name == "host") {
            // Validate host directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: host directive requires a parameter" << std::endl;
                return false;
            }
            
            // Validate IP address format
            struct in_addr addr;
            if (inet_aton(directive.parameters[0].c_str(), &addr) == 0) {
                std::cerr << "Error: Invalid IP address format: " << directive.parameters[0] << std::endl;
                return false;
            }
        }
        else if (directive.name == "root") {
            // Validate root directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: root directive requires exactly one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "server_name") {
            // Validate server_name directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: server_name directive requires at least one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "client_max_body_size") {
            // Validate client_max_body_size directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: client_max_body_size requires exactly one parameter" << std::endl;
                return false;
            }
            
            // Check if parameter is a valid number
            std::string size_str = directive.parameters[0];
            size_t len = size_str.length();
            char unit = 'B';
            
            // Check for K, M, G units
            if (len > 0 && std::isalpha(size_str[len - 1])) {
                unit = std::toupper(size_str[len - 1]);
                size_str = size_str.substr(0, len - 1);
            }
            
            const char* cstr = size_str.c_str();
            char* endptr;
            unsigned long result = strtoul(cstr, &endptr, 10);
            if (*endptr != '\0') {
                std::cerr << "Error: client_max_body_size invalid number format: " << directive.parameters[0] << std::endl;
                return false;
            }
            
            // Check unit
            if (unit != 'B' && unit != 'K' && unit != 'M' && unit != 'G') {
                std::cerr << "Error: client_max_body_size invalid unit: " << unit << std::endl;
                return false;
            }
        }
        else if (directive.name == "error_page") {
            // Validate error_page directive
            if (directive.parameters.size() < 2) {
                std::cerr << "Error: error_page directive requires at least two parameters" << std::endl;
                return false;
            }
            
            // Validate error code
            const char* cstr = directive.parameters[0].c_str();
            char* endptr;
            int code = strtol(cstr, &endptr, 10);
            if (*endptr != '\0' || code < 100 || code > 599) {
                std::cerr << "Error: Invalid error code: " << directive.parameters[0] << std::endl;
                return false;
            }
        }
        else if (directive.name == "autoindex") {
            // Validate autoindex directive
            if (directive.parameters.size() != 1 || 
                (directive.parameters[0] != "on" && directive.parameters[0] != "off")) {
                std::cerr << "Error: autoindex directive requires 'on' or 'off'" << std::endl;
                return false;
            }
        }
        else if (directive.name == "index") {
            // Validate index directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: index directive requires at least one parameter" << std::endl;
                return false;
            }
        }
        else {
            // Unknown directive
            std::cerr << "Warning: Unknown directive in server context: " << directive.name << std::endl;
        }
    }
    
    // Check if server has required directives
    if (!has_listen) {
        std::cerr << "Error: server block must have a listen directive" << std::endl;
        return false;
    }
    
    return true;
}

bool ConfigParser::validate_location_block(const Block& location) {
    // Check location block has a path parameter
    if (location.parameters.empty()) {
        std::cerr << "Error: location block requires a path parameter" << std::endl;
        return false;
    }
    
    // Validate directives in location block
    for (size_t i = 0; i < location.directives.size(); ++i) {
        const Directive& directive = location.directives[i];
        
        if (directive.name == "root") {
            // Validate root directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: root directive requires exactly one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "autoindex") {
            // Validate autoindex directive
            if (directive.parameters.size() != 1 || 
                (directive.parameters[0] != "on" && directive.parameters[0] != "off")) {
                std::cerr << "Error: autoindex directive requires 'on' or 'off'" << std::endl;
                return false;
            }
        }
        else if (directive.name == "index") {
            // Validate index directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: index directive requires at least one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "allow_methods") {
            // Validate allow_methods directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: allow_methods requires at least one method" << std::endl;
                return false;
            }
            
            // Validate each method
            for (size_t j = 0; j < directive.parameters.size(); ++j) {
                const std::string& method = directive.parameters[j];
                if (method != "GET" && method != "POST" && method != "DELETE" && 
                    method != "PUT" && method != "HEAD") {
                    std::cerr << "Error: invalid HTTP method in allow_methods: " << method << std::endl;
                    return false;
                }
            }
        }
        else if (directive.name == "return") {
            // Validate return directive
            if (directive.parameters.size() < 2) {
                std::cerr << "Error: return directive requires at least two parameters" << std::endl;
                return false;
            }
            
            // Validate redirect code
            const char* cstr = directive.parameters[0].c_str();
            char* endptr;
            int code = strtol(cstr, &endptr, 10);
            if (*endptr != '\0' || (code != 301 && code != 302 && code != 303 && code != 307 && code != 308)) {
                std::cerr << "Error: Invalid redirection code: " << directive.parameters[0] << std::endl;
                return false;
            }
        }
        else if (directive.name == "client_max_body_size") {
            // Validate client_max_body_size directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: client_max_body_size requires exactly one parameter" << std::endl;
                return false;
            }
            
            // Check if parameter is a valid number
            std::string size_str = directive.parameters[0];
            size_t len = size_str.length();
            char unit = 'B';
            
            // Check for K, M, G units
            if (len > 0 && std::isalpha(size_str[len - 1])) {
                unit = std::toupper(size_str[len - 1]);
                size_str = size_str.substr(0, len - 1);
            }
            
            const char* cstr = size_str.c_str();
            char* endptr;
            unsigned long result = strtoul(cstr, &endptr, 10);
            if (*endptr != '\0') {
                std::cerr << "Error: client_max_body_size invalid number format: " << directive.parameters[0] << std::endl;
                return false;
            }
            
            // Check unit
            if (unit != 'B' && unit != 'K' && unit != 'M' && unit != 'G') {
                std::cerr << "Error: client_max_body_size invalid unit: " << unit << std::endl;
                return false;
            }
        }
        else if (directive.name == "cgi_extension") {
            // Validate cgi_extension directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: cgi_extension requires at least one parameter" << std::endl;
                return false;
            }
            
            // Validate each extension
            for (size_t j = 0; j < directive.parameters.size(); ++j) {
                const std::string& ext = directive.parameters[j];
                if (ext.empty() || ext[0] != '.') {
                    std::cerr << "Error: invalid CGI extension: " << ext << std::endl;
                    return false;
                }
            }
        }
        else if (directive.name == "cgi_path") {
            // Validate cgi_path directive
            if (directive.parameters.empty()) {
                std::cerr << "Error: cgi_path requires at least one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "upload_store") {
            // Validate upload_store directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: upload_store requires exactly one parameter" << std::endl;
                return false;
            }
        }
        else if (directive.name == "alias") {
            // Validate alias directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: alias directive requires exactly one parameter" << std::endl;
                return false;
            }
        }
        else {
            // Unknown directive
            std::cerr << "Warning: Unknown directive in location context: " << directive.name << std::endl;
        }
    }
    
    return true;
}