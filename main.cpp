#include "./include/WebServer.hpp"
#include "./include/config/ConfigParser.hpp"
#include "./include/error/ErrorHandler.hpp"
#include "./include/error/BadRequest.hpp"
#include "./include/error/NotFound.hpp"
#include "./include/error/InternalServerError.hpp"
#include "./include/error/MethodNotAllowed.hpp"
#include "./include/error/NotImplemented.hpp"
#include "./include/error/Forbidden.hpp"


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
        std::cerr << "Configuration validation failed. Oooooops!" << std::endl;
        return 1;
    }

    std::vector<ServerConfig> configs = parser.create_servers();
    std::cout << "Created " << configs.size() << " server objects." << std::endl;

    // Just use the first config
    if (configs.empty()) {
        std::cerr << "No server configurations found." << std::endl;
        return -1;
    }
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd)) )
    {
        std::cerr << "Error getting current working directory." << std::endl;
        return -1;
    }
    for (size_t i = 0; i < configs.size(); i++)
    {
        if (configs[i].get_root().empty()) {
            std::cerr << "Error: Server root is not set in configuration." << std::endl;
            return -1;
        }
        if (configs[i].get_root()[0] != '/')
        {
            // If the root path is relative, prepend the current working directory
            configs[i].set_root(std::string(cwd) + "/" + configs[i].get_root());
        }
        else
        {
            configs[i].set_root(cwd + configs[i].get_root());
        }
    }

    // Create and initialize a single server

    WebServer server;
    if (server.init(configs[0]) == -1) {
        std::cerr << "Server initialization failed: " << configs[0].get_server_name() << std::endl;
        return -1;
    }
    server.getServerConfig().print_server_config();

    std::cout << "Started server: " << configs[0].get_server_name() << std::endl;

    // Run the server in the main thread - this will block until the server stops
    server.run();

    return 0;
    }
