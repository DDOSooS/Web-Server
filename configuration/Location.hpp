#include "ConfigParser.hpp"


class Location :public Block {
    public:
        Location();
        Location(const Location &other);
        Location &operator=(const Location &rhs);
        ~Location();

        void setPath(std::string new_path);
        void setRoot(std::string new_root);
        void setAutoIndex(bool auto_index);
        void
};