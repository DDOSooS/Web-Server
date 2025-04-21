#include "Server.hpp"

Server::Server(){
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

Server::Server(const Server &other){
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

Server& Server::operator=(const Server &other){
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

Server::~Server(){
    std::cout << "Server destructor called : " <<  _server_name << std::endl;
}


uint16_t						Server::get_port(){
    return this->_port;
}

in_addr_t						Server::get_host(){
    return this->_host;
}

std::string						Server::get_server_name(){
    return this->_server_name;
}

std::string						Server::get_root(){
    return this->_root;
}

unsigned long					Server::get_client_max_body_size(){
    return this->_client_max_body_size;
}

std::string						Server::get_index(){
    return this->_index;
}

bool							Server::get_autoindex(){
    return this->_autoindex;
}

std::map<short, std::string>	Server::get_error_pages(){
    return this->_error_pages;
}

std::vector<Location> 			Server::get_locations(){
    return this->_locations;
}

struct sockaddr_in 				Server::get_server_address(){
    return this->_server_address;
}

int     						Server::get_listen_fd(){
    return this->_listen_fd;
}




void Server::set_port(std::string param){
    const char* cstr = param.c_str();
    char* endptr;
    unsigned long result = strtoul(cstr, &endptr, 10);
    if (*endptr != '\0' || result < 1 || result > 65636) {
        std::cerr << "Error: Invalid Port number : '" << param << std::endl;
		return;
    }
	this->_port = result;
}

void Server::set_host(std::string param){
    _host = inet_addr(param.c_str());
    if (_host == INADDR_NONE)
        std::cerr << "Error: Invalid IP address format" << std::endl;
}

void Server::set_server_name(std::string param){

}

void Server::set_root(std::string param){

}

void Server::set_client_max_body_size(std::string body_size){
    const char* cstr = body_size.c_str();
    char* endptr;
    unsigned long result = strtoul(cstr, &endptr, 10);
    if (*endptr != '\0') {
        std::cerr << "client_max_body_size: '" << body_size << "' is not a valid number" << std::endl;
		return;
    }
	this->_client_max_body_size = result;
}

void Server::set_index(std::string param){
    this->_index = param;
}

void Server::set_autoindex(std::string index){
    if (index == "on" || index == "off")
		this->_autoindex = (index == "on");
	else {
		std::cout << "config error: set_autoindex [" << index << "]" << std::endl;
		return;
	}
}

void Server::set_error_pages(std::string param){ // error_page <error_code> <error_page>;
// TODO:
}

void Server::set_locations(std::string param){
//TODO:
// check if a valid location or CGI
// and then check if it ok
}

void Server::set_server_address(std::string param){

}

void Server::set_listen_fd(int sock_fd){
    this->_listen_fd = sock_fd;
}
// TODO:: setup the server