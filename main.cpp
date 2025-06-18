#include "./include/WebServer.hpp"
#include "./include/config/ConfigParser.hpp"
#include "./include/config/ServerConfig.hpp"


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
    try {
        ConfigParser parser(config_file);
        if (!parser.parse()) {
            std::cerr << "Failed to parse configuration file." << std::endl;
            return 1;
        }
        
        // Validate configuration
        if (!parser.validate_config()) {
            std::cerr << "Configuration validation failed. Oooooops!" << std::endl;
            return 1;
        }
        
        std::vector<ServerConfig> configs = parser.create_servers();
        std::cout << "Created [" << configs.size() << "] server configuration(s)!" << std::endl;
        
        // Check if we have any server configurations
        if (configs.empty()) {
            std::cerr << "No server configurations found." << std::endl;
            return 1;
        }
   
        std::cout << "[DEBUG] *********** Configuration parsing completed successfully" << std::endl;
        
        // Print summary of server configurations
        std::cout << "\n=== Server Configuration Summary ===" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "Server " << (i + 1) << ": " 
                      << configs[i].get_server_name() 
                      << " (http://" << configs[i].get_host() 
                      << ":" << configs[i].get_port() << ")" << std::endl;
        }
        std::cout << "===================================\n" << std::endl;
        
        // Create and initialize a single WebServer with all configurations
        WebServer webServer;
        
        if (webServer.init(configs) != 0) {
            std::cerr << "WebServer initialization failed!" << std::endl;
            return 1;
        }

        std::cout << "WebServer initialized successfully with " << configs.size() << " server(s)" << std::endl;
        
        // Run the multi-server WebServer - this will block until the server stops
        std::cout << "Starting WebServer..." << std::endl;
        int result = webServer.run();
        
        if (result == 0) {
            std::cout << "WebServer shut down gracefully." << std::endl;
        } else {
            std::cerr << "WebServer encountered an error during execution." << std::endl;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main" << std::endl;
        return 1;
    }
}
