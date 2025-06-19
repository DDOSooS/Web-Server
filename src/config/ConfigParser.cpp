#include "config/ConfigParser.hpp"
#include "config/ServerConfig.hpp"
#include "config/Location.hpp"

ValidationError::ValidationError(ErrorLevel level, const std::string& message, int line, const std::string& context)
    : _level(level), _message(message), _line(line), _context(context) {}

void ValidationError::print() const {
    std::string level_str = errorLevelToString(_level);
    std::string context_info = _context.empty() ? "" : " in " + _context + " context";
    
    std::string line_info;
    if (_line > 0) {
        std::ostringstream oss;
        oss << _line;
        line_info = " on line[" + oss.str() + "]";
    } else {
        line_info = "";
    }
    
    std::cerr << level_str << ":" << line_info << context_info << " " << _message << std::endl;
}

std::string ValidationError::errorLevelToString(ErrorLevel level) {
    switch (level) {
        case WARNING: return "warning";
        case ERROR: return "error";
        case CRITICAL: return "critical";
        default: return "unknown";
    }
}

ValidationError::ErrorLevel ValidationError::getLevel() const {
    return _level;
}

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
    
    content = "servers { "+ content + "}";
    std::vector<std::string> tokens = tokenize(content);
    process_tokens(tokens);
    
    std::vector<std::string>::iterator it = tokens.begin();
    root_block_ = parse_block(it, tokens.end());
    servers_ = get_servers();
    return true;
}

bool ConfigParser::test_config() {
    if (!parse()) {
        std::cerr << "Failed to parse configuration file." << std::endl;
        return false;
    }
    
    bool is_valid = validate_config();
    
    return is_valid;
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
    int line_number = 1;
    char c;
    
    for(size_t i = 0; i < content.length(); ++i){
        c = content[i];
        
        if (c == '\n') {
            line_number++;
        }
        
        if (c == '#' && !in_quotes) {
            in_comment = true;
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                token_line_numbers_[current_token] = line_number;
                current_token.clear();
            }
            continue;
        }
        if (in_comment && c == '\n') {
            in_comment = false;
            continue;
        }
        if (in_comment) {
            continue;
        }
        
        if (c == '"') {
            if (!in_quotes) {
                in_quotes = true;
            } else {
                in_quotes = false;
                tokens.push_back(current_token);
                token_line_numbers_[current_token] = line_number;
                current_token.clear();
            }
            continue;
        }

        if (!in_quotes && (c == '{' || c == '}' || c == ';')) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                token_line_numbers_[current_token] = line_number;
                current_token.clear();
            }
            std::string special(1, c);
            tokens.push_back(special);
            token_line_numbers_[special] = line_number;
            continue;
        }
        
        if (!in_quotes && std::isspace(c)) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                token_line_numbers_[current_token] = line_number;
                current_token.clear();
            }
            continue;
        }
        
        current_token += c;
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
        token_line_numbers_[current_token] = line_number;
    }
    
    if (in_quotes) {
        addError(ValidationError::ERROR, "Unclosed quoted string", line_number, "");
    }
    
    return tokens;
}

void ConfigParser::process_tokens(std::vector<std::string>& tokens) {
    int brace_count = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "{") brace_count++;
        else if (tokens[i] == "}") brace_count--;
        
        if (brace_count < 0) {
            addError(ValidationError::ERROR, "Unbalanced braces - closing brace without matching opening brace", 
                    getTokenLine(tokens[i]), "");
            tokens.clear();
            return;
        }
    }
    
    if (brace_count != 0) {
        addError(ValidationError::ERROR, "Unbalanced braces - missing closing braces", -1, "");
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
    
    if (it != end) {
        name = *it++;
    } else {
        Block b("", std::vector<std::string>());
        return b;
    }
    
    while (it != end && *it != "{") {
        parameters.push_back(*it++);
    }
    
    Block block(name, parameters);
    
    if (it != end && *it == "{") {
        ++it;
    } else {
        addError(ValidationError::ERROR, "Expected '{' after block header", 
                getTokenLine(*(it-1)), name);
        return block;
    }
    
    while (it != end && *it != "}") {
        if (check_if_directive(it, end)) {
            block.directives.push_back(parse_directive(it, end));
        }
        else {
            block.nested_blocks.push_back(parse_block(it, end));
        }
    }

    if (it != end && *it == "}") {
        ++it;
    } else {
        addError(ValidationError::ERROR, "Expected '}' at end of block", -1, name);
    }
    
    return block;
}

Directive ConfigParser::parse_directive(std::vector<std::string>::iterator& it, 
                                     const std::vector<std::string>::iterator& end) {
    std::string name;
    std::vector<std::string> parameters;
    
    if (it != end) {
        name = *it++;
    } else {
        Directive d("", std::vector<std::string>());
        return d;
    }
    
    while (it != end && *it != ";") {
        parameters.push_back(*it++);
    }
    
    if (it != end && *it == ";") {
        ++it;
    } else {
        addError(ValidationError::ERROR, "Expected ';' at end of directive", 
                getTokenLine(*(it-1)), "");
    }
    return Directive(name, parameters);
}

void ConfigParser::addError(ValidationError::ErrorLevel level, const std::string& message, int line, const std::string& context) {
    _errors.push_back(ValidationError(level, message, line, context));
}

bool ConfigParser::validateDirective(const Directive& directive, const std::string& context, 
                                   const std::vector<std::string>& allowed_directives) {
    bool allowed = false;
    for (size_t i = 0; i < allowed_directives.size(); ++i) {
        if (directive.name == allowed_directives[i]) {
            allowed = true;
            break;
        }
    }
    
    if (!allowed) {
        addError(ValidationError::ERROR, "Unknown directive \"" + directive.name + "\"", 
                getTokenLine(directive.name), context);
        return false;
    }
    if (directive.parameters.empty()) {
        addError(ValidationError::ERROR, "Directive \"" + directive.name + "\" requires parameters", 
                getTokenLine(directive.name), context);
        return false;
    }
    for (size_t i = 0; i < directive.parameters.size(); ++i) {
        if (directive.parameters[i].empty()) {
            addError(ValidationError::ERROR, "Directive \"" + directive.name + "\" parameter cannot be empty", 
                    getTokenLine(directive.name), context);
            return false;
        }
    }
    return true;
}

bool ConfigParser::printValidationResults() const {
    if (_errors.empty()) {
        std::cout << "Configuration file syntax is ok" << std::endl;
    } else {
        int error_count = 0;
        int warning_count = 0;
        
        for (size_t i = 0; i < _errors.size(); ++i) {
            ValidationError::ErrorLevel level = _errors[i].getLevel();
            if (level == ValidationError::ERROR || level == ValidationError::CRITICAL) {
                error_count++;
            } else if (level == ValidationError::WARNING) {
                warning_count++;
            }
        }
        
        std::cerr << "Configuration file test failed" << std::endl;
        std::cerr << error_count << " errors and " << warning_count << " warnings found" << std::endl;
    }
    if (_errors.empty()) {
        return true;
    }
    return false;
}

int ConfigParser::getTokenLine(const std::string& token) {
    std::map<std::string, int>::iterator it = token_line_numbers_.find(token);
    if (it != token_line_numbers_.end()) {
        return it->second;
    }
    return -1;
}

bool ConfigParser::validate_config() {
    bool valid = true;
    _errors.clear();
    
    for (size_t i = 0; i < servers_.size(); ++i) {
        if (!validate_server_block(servers_[i])) {
            valid = false;
        }
        
        for (size_t j = 0; j < servers_[i].nested_blocks.size(); ++j) {
            if (servers_[i].nested_blocks[j].name == "location") {
                if (!validate_location_block(servers_[i].nested_blocks[j])) {
                    valid = false;
                }
            } else if (servers_[i].nested_blocks[j].name != "location") {
                addError(ValidationError::ERROR, 
                        "Unexpected block \"" + servers_[i].nested_blocks[j].name + "\" in server context", 
                        getTokenLine(servers_[i].nested_blocks[j].name), "server");
                valid = false;
            }
        }
    }
    
    for (size_t i = 0; i < _errors.size(); ++i) {
        _errors[i].print();
    }
    
    valid = printValidationResults();
    
    return valid;
}

bool ConfigParser::validate_server_block(const Block& server) {
    std::vector<std::string> ALLOWED_DIRECTIVES;
    ALLOWED_DIRECTIVES.push_back("listen");
    ALLOWED_DIRECTIVES.push_back("server_name");
    ALLOWED_DIRECTIVES.push_back("root");
    ALLOWED_DIRECTIVES.push_back("client_max_body_size");
    ALLOWED_DIRECTIVES.push_back("error_page");
    ALLOWED_DIRECTIVES.push_back("autoindex");
    ALLOWED_DIRECTIVES.push_back("index");
    
    bool valid = true;
    bool has_listen = false;
    
    if (server.name != "server") {
        addError(ValidationError::ERROR, "Unexpected block \"" + server.name + "\"", 
                getTokenLine(server.name), "http");
        return false;
    }
    
    // C++98 style loop
    for (size_t i = 0; i < server.directives.size(); ++i) {
        const Directive& directive = server.directives[i];
        
        validateDirective(directive, "server", ALLOWED_DIRECTIVES);
        
        if (directive.name == "listen") {
            has_listen = true;
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "listen directive requires parameters", 
                    getTokenLine(directive.name), "server");
                    valid = false;
                    continue;
                }
            if (directive.parameters.size() > 1) {
                    addError(ValidationError::ERROR, " \'" + directive.parameters[1] + "\'", 
                        getTokenLine(directive.name), "server");
                    valid = false;
                    continue;
            }
                std::string param = directive.parameters[0];
                size_t colon_pos = param.find(':');
            std::string port_str;
            
            if (colon_pos != std::string::npos) {
                port_str = param.substr(colon_pos + 1);
            } else {
                port_str = param;
            }
            const char* cstr = port_str.c_str();
            char* endptr;
            unsigned long port = strtoul(cstr, &endptr, 10);
            if (*endptr != '\0' || port < 1 || port > 65535) {
                addError(ValidationError::ERROR, "Invalid port number: " + port_str, 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        } 
        else if (directive.name == "root") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "root directive requires exactly one parameter", 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
        else if (directive.name == "server_name") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "server_name directive requires at least one parameter", 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
        else if (directive.name == "client_max_body_size") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "client_max_body_size requires exactly one parameter", 
                        getTokenLine(directive.name), "server");
                valid = false;
                continue;
            }
            
            std::string size_str = directive.parameters[0];
            size_t len = size_str.length();
            char unit = 'B';
            
            if (len > 0 && std::isalpha(size_str[len - 1])) {
                unit = std::toupper(size_str[len - 1]);
                size_str = size_str.substr(0, len - 1);
            }
            
            const char* cstr = size_str.c_str();
            char* endptr;
            strtoul(cstr, &endptr, 10);
            if (*endptr != '\0') {
                addError(ValidationError::ERROR, "client_max_body_size invalid number format: " + directive.parameters[0], 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
            
            if (unit != 'B' && unit != 'K' && unit != 'M' && unit != 'G') {
                addError(ValidationError::ERROR, "client_max_body_size invalid unit: " + std::string(1, unit), 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
        else if (directive.name == "error_page") {

            if (directive.parameters.size() < 2) {
                addError(ValidationError::ERROR, "error_page directive requires at least two parameters", 
                        getTokenLine(directive.name), "server");
                valid = false;
                continue;
            }
            
            const char* cstr = directive.parameters[0].c_str();
            char* endptr;
            int code = strtol(cstr, &endptr, 10);
            if (*endptr != '\0' || code < 100 || code > 599) {
                addError(ValidationError::ERROR, "Invalid error code: " + directive.parameters[0], 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
        else if (directive.name == "autoindex") {
            if (directive.parameters.size() != 1 || 
                (directive.parameters[0] != "on" && directive.parameters[0] != "off")) {
                addError(ValidationError::ERROR, "autoindex directive requires 'on' or 'off'", 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
        else if (directive.name == "index") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "index directive requires at least one parameter", 
                        getTokenLine(directive.name), "server");
                valid = false;
            }
        }
    }
    
    if (!has_listen) {
        addError(ValidationError::ERROR, "server block must have a listen directive", 
                getTokenLine(server.name), "server");
        valid = false;
    }
    
    return valid;
}

bool ConfigParser::validate_location_block(const Block& location) {
    std::vector<std::string> ALLOWED_DIRECTIVES;
    ALLOWED_DIRECTIVES.push_back("root");
    ALLOWED_DIRECTIVES.push_back("autoindex");
    ALLOWED_DIRECTIVES.push_back("index");
    ALLOWED_DIRECTIVES.push_back("allow_methods");
    ALLOWED_DIRECTIVES.push_back("return");
    ALLOWED_DIRECTIVES.push_back("client_max_body_size");
    ALLOWED_DIRECTIVES.push_back("cgi_extension");
    ALLOWED_DIRECTIVES.push_back("cgi_path");
    ALLOWED_DIRECTIVES.push_back("upload_store");
    ALLOWED_DIRECTIVES.push_back("alias");
    
    bool valid = true;
    
    if (location.parameters.empty()) {
        addError(ValidationError::ERROR, "location block requires a path parameter", 
                getTokenLine(location.name), "location");
        valid = false;
    }
    
    for (size_t i = 0; i < location.directives.size(); ++i) {
        const Directive& directive = location.directives[i];
        
        validateDirective(directive, "location", ALLOWED_DIRECTIVES);
        
        if (directive.name == "root") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "root directive requires exactly one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "autoindex") {
            if (directive.parameters.size() != 1 || 
                (directive.parameters[0] != "on" && directive.parameters[0] != "off")) {
                addError(ValidationError::ERROR, "autoindex directive requires 'on' or 'off'", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "index") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "index directive requires at least one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "allow_methods") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "allow_methods requires at least one method", 
                        getTokenLine(directive.name), "location");
                valid = false;
                continue;
            }
            for (size_t j = 0; j < directive.parameters.size(); ++j) {
                const std::string& method = directive.parameters[j];
                if (method != "GET" && method != "POST" && method != "DELETE" && 
                    method != "PUT" && method != "HEAD") {
                    addError(ValidationError::ERROR, "invalid HTTP method in allow_methods: " + method, 
                            getTokenLine(directive.name), "location");
                    valid = false;
                }
            }
        }
        else if (directive.name == "return") {
            if (directive.parameters.size() < 2) {
                addError(ValidationError::ERROR, "return directive requires at least two parameters", 
                        getTokenLine(directive.name), "location");
                valid = false;
                continue;
            }
            const char* cstr = directive.parameters[0].c_str();
            char* endptr;
            int code = strtol(cstr, &endptr, 10);
            if (*endptr != '\0' || (code != 301 && code != 302 && code != 303 && code != 307 && code != 308)) {
                addError(ValidationError::ERROR, "Invalid redirection code: " + directive.parameters[0], 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "client_max_body_size") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "client_max_body_size requires exactly one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
                continue;
            }
            
            std::string size_str = directive.parameters[0];
            size_t len = size_str.length();
            char unit = 'B';
            if (len > 0 && std::isalpha(size_str[len - 1])) {
                unit = std::toupper(size_str[len - 1]);
                size_str = size_str.substr(0, len - 1);
            }
            
            const char* cstr = size_str.c_str();
            char* endptr;
            strtoul(cstr, &endptr, 10);
            if (*endptr != '\0') {
                addError(ValidationError::ERROR, "client_max_body_size invalid number format: " + directive.parameters[0], 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
            if (unit != 'B' && unit != 'K' && unit != 'M' && unit != 'G') {
                addError(ValidationError::ERROR, "client_max_body_size invalid unit: " + std::string(1, unit), 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "cgi_extension") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "cgi_extension requires at least one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
                continue;
            }
            for (size_t j = 0; j < directive.parameters.size(); ++j) {
                const std::string& ext = directive.parameters[j];
                if (ext.empty() || ext[0] != '.') {
                    addError(ValidationError::ERROR, "invalid CGI extension: " + ext, 
                            getTokenLine(directive.name), "location");
                    valid = false;
                }
            }
        }
        else if (directive.name == "cgi_path") {
            if (directive.parameters.empty()) {
                addError(ValidationError::ERROR, "cgi_path requires at least one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "upload_store") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "upload_store requires exactly one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
        else if (directive.name == "alias") {
            if (directive.parameters.size() != 1) {
                addError(ValidationError::ERROR, "alias directive requires exactly one parameter", 
                        getTokenLine(directive.name), "location");
                valid = false;
            }
        }
    }
    
    return valid;
}

std::vector<ServerConfig> ConfigParser::create_servers() {
    std::vector<ServerConfig> servers;
    
    for (size_t i = 0; i < servers_.size(); ++i) {
        ServerConfig server;
        
        for (size_t j = 0; j < servers_[i].directives.size(); ++j) {
            const Directive& directive = servers_[i].directives[j];
            
            if (directive.name == "listen") {
                if (!directive.parameters.empty()) {
                    std::string param = directive.parameters[0];
                    size_t colon_pos = param.find(':');
                    
                    if (colon_pos != std::string::npos) {
                        std::string host = param.substr(0, colon_pos);
                        std::string port = param.substr(colon_pos + 1);
                        server.set_host(host);
                        server.set_port(port);
                    } else {
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
                   server.set_index(directive.parameters);
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
                    std::vector<std::string> error_codes(directive.parameters.begin(), 
                                                         directive.parameters.end() - 1);
                    std::string error_page = directive.parameters.back();
                    server.set_error_pages(error_codes, error_page);
                } else {
                    std::cerr << "Error: error_page directive requires at least two parameters" << std::endl;
                }
            }
        }
        
        for (size_t j = 0; j < servers_[i].nested_blocks.size(); ++j) {
            if (servers_[i].nested_blocks[j].name == "location") {
                Location location(servers_[i].nested_blocks[j]);
                server.add_location(location);
            }
        }
        servers.push_back(server);
    }
    
    return servers;
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

bool test_config(const std::string& config_file) {
    std::cout << "Testing configuration file: " << config_file << std::endl;
    
    ConfigParser parser(config_file);
    
    if (!parser.parse()) {
        std::cerr << "Failed to parse configuration file." << std::endl;
        return false;
    }
    bool is_valid = parser.validate_config();
    parser.print_config();
    return is_valid;
}