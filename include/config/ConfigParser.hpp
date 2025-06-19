#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Block.hpp"
#include "Directive.hpp"
#include "ServerConfig.hpp"

class ValidationError {
public:
    enum ErrorLevel {
        WARNING,
        ERROR,
        CRITICAL
    };

    ValidationError(ErrorLevel level, const std::string& message, int line = -1, const std::string& context = "");
    
    void print() const;
    
    static std::string errorLevelToString(ErrorLevel level);
    
    ErrorLevel getLevel() const;
    
private:
    ErrorLevel _level;
    std::string _message;
    int _line;
    std::string _context;
};

class ConfigParser {
public:
    ConfigParser(const std::string& file_name);
    
    bool parse();
    void print_config() const;
    bool test_config();
    const Block& get_root_block() const;
    const std::vector<Block>& get_servers() const;
    std::vector<ServerConfig> create_servers();
    bool validate_config();

private:
    std::string file_name_;
    Block root_block_;
    std::vector<Block> servers_;
    std::map<std::string, int> token_line_numbers_;
    std::vector<ValidationError> _errors;
    
    bool read_file_content(std::string& content);
    std::vector<std::string> tokenize(const std::string& content);
    bool check_if_directive(std::vector<std::string>::iterator it,
                          const std::vector<std::string>::iterator& end);
    Block parse_block(std::vector<std::string>::iterator& it, 
                     const std::vector<std::string>::iterator& end);
    Directive parse_directive(std::vector<std::string>::iterator& it, 
                            const std::vector<std::string>::iterator& end);
    void process_tokens(std::vector<std::string>& tokens);
    bool validate_server_block(const Block& server);
    bool validate_location_block(const Block& location);
    void addError(ValidationError::ErrorLevel level, const std::string& message, int line = -1, const std::string& context = "");
    bool validateDirective(const Directive& directive, const std::string& context, const std::vector<std::string>& allowed_directives);
    bool printValidationResults() const;
    int getTokenLine(const std::string& token);
};

void print_block(const Block& block, int indent_level);

bool test_config(const std::string& config_file);

#endif