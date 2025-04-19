// ConfigParser.cpp
#include "ConfigParser.hpp"


// Directive implementation
Directive::Directive(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}

// Block implementation
Block::Block(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}
// Block default constructor
Block::Block() {}

// ConfigParser implementation
ConfigParser::ConfigParser(const std::string& file_name) : file_name_(file_name) {
    root_block_ = Block();
}

bool ConfigParser::parse() {
    std::string content;
    if (!read_file_content(content)) {
        return false;
    }
    
    // Tokenize content
    std::vector<std::string> tokens = tokenize(content);
    process_tokens(tokens);
    
    std::vector<std::string>::iterator it = tokens.begin();
    root_block_ = parse_block(it, tokens.end());
    
    return true;
}

void ConfigParser::print_config() const {
    print_block(root_block_, 0);
}

const Block& ConfigParser::get_root_block() const {
    return root_block_;
}

bool ConfigParser::read_file_content(std::string& content) {
    std::ifstream file(file_name_);
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
    
   for(int i = 0; i < content.length(); ++i){
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
    std::cout << "<";
    for(std::string tok:tokens){
        std::cout << tok << "> <";
    }
    std::cout << ">" << std::endl;
    return tokens;
}


void ConfigParser::process_tokens(std::vector<std::string>& tokens) {
    // Validate balanced braces
    int brace_count = 0;
    for (const std::string& token : tokens) {
        if (token == "{") brace_count++;
        else if (token == "}") brace_count--;
        
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
        return Block("", {});
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
            std::cout << "nested block!";
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
        return Directive("", {});
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
    for (const std::string& param : block.parameters) {
        std::cout << " " << param;
    }
    std::cout << " {" << std::endl;
    
    for (const Directive& directive : block.directives) {
        std::cout << indent << "  " << directive.name;
        for (const std::string& param : directive.parameters) {
            std::cout << " " << param;
        }
        std::cout << ";" << std::endl;
    }
    
    for (const Block& nested_block : block.nested_blocks) {
        print_block(nested_block, indent_level + 1);
    }
    std::cout << indent << "}" << std::endl;
}