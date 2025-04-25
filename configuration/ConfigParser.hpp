#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include "Block.hpp"
#include "Directive.hpp"
#include "Server.hpp"


class ConfigParser {
public:
    ConfigParser(const std::string& file_name);
    
    bool parse();
    void print_config() const;
    const Block& get_root_block() const;
    const std::vector<Block>& get_servers() const;
    std::vector<Server> create_servers();

private:
    std::string file_name_;
    Block root_block_;
    std::vector<Block> servers_;
    
    bool read_file_content(std::string& content);
    std::vector<std::string> tokenize(const std::string& content);
    bool check_if_directive(std::vector<std::string>::iterator it,
                          const std::vector<std::string>::iterator& end);
    Block parse_block(std::vector<std::string>::iterator& it, 
                     const std::vector<std::string>::iterator& end);
    Directive parse_directive(std::vector<std::string>::iterator& it, 
                            const std::vector<std::string>::iterator& end);
    void process_tokens(std::vector<std::string>& tokens);
    bool validate_config();
    bool validate_server_block(const Block& server);
    bool validate_location_block(const Block& location);
};
void print_block(const Block& block, int indent_level);