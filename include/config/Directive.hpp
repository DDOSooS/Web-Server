#ifndef DIRECTIVE_HPP
#define DIRECTIVE_HPP

#include <string>
#include <vector>

class Directive {
public:
    Directive(const std::string& name, const std::vector<std::string>& parameters);
    Directive();
    Directive(const Directive &other);
    Directive &operator=(const Directive &rhs);
    ~Directive();

    std::string name;
    std::vector<std::string> parameters;
};

#endif