#include "TcpListner.hpp"

int main() {
    TcpListner* server = new TcpListner("0.0.0.0", 1342);
    server->init();
    server->run();
    return (0);
}