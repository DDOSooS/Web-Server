#include "ServerConfig.hpp"
#include "Location.hpp"

ServerConfig::ServerConfig(){
    this->_port = 0;
    this->_host = INADDR_NONE;
    this->_server_name = "";
    this->_root = "";
    this->_client_max_body_size = 0;
    this->_index = "";
    this->_autoindex = false;
    this->_listen_fd = 0;
    // init error pages
    _error_pages[301] = "";
	_error_pages[302] = "";
	_error_pages[400] = "";
	_error_pages[401] = "";
	_error_pages[402] = "";
	_error_pages[403] = "";
	_error_pages[404] = "";
	_error_pages[405] = "";
	_error_pages[406] = "";
	_error_pages[500] = "";
	_error_pages[501] = "";
	_error_pages[502] = "";
	_error_pages[503] = "";
	_error_pages[505] = "";
	_error_pages[505] = "";
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
        this->_server_address = other._server_address;
        this->_listen_fd = other._listen_fd;
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
        this->_server_address = other._server_address;
        this->_listen_fd = other._listen_fd;
    }
    return (*this);
}


ServerConfig::~ServerConfig() {
    if (_listen_fd > 0) {
        close(_listen_fd);
    }
    std::cout << "ServerConfig destructor called : " << _server_name << std::endl;
}


uint16_t						ServerConfig::get_port(){
    return this->_port;
}

in_addr_t						ServerConfig::get_host(){
    return this->_host;
}

std::string						ServerConfig::get_server_name(){
    return this->_server_name;
}

std::string						ServerConfig::get_root(){
    return this->_root;
}

unsigned long					ServerConfig::get_client_max_body_size(){
    return this->_client_max_body_size;
}

std::string						ServerConfig::get_index(){
    return this->_index;
}

bool							ServerConfig::get_autoindex(){
    return this->_autoindex;
}

std::map<short, std::string>	ServerConfig::get_error_pages(){
    return this->_error_pages;
}

std::vector<Location> 			ServerConfig::get_locations(){
    return this->_locations;
}

struct sockaddr_in 				ServerConfig::get_server_address(){
    return this->_server_address;
}

int     						ServerConfig::get_listen_fd(){
    return this->_listen_fd;
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
    _host = inet_addr(param.c_str());
    if (_host == INADDR_NONE)
        std::cerr << "Error: Invalid IP address format" << std::endl;
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

void ServerConfig::set_server_address() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = this->_host;
    server_addr.sin_port = htons(this->_port);
    
    this->_server_address = server_addr;
}

void ServerConfig::set_listen_fd(int sock_fd){
    this->_listen_fd = sock_fd;
}
// TODO:: setup the server