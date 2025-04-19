#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

// Forward declarations
class Block;
class Directive;

// Class representing a configuration directive (key-value pair)
class Directive {
public:
    Directive(const std::string& name, const std::vector<std::string>& parameters);
    
    std::string name;
    std::vector<std::string> parameters;
};

// Class representing a configuration block (like server, location, etc.)
class Block {
public:
    Block(const std::string& name, const std::vector<std::string>& parameters);
    Block();
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Directive> directives;
    std::vector<Block> nested_blocks;
};



class ConfigParser {
public:
    ConfigParser(const std::string& file_name);
    
    bool parse();
    void print_config() const;
    const Block& get_root_block() const;
    
private:
    std::string file_name_;
    Block root_block_;
    
    bool read_file_content(std::string& content);
    std::vector<std::string> tokenize(const std::string& content);
    bool check_if_directive(std::vector<std::string>::iterator it,
                          const std::vector<std::string>::iterator& end);
    Block parse_block(std::vector<std::string>::iterator& it, 
                     const std::vector<std::string>::iterator& end);
    Directive parse_directive(std::vector<std::string>::iterator& it, 
                            const std::vector<std::string>::iterator& end);
    void process_tokens(std::vector<std::string>& tokens);
};
void print_block(const Block& block, int indent_level);