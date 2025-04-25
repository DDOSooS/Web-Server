// ConfigParser.cpp
#include "ConfigParser.hpp"



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
        
        // Handle comment
        if (c == '#' && !in_quotes) {
            in_comment = true;
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
         // End of comment
        if (in_comment && c == '\n') {
            in_comment = false;
            continue;
        }
        
        if (in_comment) {
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
    // std::cout << "<";
    // for(std::string tok:tokens){
    //     std::cout << tok << "> <";
    // }
    // std::cout << ">" << std::endl;
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
        } 
        else if (directive.name == "root") {
            // Validate root directive
            if (directive.parameters.size() != 1) {
                std::cerr << "Error: root directive requires exactly one parameter" << std::endl;
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
            const char* cstr = directive.parameters[0].c_str();
            char* endptr;
            unsigned long result = strtoul(cstr, &endptr, 10);
            if (*endptr != '\0') {
                std::cerr << "Error: client_max_body_size requires a number" << std::endl;
                return false;
            }
        }
        // Add validation for other server directives
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
        
        if (directive.name == "autoindex") {
            // Validate autoindex directive
            if (directive.parameters.size() != 1 || 
                (directive.parameters[0] != "on" && directive.parameters[0] != "off")) {
                std::cerr << "Error: autoindex directive requires 'on' or 'off'" << std::endl;
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
        // Add validation for other location directives
    }
    
    return true;
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
                    server.set_port(directive.parameters[0]);
                }
            }
            else if (directive.name == "server_name") {
                if (!directive.parameters.empty()) {
                    server.set_server_name(directive.parameters[0]);
                }
            }
            // Process other directives...
        }
        
        // Process location blocks
        for (size_t j = 0; j < servers_[i].nested_blocks.size(); ++j) {
            if (servers_[i].nested_blocks[j].name == "location") {
                Location location;
                
                // Set location path
                if (!servers_[i].nested_blocks[j].parameters.empty()) {
                    location.set_path(servers_[i].nested_blocks[j].parameters[0]);
                }
                
                // Process location directives
                for (size_t k = 0; k < servers_[i].nested_blocks[j].directives.size(); ++k) {
                    const Directive& directive = servers_[i].nested_blocks[j].directives[k];
                    
                    if (directive.name == "autoindex") {
                        if (!directive.parameters.empty()) {
                            location.set_autoindex(directive.parameters[0]);
                        }
                    }
                    else if (directive.name == "allow_methods") {
                        location.set_allowMethods(directive.parameters);
                    }
                    // Process other location directives...
                }
                
                // Add the location to the server
                server._locations.push_back(location);
            }
        }
        
        servers.push_back(server);
    }
    
    return servers;
}