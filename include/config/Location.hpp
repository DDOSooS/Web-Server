#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include "Block.hpp"

class Location {
private:
    std::string                 _path;
    std::string                 _root;
    bool                        _autoindex;
    std::string                 _index;
    std::vector<bool>          _allow_methods; // GET POST DELETE PUT- HEAD-
    std::string                 _return;
    std::string                 _alias;
    std::vector<std::string>    _cgi_path;
    std::vector<std::string>    _cgi_ext;
    unsigned long               _client_max_body_size;

public:
    Location();
    Location(const Location &other);
    Location(const Block &location);
    Location &operator=(const Location &rhs);
    ~Location();

    void set_path(std::string new_path);
    void set_root_location(std::string new_root);     
    void set_autoindex(std::string new_auto_index);
    void set_index(std::string new_index);
    void set_allowMethods(std::vector<std::string> allow_methods); // GET POST- DELETE- PUT- HEAD-
    void set_return(std::string new_return);
    void set_alias(std::string new_alias);
    void set_cgiPath(std::vector<std::string> cgi_paths);
    void set_cgiExt(std::vector<std::string> cgi_exts);
    void set_clientMaxBodySize(std::string client_max_body_size);

    std::string                 get_path() const;
    std::string                 get_root_location() const;
    bool                        get_autoindex() const;
    std::string                 get_index() const;
    std::vector<bool>          get_allowMethods() const; // GET+ POST- DELETE- PUT- HEAD-
    std::string                 get_return() const;
    std::string                 get_alias() const;
    std::vector<std::string>    get_cgiPath() const;
    std::vector<std::string>    get_cgiExt() const;
    unsigned long               get_clientMaxBodySize() const;
};

#endif // LOCATION_HPP