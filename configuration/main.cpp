
#include "ConfigParser.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    
    ConfigParser parser(argv[1]);
    
    if (!parser.parse()) {
        std::cerr << "Failed to parse configuration file." << std::endl;
        return 1;
    }
    
    std::cout << "Configuration parsed successfully:" << std::endl;
    parser.print_config();
    
    return 0;
}