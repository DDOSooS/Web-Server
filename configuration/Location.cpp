#include "Location.hpp"
#include <cstdlib>


Location::Location(){
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_index = "";
	this->_allow_methods.reserve(5);
	this->_allow_methods.push_back(false);
	this->_allow_methods.push_back(false);
	this->_allow_methods.push_back(false);
	this->_allow_methods.push_back(false);
	this->_allow_methods.push_back(false);
	this->_return = "";
	this->_alias = "";
	this->_client_max_body_size = 0;
}

Location::Location(const Location &other){
	this->_path = other._path;
	this->_root = other._root;
	this->_autoindex = other._autoindex;
	this->_index = other._index;
	this->_allow_methods = other._allow_methods; // GET
	this->_return = other._return;
	this->_alias = other._alias;
	this->_client_max_body_size = other._client_max_body_size;
	this->_cgi_ext = other._cgi_ext;
	this->_cgi_path = other._cgi_path;
}

Location &Location::operator=(const Location &other) {

	if (this != &other){
		this->_path = other._path;
		this->_root = other._root;
		this->_autoindex = other._autoindex;
		this->_index = other._index;
		this->_allow_methods = other._allow_methods; // GET
		this->_return = other._return;
		this->_alias = other._alias;
		this->_client_max_body_size = other._client_max_body_size;
		this->_cgi_ext = other._cgi_ext;
		this->_cgi_path = other._cgi_path;
	}
}

Location::~Location(){}



void Location::set_path(std::string new_path){
	this->_path = new_path;
}

void Location::set_root_location(std::string new_root){
	this->_root = new_root;
}

void Location::set_autoindex(std::string index){
	if (index == "on" || index == "off")
		this->_autoindex = (index == "on");
	else {
		std::cout << "config error: set_autoindex [" << index << "]" << std::endl;
		return;
	}
}

void Location::set_index(std::string new_index){
	this->_index =	new_index;
}

void Location::set_allowMethods(std::vector<std::string> allow_methods){
	this->_allow_methods.assign({false, false, false, false, false});// TODO: is it C++ 98 compatible ?
	for(size_t i = 0; i < allow_methods.size(); ++i) {
		if (allow_methods[i] == "GET")
			this->_allow_methods[0] = true;
		else if (allow_methods[i] == "POST")
			this->_allow_methods[1] = true;
		else if (allow_methods[i] == "DELETE")
			this->_allow_methods[2] = true;
		else if (allow_methods[i] == "PUT")
			this->_allow_methods[3] = true;
		else if (allow_methods[i] == "HEAD")
			this->_allow_methods[4] = true;
		else {
			std::cout << "config error: set_allowMeths [" << allow_methods[i] << "]" << std::endl;
			return;
		}
		
	}
}
// GET POST- DELETE- PUT- HEAD-
void Location::set_return(std::string new_return){
	this->_return = new_return;
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

	const char* cstr = body_size.c_str();
    char* endptr;
    unsigned long result = strtoul(cstr, &endptr, 10);
    if (*endptr != '\0') {
        std::cerr << "client_max_body_size: '" << body_size << "' is not a valid number" << std::endl;
		return;
    }
	this->_client_max_body_size = result;
}

std::string Location::get_path(){
	return this->_path;
}

std::string Location::get_root_location(){
	return this->_root;
}

bool 	Location::get_autoindex(){
	return this->_autoindex;
}

std::string Location::get_index(){
	return this->_index;
}

std::vector<bool> Location::get_allowMethods(){
	return this->_allow_methods;
}

std::string Location::get_return(){
	return this->_return;
}

std::string Location::get_alias(){
	return this->_alias;
}

std::vector<std::string>	Location::get_cgiPath(){
	return this->_cgi_path;
}

std::vector<std::string>	Location::get_cgiExt(){
	return this->_cgi_ext;
}

unsigned long Location::get_clientMaxBodySize(){
	return this->_client_max_body_size;
}
