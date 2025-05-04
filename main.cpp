#include "WebServer.hpp"
#include "config/ConfigParser.hpp"
#include <thread>
int main(int argc, char *argv[]) {
    bool test_mode = false;
    std::string config_file;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" || arg == "--test") {
            test_mode = true;
        } else if (i == argc - 1) {
            config_file = arg;
        } else {
            std::cerr << "Usage: " << argv[0] << " [-t|--test] <config_file>" << std::endl;
            return 1;
        }
    }
    
    if (config_file.empty()) {
        // Use default configuration file if none specified
        config_file = "default.config";
        std::cout << "No configuration file specified, using default: " << config_file << std::endl;
    }
    
    // Test mode - just validate the configuration and exit
    if (test_mode) {
        bool valid = test_config(config_file);
        return valid ? 0 : 1;
    }
    
    // Normal server operation
    ConfigParser parser(config_file);
    
    if (!parser.parse()) {
        std::cerr << "Failed to parse configuration file." << std::endl;
        return 1;
    }
    
    // Validate configuration
    if (!parser.validate_config()) {
        std::cerr << "Configuration validation failed." << std::endl;
        return 1;
    }
    
    std::vector<ServerConfig> configs = parser.create_servers();
    std::cout << "Created " << configs.size() << " server objects." << std::endl;
    
    std::vector<WebServer*> servers;
    std::vector<std::thread> server_threads;
    
    // Create and initialize all servers
    for(size_t i = 0; i < configs.size(); ++i) {
        WebServer* server = new WebServer();
        if (server->init(configs[i]) == -1) {
            std::cerr << "Server initialization failed: " << configs[i].get_server_name() << std::endl;
            // Clean up any servers already created
            for(size_t j = 0; j < servers.size(); ++j) {
                delete servers[j];
            }
            return -1;
        }
        servers.push_back(server);
    }
    
    for(size_t i = 0; i < servers.size(); ++i) {
        server_threads.push_back(std::thread(&WebServer::run, servers[i]));
        std::cout << "Started server " << i+1 << ": " << configs[i].get_server_name() << std::endl;
    }
    
    for(size_t i = 0; i < server_threads.size(); ++i) {
        server_threads[i].join();
    }
    
    // Clean up
    for(size_t i = 0; i < servers.size(); ++i) {
        delete servers[i];
    }
    return 0;
}
