#include "config/ServerConfig.hpp"
#include "config/Location.hpp"

ServerConfig::ServerConfig() {
    this->_port = 0;
    this->_host = "0.0.0.0";
    this->_server_name = "";
    this->_root = "";
    this->_client_max_body_size = 0;
    this->_index = "";
    this->_autoindex = false;
    
    // Initialize common error pages more elegantly
    initializeDefaultErrorPages();
}

void ServerConfig::initializeDefaultErrorPages() {
    const int common_errors[] = {
        301, 302, 400, 401, 403, 404, 405, 406, 
        500, 501, 502, 503, 504, 505
    }; 
    const size_t num_errors = sizeof(common_errors) / sizeof(common_errors[0]);
    for (size_t i = 0; i < num_errors; ++i) {
        _error_pages[common_errors[i]] = "";
    }
}

ServerConfig::ServerConfig(const ServerConfig &other){
    if (this != &other){
        this->_port = other._port;
        this->_host = other._host;
        this->_server_name = other._server_name;
        this->_root = other._root;
        this->_client_max_body_size = other._client_max_body_size;
        this->_index = other._index;
        this->_autoindex = other._autoindex;
        this->_error_pages = other._error_pages;
        this->_locations = other._locations;
    }
}

ServerConfig& ServerConfig::operator=(const ServerConfig &other){
    if (this != &other){
        this->_port = other._port;
        this->_host = other._host;
        this->_server_name = other._server_name;
        this->_root = other._root;
        this->_client_max_body_size = other._client_max_body_size;
        this->_index = other._index;
        this->_autoindex = other._autoindex;
        this->_error_pages = other._error_pages;
        this->_locations = other._locations;
    }
    return (*this);
}


ServerConfig::~ServerConfig() {
}


uint16_t						ServerConfig::get_port() const {
    return this->_port;
}

std::string					ServerConfig::get_host() const {
    return this->_host;
}

std::string						ServerConfig::get_server_name() const {
    return this->_server_name;
}

std::string						ServerConfig::get_root() const {
    return this->_root;
}

unsigned long					ServerConfig::get_client_max_body_size() const {
    return this->_client_max_body_size;
}

std::string						ServerConfig::get_index() const {
    return this->_index;
}

bool							ServerConfig::get_autoindex() const {
    return this->_autoindex;
}

std::map<short, std::string>	ServerConfig::get_error_pages() const {
    return this->_error_pages;
}

std::vector<Location> 			ServerConfig::get_locations() const {
    return this->_locations;
}






void ServerConfig::set_port(std::string param){
    const char* cstr = param.c_str();
    char* endptr;
    unsigned long result = strtoul(cstr, &endptr, 10);
    if (*endptr != '\0' || result < 1 || result > 65636) {
        std::cerr << "Error: Invalid Port number : '" << param << std::endl;
		return;
    }
	this->_port = result;
}

void ServerConfig::set_host(std::string param){
    in_addr_t host = inet_addr(param.c_str());
    if (host == INADDR_NONE)
        std::cerr << "Error: Invalid IP address format" << std::endl;
    _host = param;
}

void ServerConfig::set_server_name(std::string param){
    this->_server_name = param;
}

void ServerConfig::set_root(std::string param){
    this->_root = param;
}

void ServerConfig::set_client_max_body_size(std::string body_size){
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

void ServerConfig::set_index(std::string param){
    this->_index = param;
}

void ServerConfig::set_autoindex(std::string index){
    if (index == "on" || index == "off")
		this->_autoindex = (index == "on");
	else {
		std::cout << "config error: set_autoindex [" << index << "]" << std::endl;
		return;
	}
}


void ServerConfig::set_error_pages(std::string error_code, std::string error_page) {
    const char* cstr = error_code.c_str();
    char* endptr;
    int code = strtol(cstr, &endptr, 10);
    
    if (*endptr != '\0' || code < 100 || code > 599) {
        std::cerr << "Error: Invalid error code: " << error_code << std::endl;
        return;
    }

    this->_error_pages[static_cast<short>(code)] = error_page;
}

void ServerConfig::add_location(const Location& location) {
    this->_locations.push_back(location);
}


const Location *ServerConfig::findMatchingLocation(const std::string &path) const
{
   // look for the longest matching location
    const Location *best_match = nullptr;
    size_t best_length = 0;

    for (size_t i = 0; i < this->_locations.size(); ++i) {
        const Location &location = this->_locations[i];
        // Check if the path starts with the location's path
        const std::string &location_path = location.get_path();
        if (path.find(location_path) == 0) {
            // Check if this location is longer than the best match found so far
            if (location_path.length() > best_length) {
                best_length = location_path.length();
                best_match = &location;
            }
        }
    }

    return best_match;
}
// in C++98  no auto 
void ServerConfig::print_server_config() const {
    std::cout << "Server Config:" << std::endl;
    std::cout << "  Port: " << this->_port << std::endl;
    std::cout << "  Host: " << this->_host << std::endl;
    std::cout << "  Server Name: " << this->_server_name << std::endl;
    std::cout << "  Root: " << this->_root << std::endl;
    std::cout << "  Client Max Body Size: " << this->_client_max_body_size << " bytes" << std::endl;
    std::cout << "  Index: " << this->_index << std::endl;
    std::cout << "  Autoindex: " << (this->_autoindex ? "on" : "off") << std::endl;

    // Print error pages
   for (size_t i = 100; i <= 599; ++i) {
        std::map<short, std::string>::const_iterator it = this->_error_pages.find(static_cast<short>(i));
        if (it == this->_error_pages.end()) {
            continue; // Skip if no error page is set for this code
        }
        std::cout << "  Error Page " << i << ": ";
        // If the error page is empty, print a message indicating no page is set
        if (it != this->_error_pages.end() && !it->second.empty()) {
            std::cout << "  Error Page " << i << ": " << it->second << std::endl;
        }
    }
    std::cout << "  Locations: " << this->_locations.size() << std::endl;
    for (size_t i = 0; i < this->_locations.size(); ++i) {
        std::cout << "  Location " << i + 1 << ":" << std::endl;
        this->_locations[i].print_location_config();
    }
    if (this->_locations.empty()) {
        std::cout << "  No locations configured." << std::endl;
    }
    // Print end of server config
    std::cout << "End of Server Config" << std::endl;
}