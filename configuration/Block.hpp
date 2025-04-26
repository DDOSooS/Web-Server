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
        std::string name;                      // "server", "location"
        std::vector<std::string> parameters;   // ["/"] for "location /"
        std::vector<Directive> directives;     // Contained directives
        std::vector<Block> nested_blocks;      // Nested blocks

        Block(const std::string& name, const std::vector<std::string>& parameters);
        Block();
        Block(const Block &other);
		Block &operator=(const Block &rhs);
		~Block();

        // Utility methods
        std::vector<std::string>& get_parameters();
        std::vector<std::string>& get_directive(std::string& directive_name);
    };

#endif // BLOCK_HPP