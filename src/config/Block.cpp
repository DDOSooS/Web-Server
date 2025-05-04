#include "config/Block.hpp"

Block::Block(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}

Block::Block() {}

Block::Block(const Block &other){
    if (this != &other){
        this->name = other.name;
        this->parameters = other.parameters;
        this->directives = other.directives;
        this->nested_blocks = other.nested_blocks;
    }
}

Block& Block::operator=(const Block &rhs){
    if (this != &rhs){
        this->name = rhs.name;
        this->parameters = rhs.parameters;
        this->directives = rhs.directives;
        this->nested_blocks = rhs.nested_blocks;
    }
    return (*this);
}

Block::~Block(){}


std::vector<std::string>& Block::get_parameters(){
    return this->parameters;
}

std::vector<std::string>& Block::get_directive(std::string& directive_name){
    static std::vector<std::string> empty;
    
    for (size_t i = 0; i < directives.size(); ++i) {
        if (directives[i].name == directive_name) {
            return this->directives[i].parameters;
        }
    }
    empty.clear();
    return empty;
}