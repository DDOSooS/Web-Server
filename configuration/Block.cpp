#include "Directive.hpp"
#include "Block.hpp"
// Directive implementation
Directive::Directive(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}

// Block implementation
Block::Block(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}
// Block default constructor
Block::Block() {}

Block::~Block() {}
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