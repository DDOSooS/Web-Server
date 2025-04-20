#include "ConfigParser.hpp"

// location parameters
class Location {
    private:
        std::string _path;
        std::string _root;
        

    public:
        Location();
        Location(const Location &other);
        Location &operator=(const Location &rhs);
        ~Location();
};