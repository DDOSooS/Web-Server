#include "config/Directive.hpp"

Directive::Directive(const std::string& name, const std::vector<std::string>& parameters)
    : name(name), parameters(parameters) {}

Directive::Directive() {}

Directive::Directive(const Directive &other){
    if (this != &other){
        this->name = other.name;
        this->parameters = other.parameters;
    }
}

Directive& Directive::operator=(const Directive &rhs){
    if (this != &rhs){
        this->name = rhs.name;
        this->parameters = rhs.parameters;
    }
    return (*this);
}

Directive::~Directive(){}