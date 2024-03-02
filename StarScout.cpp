#include "CivetServer.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

// Function to check if iperf is installed
bool isIperfInstalled() {
    if (system("which iperf > /dev/null 2>&1") == 0) {
        return true; // iperf is installed
    } else {
        return false; // iperf is not installed
    }
}

// Function to install iperf
void installIperf() {
    std::cout << "iperf not found. Installing iperf..." << std::endl;
    // Use system call to install iperf. This requires superuser privileges.
    int result = system("sudo apt-get update && sudo apt-get install -y iperf");
    if (result != 0) {
        std::cerr << "Failed to install iperf. Please install it manually." << std::endl;
        exit(1); // Exit if installation fails
    }
}

// Function to perform initial setup, including dependency checks
void initialSetup() {
    std::cout << "Welcome to StarScout!. Please wait while we get things setup from here...." << std::endl;

    if (!isIperfInstalled()) {
        installIperf();
    }

    // Create a setup file to indicate completion of the setup process
    std::ofstream setupFile(".starscout_setup");
    setupFile << "Setup completed." << std::endl;
    setupFile.close();
}

class HelloWorldHandler : public CivetHandler {
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
        mg_printf(conn, "<html><body><h2>Welcome to StarScout</h2></body></html>\n");
        return true;
    }
};

int main() {
    struct stat buffer;
    if (stat(".starscout_setup", &buffer) != 0) { // Check if setup file exists
        initialSetup();
    }

    const char *options[] = {
        "listening_ports", "8080",
        nullptr
    };

    CivetServer server(options);
    HelloWorldHandler handler;
    server.addHandler("/", handler);

    std::cout << "Server started on port 8080\n";
    while (true) {
        sleep(10);
    }

    return 0;
}
