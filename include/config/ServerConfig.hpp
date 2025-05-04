#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
// Forward declaration
class Location;

class ServerConfig {
private:
    uint16_t                    _port;
    std::string                   _host;
    std::string                 _server_name;
    std::string                 _root;
    unsigned long               _client_max_body_size;
    std::string                 _index;
    bool                        _autoindex;
    std::map<short, std::string> _error_pages;
    std::vector<Location>       _locations;

public:
    ServerConfig();
    ServerConfig(const ServerConfig &other);
    ServerConfig &operator=(const ServerConfig &rhs);
    ~ServerConfig();

    uint16_t                    get_port();
    std::string                   get_host();
    std::string                 get_server_name();
    std::string                 get_root();
    unsigned long               get_client_max_body_size();
    std::string                 get_index();
    bool                        get_autoindex();
    std::map<short, std::string> get_error_pages();
    std::vector<Location>       get_locations();

    void set_port(std::string param);
    void set_host(std::string param);
    void set_server_name(std::string param);
    void set_root(std::string param);
    void set_client_max_body_size(std::string param);
    void set_index(std::string param);
    void set_autoindex(std::string param);
    void set_error_pages(std::string error_code, std::string error_page);
    void add_location(const Location& location);

    friend class ConfigParser;
};

#endif // ServerConfig_HPP