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
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Directive> directives;
    std::vector<Block> nested_blocks;

    Block(const std::string& name, const std::vector<std::string>& parameters);
    Block();
    Block(const Block &other);
    Block &operator=(const Block &rhs);
    ~Block();

    const std::vector<std::string>& get_parameters() const;
    std::vector<std::string> get_directive_params(const std::string& directive_name) const;
    bool has_directive(const std::string& directive_name) const;
    const Directive* find_directive(const std::string& directive_name) const;
};

#endif // BLOCK_HPP