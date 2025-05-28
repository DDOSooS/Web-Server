#include "config/Location.hpp"
#include "config/Block.hpp"
#include <cstdlib>

Location::Location() {
    this->_path = "";
    this->_root = "";
    this->_autoindex = false;
    this->_index = "";
    this->_alias = "";
    this->_client_max_body_size = 0;
    this->_cgi_ext.clear();
    this->_cgi_path.clear();
}

Location::Location(const Location &other) {
    this->_path = other._path;
    this->_root = other._root;
    this->_autoindex = other._autoindex;
    this->_index = other._index;
    this->_allow_methods = other._allow_methods;
    this->_return = other._return;
    this->_alias = other._alias;
    this->_client_max_body_size = other._client_max_body_size;
    this->_cgi_ext = other._cgi_ext;
    this->_cgi_path = other._cgi_path;
}

Location::Location(const Block &location) {
    // Initialize with defaults
    this->_path = "";
    this->_root = "";
    this->_autoindex = false;
    this->_index = "";
    this->_alias = "";
    this->_client_max_body_size = 0;
    this->_cgi_ext.clear();
    this->_cgi_path.clear();
    
    // Set path from block parameters
    if (!location.parameters.empty()) {
        this->_path = location.parameters[0];
    }
    
    // Process directives
    for (size_t i = 0; i < location.directives.size(); ++i) {
        const Directive& directive = location.directives[i];
        
        if (directive.name == "root" && !directive.parameters.empty()) {
            this->_root = directive.parameters[0];
        }
        else if (directive.name == "autoindex" && !directive.parameters.empty()) {
            if (directive.parameters[0] == "on") {
                this->_autoindex = true;
            } else if (directive.parameters[0] == "off") {
                this->_autoindex = false;
            } else {
                std::cerr << "config error: autoindex must be 'on' or 'off'" << std::endl;
            }
        }
        else if (directive.name == "index" && !directive.parameters.empty()) {
            this->_index = directive.parameters[0];
        }
        else if (directive.name == "allow_methods") {
            this->set_allowMethods(directive.parameters);
        }
        else if (directive.name == "return" && !directive.parameters.empty()) {
            this->set_return(directive.parameters);
        }
        else if (directive.name == "alias" && !directive.parameters.empty()) {
            this->_alias = directive.parameters[0];
        }
        else if (directive.name == "client_max_body_size" && !directive.parameters.empty()) {
            this->set_clientMaxBodySize(directive.parameters[0]);
        }
        else if (directive.name == "cgi_extension") {
            this->_cgi_ext = directive.parameters;
        }
        else if (directive.name == "cgi_path") {
            this->_cgi_path = directive.parameters;
        }
    }
}

Location &Location::operator=(const Location &other) {
    if (this != &other) {
        this->_path = other._path;
        this->_root = other._root;
        this->_autoindex = other._autoindex;
        this->_index = other._index;
        this->_allow_methods = other._allow_methods;
        this->_return = other._return;
        this->_alias = other._alias;
        this->_client_max_body_size = other._client_max_body_size;
        this->_cgi_ext = other._cgi_ext;
        this->_cgi_path = other._cgi_path;
    }
    return *this;
}

Location::~Location() {}

void Location::set_path(const std::string &new_path) {
    if (new_path.empty() || new_path[0] != '/') {
        std::cerr << "config error: set_path [" << new_path << "] must start with '/'" << std::endl;
        return;
    }
    this->_path = new_path;
}

void Location::set_root_location(std::string new_root){
	this->_root = new_root;
}

void Location::set_autoindex(bool new_auto_index){
    if (new_auto_index)
        this->_autoindex = true;
    else
        this->_autoindex = false;
}

void Location::set_index(std::string new_index){
	this->_index =	new_index;
}

void Location::set_allowMethods(const std::vector<std::string> &methods) {
    if (methods.empty()) {
        std::cerr << "config error: set_allowMethods requires at least one method" << std::endl;
        return;
    }
    
    // Clear existing methods and add new ones
    this->_allow_methods.clear();
    for (size_t i = 0; i < methods.size(); ++i) {
        std::string method = methods[i];
        if (method == "GET" || method == "POST" || method == "DELETE" || method == "PUT" || method == "HEAD") {
            this->_allow_methods.push_back(method);
        } else {
            std::cerr << "config error: set_allowMethods [" << method << "] is not a valid HTTP method" << std::endl;
        }
    }
    if (this->_allow_methods.empty()) {
        std::cerr << "config error: set_allowMethods must contain at least one valid HTTP method" << std::endl;
    }
}

bool Location::is_method_allowed(const std::string& method) const {
    if (_allow_methods.empty()) {
        // Default: allow only GET and HEAD if no methods specified
        return (method == "GET" || method == "HEAD");
    }
    
    return std::find(_allow_methods.begin(), _allow_methods.end(), method) 
           != _allow_methods.end();
}

// GET POST- DELETE- PUT- HEAD-
void Location::set_return(const std::vector<std::string> &new_return){
    if (new_return.size() < 2) {
        std::cerr << "config error: set_return requires at least two parameters" << std::endl;
        return;
    }
    // Clear existing return values and add new ones
    this->_return.clear();
    for (size_t i = 0; i < new_return.size(); ++i) {
        this->_return.push_back(new_return[i]);
    }
}

void Location::set_alias(std::string new_alias){
	this->_alias = new_alias;
}

void Location::set_cgiPath(std::vector<std::string> cgi_paths){
	this->_cgi_path = cgi_paths;
}

void Location::set_cgiExt(std::vector<std::string> cgi_exts){
	this->_cgi_ext = cgi_exts;
}

void Location::set_clientMaxBodySize(std::string body_size){
    // Check if parameter is a valid number
    std::string size_str = body_size;
    size_t len = size_str.length();
    char unit = 'B';
    
    // Check for K, M, G units
    if (len > 0 && std::isalpha(size_str[len - 1])) {
        unit = std::toupper(size_str[len - 1]);
        size_str = size_str.substr(0, len - 1);
    }
    
    const char* cstr = size_str.c_str();
    char* endptr;
    unsigned long result = strtoul(cstr, &endptr, 10);
    if (*endptr != '\0') {
        std::cerr << "client_max_body_size: '" << body_size << "' is not a valid number" << std::endl;
        return;
    }
    
    // Apply multiplier based on unit
    switch (unit) {
        case 'K':
            result *= 1024;
            break;
        case 'M':
            result *= 1024 * 1024;
            break;
        case 'G':
            result *= 1024 * 1024 * 1024;
            break;
        case 'B':
            break;
        default:
            std::cerr << "client_max_body_size: invalid unit: " << unit << std::endl;
            return;
    }
    
    this->_client_max_body_size = result;
}

std::string Location::get_path() const{
	return this->_path;
}

std::string Location::get_root_location() const {
	return this->_root;
}

bool 	Location::get_autoindex() const{
	return this->_autoindex;
}

std::string Location::get_index() const{
	return this->_index;
}

std::vector<std::string> Location::get_allowMethods() const
{
	return this->_allow_methods;
}

std::vector<std::string> Location::get_return() const {
	return this->_return;
}

std::string Location::get_alias() const {
	return this->_alias;
}

std::vector<std::string>	Location::get_cgiPath() const {
	return this->_cgi_path;
}

std::vector<std::string>	Location::get_cgiExt() const {
	return this->_cgi_ext;
}

unsigned long Location::get_clientMaxBodySize() const {
	return this->_client_max_body_size;
}
// void Location::print_location_config() const in c++ 98
void Location::print_location_config() const {
    std::cout << "Location Config:" << std::endl;
    std::cout << "  Path: " << this->_path << std::endl;
    std::cout << "  Root: " << this->_root << std::endl;
    std::cout << "  Autoindex: " << (this->_autoindex ? "on" : "off") << std::endl;
    std::cout << "  Index: " << this->_index << std::endl;
    std::cout << "  Allow Methods: ";
    for (size_t i = 0; i < this->_allow_methods.size(); ++i) {
        const std::string &method = this->_allow_methods[i];
        if (i > 0) {
            std::cout << "+";
        }
        if (method == "GET") {
            std::cout << "GET+";
        } else if (method == "POST") {
            std::cout << "POST+";
        } else if (method == "DELETE") {
            std::cout << "DELETE+";
        } else if (method == "PUT") {
            std::cout << "PUT+";
        } else if (method == "HEAD") {
            std::cout << "HEAD-";
        }
        // Print the method without the trailing '+' or '-'
        else {
            std::cout << method;
        }
    }
    if (this->_allow_methods.empty()) {
        std::cout << "None";
    }
    std::cout << std::endl;
    std::cout << "  Return: ";
    for (size_t i = 0; i < this->_return.size(); ++i) {
        if (i > 0) {
            std::cout << " ";
        }
        std::cout << this->_return[i];
    }
    if (this->_return.empty()) {
        std::cout << "None";
    }
    std::cout << std::endl;
    std::cout << "  Alias: " << this->_alias << std::endl;
    std::cout << "  CGI Path: ";
    for (size_t i = 0; i < this->_cgi_path.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << this->_cgi_path[i];
    }
    if (this->_cgi_path.empty()) {
        std::cout << "None";
    }
    std::cout << std::endl;
    std::cout << "  CGI Extensions: ";
    for (size_t i = 0; i < this->_cgi_ext.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << this->_cgi_ext[i];
    }
    if (this->_cgi_ext.empty()) {
        std::cout << "None";
    }
    std::cout << std::endl;
    std::cout << "  Client Max Body Size: " << this->_client_max_body_size << " bytes" << std::endl;
    std::cout << std::endl;
}
bool Location::operator==(const Location &rhs) const {
    return (this->_path == rhs._path &&
            this->_root == rhs._root &&
            this->_autoindex == rhs._autoindex &&
            this->_index == rhs._index &&
            this->_allow_methods == rhs._allow_methods &&
            this->_return == rhs._return &&
            this->_alias == rhs._alias &&
            this->_client_max_body_size == rhs._client_max_body_size &&
            this->_cgi_ext == rhs._cgi_ext &&
            this->_cgi_path == rhs._cgi_path);
}