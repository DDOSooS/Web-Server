#include <string>
#include <vector>
// Class representing a configuration directive (key-value pair)
class Directive {
public:
    Directive(const std::string& name, const std::vector<std::string>& parameters);
    
    std::string name;
    std::vector<std::string> parameters;
};
