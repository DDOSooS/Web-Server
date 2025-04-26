#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include "Directive.hpp"

class Block {
public:
    Block(const std::string& name, const std::vector<std::string>& parameters);
    Block();
    ~Block();
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Directive> directives;
    std::vector<Block> nested_blocks;
    std::vector<std::string>& get_parameters();
    std::vector<std::string>& get_directive(std::string& directive_name);
};

#endif // BLOCK_HPP