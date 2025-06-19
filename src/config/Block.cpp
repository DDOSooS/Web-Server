#include "config/Block.hpp"

Block::Block(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}

Block::Block() {}

Block::Block(const Block &other) {
    if (this != &other) {
        this->name = other.name;
        this->parameters = other.parameters;
        this->directives = other.directives;
        this->nested_blocks = other.nested_blocks;
    }
}

Block& Block::operator=(const Block &rhs) {
    if (this != &rhs) {
        this->name = rhs.name;
        this->parameters = rhs.parameters;
        this->directives = rhs.directives;
        this->nested_blocks = rhs.nested_blocks;
    }
    return (*this);
}

Block::~Block() {}

const std::vector<std::string>& Block::get_parameters() const {
    return this->parameters;
}

std::vector<std::string> Block::get_directive_params(const std::string& directive_name) const {
    for (size_t i = 0; i < directives.size(); ++i) {
        if (directives[i].name == directive_name) {
            return directives[i].parameters;
        }
    }
    return std::vector<std::string>();
}

bool Block::has_directive(const std::string& directive_name) const {
    for (size_t i = 0; i < directives.size(); ++i) {
        if (directives[i].name == directive_name) {
            return true;
        }
    }
    return false;
}

const Directive* Block::find_directive(const std::string& directive_name) const {
    for (size_t i = 0; i < directives.size(); ++i) {
        if (directives[i].name == directive_name) {
            return &directives[i];
        }
    }
    return NULL;
}