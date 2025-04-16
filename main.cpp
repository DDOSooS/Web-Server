#include "WebServer.hpp"

int main() {
    WebServer* webserver = new WebServer("0.0.0.0", 8080);

    if (webserver->init() != 0) {
        return -1;
    }

    webserver->run();
    return 0;
}
