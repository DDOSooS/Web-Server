#include "ConfigParser.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
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
    
    // Create server objects
    std::vector<ServerConfig> servers = parser.create_servers();
    std::cout << "Created " << servers.size() << " server objects." << std::endl;
    
    parser.print_config();
    
    return 0;
}