#include "ConfigParser.hpp"
#include "Location.hpp"
#include <map>
#include "../include/WebServer.hpp"

// Server parameters
class Server {
	private:
		uint16_t						_port;
		in_addr_t						_host;
		std::string						_server_name;
		std::string						_root;
		unsigned long					_client_max_body_size;
		std::string						_index;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<Location> 			_locations;
		struct sockaddr_in 				_server_address;
		int     						_listen_fd;


	public:
		Server();
		Server(const Server &other);
		Server &operator=(const Server &rhs);
		~Server();

		uint16_t						get_port();
		in_addr_t						get_host();
		std::string						get_server_name();
		std::string						get_root();
		unsigned long					get_client_max_body_size();
		std::string						get_index();
		bool							get_autoindex();
		std::map<short, std::string>	get_error_pages();
		std::vector<Location> 			get_locations();
		struct sockaddr_in 				get_server_address();
		int     						get_listen_fd();



		void set_port(std::string param);
		void set_host(std::string param);
		void set_server_name(std::string param);
		void set_root(std::string param);
		void set_client_max_body_size(std::string param);
		void set_index(std::string param);
		void set_autoindex(std::string param);
		void set_error_pages(std::string param);
		void set_locations(std::string param);
		void set_server_address(std::string param);
		void set_listen_fd(int sock_fd);
};
