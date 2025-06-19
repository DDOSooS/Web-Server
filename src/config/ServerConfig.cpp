#include "config/ServerConfig.hpp"
#include "config/Location.hpp"

ServerConfig::ServerConfig() {
    this->_port = 0;
    this->_host = "0.0.0.0";
    this->_server_name = "";
    this->_root = "";
    this->_client_max_body_size = 0;
    this->_autoindex = false;
    this->_index.clear();
    this->_error_pages.clear();
    this->_locations.clear();

    
    initializeDefaultErrorPages();
}

void ServerConfig::initializeDefaultErrorPages() {
    const int common_errors[] = {
        301, 302, 400, 401, 403, 404, 405, 406, 
        500, 501, 502, 503, 504, 505
    }; 
    const size_t num_errors = sizeof(common_errors) / sizeof(common_errors[0]);
    for (size_t i = 0; i < num_errors; ++i)
    {
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

std::vector<std::string>		ServerConfig::get_index() const {
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
    std::string size_str = body_size;
    size_t len = size_str.length();
    char unit = 'B';
    
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

void ServerConfig::set_index(std::vector<std::string> param){
    this->_index.clear();
    for (size_t i = 0; i < param.size(); ++i) {
        if (!param[i].empty()) {
            this->_index.push_back(param[i]);
        } else {
            std::cerr << "config error: set_index [" << param[i] << "] cannot be empty" << std::endl;
        }
    }
    if (this->_index.empty()) {
        std::cerr << "config error: set_index requires at least one index file" << std::endl;
        return;
    }
    std::sort(this->_index.begin(), this->_index.end());
    this->_index.erase(std::unique(this->_index.begin(), this->_index.end()), this->_index.end());
}

void ServerConfig::set_autoindex(std::string index){
    if (index == "on" || index == "off")
		this->_autoindex = (index == "on");
	else {
		std::cout << "config error: set_autoindex [" << index << "]" << std::endl;
		return;
	}
}


void ServerConfig::set_error_pages(const std::vector<std::string>& error_codes, const std::string& error_page) {
    if (error_codes.empty()) {
        std::cerr << "Error: No error codes provided" << std::endl;
        return;
    }
    for (size_t i = 0; i < error_codes.size(); ++i) {
        const char* cstr = error_codes[i].c_str();
        char* endptr;
        int code = strtol(cstr, &endptr, 10);
        
        if (*endptr != '\0' || code < 100 || code > 599) {
            std::cerr << "Error: Invalid error code: " << error_codes[i] << std::endl;
            continue;
        }

        this->_error_pages[static_cast<short>(code)] = error_page;
    }
}

void ServerConfig::add_location(const Location& location) {
    this->_locations.push_back(location);
}


const Location *ServerConfig::findMatchingLocation(const std::string &path) const
{
    const Location *default_location = NULL;
    
    for (size_t i = 0; i < this->_locations.size(); i++)
    {
        if (!this->_locations[i].get_path().empty() && this->_locations[i].get_path() == path)
            return &this->_locations[i];
        if (this->_locations[i].get_path() == "/")
        {
            default_location = &this->_locations[i];
        }
    }
    return default_location;
}

void ServerConfig::print_server_config() const {
    std::cout << "Server Config:" << std::endl;
    std::cout << "  Port: " << this->_port << std::endl;
    std::cout << "  Host: " << this->_host << std::endl;
    std::cout << "  Server Name: " << this->_server_name << std::endl;
    std::cout << "  Root: " << this->_root << std::endl;
    std::cout << "  Client Max Body Size: " << this->_client_max_body_size << " bytes" << std::endl;
    std::cout << "  Index Files: ";
    if (this->_index.empty()) {
        std::cout << "None" << std::endl;
    } else {
        for (size_t i = 0; i < this->_index.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << this->_index[i];
        }
        std::cout << std::endl;
    }
    std::cout << "  Autoindex: " << (this->_autoindex ? "on" : "off") << std::endl;

    std::cout << "  Error Pages: " << this->_error_pages.size() << std::endl;
    for (std::map<short, std::string>::const_iterator it = this->_error_pages.begin(); 
        it != this->_error_pages.end(); ++it) {
        if (!it->second.empty()) {
            std::cout << "    Error_page: " << it->first << " --> " << it->second << std::endl;
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

    std::cout << "End of Server Config" << std::endl;
}


const Location* ServerConfig::findBestMatchingLocation(const std::string& path) const {
    const Location* best_match = NULL;
    size_t best_length = 0;

    for (size_t i = 0; i < this->_locations.size(); ++i) {
        const Location& loc = this->_locations[i];
        const std::string& loc_path = loc.get_path();
        
        if (path.find(loc_path) == 0) {
            if (loc_path.length() > best_length) {
                best_length = loc_path.length();
                best_match = &loc;
            }
        }
    }
    
    return best_match ? best_match : findMatchingLocation(path);
}


