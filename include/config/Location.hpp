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
    std::vector<std::string>    _index;
    std::vector<std::string>     _allow_methods; // GET POST DELETE PUT- HEAD-
    std::vector<std::string>    _return; // 301 /new-page
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
    // == operator is not needed for this class, as it is not a POD type and has no dynamic memory management.
    bool operator==(const Location &rhs) const;
    void set_path(const std::string &new_path); // e.g. "/"
    void set_root_location(std::string new_root); // e.g. "/var/www/html"
    void set_autoindex(bool new_auto_index); // true for "on", false for "off
    void set_index(std::vector<std::string>  new_index); // e.g. "index.html"
    void set_allowMethods(const std::vector<std::string> &methods); // e.g. {"GET", "POST", "DELETE", "PUT", "HEAD"}
    void set_return(const std::vector<std::string> &return_values); // 301 /new-page
    void set_alias(std::string new_alias); // e.g. "/alias
    void set_cgiPath(std::vector<std::string> cgi_paths); // e.g. {"/usr/bin/php-cgi", "/usr/bin/python"}
    void set_cgiExt(std::vector<std::string> cgi_exts); // e.g. {".php", ".py"}
    void set_clientMaxBodySize(std::string client_max_body_size);

    std::string                 get_path() const;
    std::string                 get_root_location() const;
    bool                        get_autoindex() const;
    std::vector<std::string>    get_index() const;
    std::vector<std::string>    get_allowMethods() const; // GET+ POST- DELETE- PUT- HEAD-
    std::vector<std::string>    get_return() const;
    std::string                 get_alias() const;
    std::vector<std::string>    get_cgiPath() const;
    std::vector<std::string>    get_cgiExt() const;
    unsigned long               get_clientMaxBodySize() const;
    bool                        is_method_allowed(const std::string& method) const; // Check if a method is allowed in this location
    void print_location_config() const; // For debugging purposes
};

#endif // LOCATION_HPP