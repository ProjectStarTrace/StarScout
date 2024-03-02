#include "crow.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

// Function to check if iperf is installed
bool isIperfInstalled() {
    return system("which iperf > /dev/null 2>&1") == 0;
}

// Function to install iperf
void installIperf() {
    std::cout << "iperf not found. Installing iperf..." << std::endl;
    if (system("sudo apt-get update && sudo apt-get install -y iperf") != 0) {
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

int main() {
    struct stat buffer;
    if (stat(".starscout_setup", &buffer) != 0) { // Check if setup file exists
        initialSetup();
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "<html><body><h2>Welcome to StarScout</h2></body></html>";
    });

    app.port(8080).multithreaded().run();
}
