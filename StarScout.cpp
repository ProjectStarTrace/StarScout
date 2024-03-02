#include "crow_all.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

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

// Function to get the first non-loopback IP address of the host machine
std::string getHostIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ipStr = "";

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;  
        if (ifa->ifa_addr->sa_family == AF_INET) { // Check it is IP4
            // is a valid IP4 Address
            void* tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0) {  // ignore loop back interface
                ipStr = addressBuffer;
                break;
            }
        } 
    }

    freeifaddrs(ifaddr);
    return ipStr;
}

int main() {
    struct stat buffer;
    if (stat(".starscout_setup", &buffer) != 0) { // Check if setup file exists
        initialSetup();
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        std::string hostIP = getHostIPAddress();
        std::string response = "<html><body><h2>Welcome to StarScout!</h2><p>Host IP: " + hostIP + "</p></body></html>";
        return response;
    });

    app.port(8080).multithreaded().run();
}
