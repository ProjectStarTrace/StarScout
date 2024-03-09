#include "ScoutNetworkUtilities.h"
#include "external_libraries/crow_all.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>
#include <regex>
#include <memory>
#include <stdexcept>
#include <array>
#include "external_libraries/json.hpp"

// Function to check if iperf is installed
bool isIperfInstalled() {
    return system("which iperf > /dev/null 2>&1") == 0;
}

bool isSpeedtestInstalled() {
    return system("which speedtest") == 0;
}

bool isPythonInstalled() {
    return system("which python3") == 0;
}

// Function to install iperf
void installIperf() {
    std::cout << "iperf not found. Installing iperf..." << std::endl;
    if (system("sudo apt-get update && sudo apt-get install -y iperf") != 0) {
        std::cerr << "Failed to install iperf. Please install it manually." << std::endl;
        exit(1); // Exit if installation fails
    }
}

// Function to install iperf
void installSpeedtest() {
    std::cout << "Speedtest CLI not found. Installing Speedtest CLI.." << std::endl;
    if (system("sudo apt-get install speedtest-cli") != 0) {
        std::cerr << "Failed to install Speedtest. Please install it manually." << std::endl;
        exit(1); // Exit if installation fails
    }
}

// Function to install Python
void installPython() {
    std::cout << "Python not found. Installing Python.." << std::endl;
    if (system("sudo apt-get install python3") != 0) {
        std::cerr << "Failed to install Python. Please install it manually." << std::endl;
        exit(1); // Exit if installation fails
    }
}

// Function to install Google-Auth
void installGoogleAuth() {
    std::cout << "Ensuring Google Auth is installed.." << std::endl;
    if (system("sudo apt install python3-google-auth python3-google-auth-oauthlib python3-google-auth-httplib2") != 0) {
        std::cerr << "Failed to install Google Auth through PIP. Please install it manually." << std::endl;
        exit(1); // Exit if installation fails
    }
}

void installPIP3() {
    std::cout << "Ensuring PIP3 is installed.." << std::endl;
    if (system("sudo apt install python3-pip") != 0) {
        std::cerr << "Failed to install PIP3 Please install it manually. (sudo apt install python3-pip)" << std::endl;
        exit(1); // Exit if installation fails
    }
}


// Function to generate a unique Scout ID
std::string generateScoutID(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string scoutID;
    for (size_t i = 0; i < length; ++i) {
        scoutID += chars[distribution(generator)];
    }
    return scoutID;
}

// Function to perform initial setup, including dependency checks and Scout ID generation
void initialSetup() {
    std::cout << "Welcome to StarScout! Please wait while we get things setup from here...." << std::endl;
    if (!isIperfInstalled()) {
        installIperf();
    }

    if (!isSpeedtestInstalled()) {
        installSpeedtest();
    }

    if (!isPythonInstalled()) {
        installPython();
    }

    installPIP3();
    installGoogleAuth(); //ensures that Google Auth is Installed.

    // Generate and save Scout ID
    std::string scoutID = generateScoutID(12);
    std::ofstream scoutIDFile(".starscout_id");
    scoutIDFile << scoutID << std::endl;
    scoutIDFile.close();

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

    // Start iperf test thread
    std::thread iperfThread(grabNetworkInfo);
    iperfThread.detach(); // Detach the thread so the application doesn't wait for it

    crow::SimpleApp app;

  

    CROW_ROUTE(app, "/")([]() 
    {
    std::string hostIP = getHostIPAddress();
    std::string scoutID = readScoutID();
    std::string download = "Waiting for Result"; // Default value
    std::string upload = "Waiting for Result"; // Default value
    std::string city, region, country, location;

    // Read and extract data from StarScoutStats.txt
    std::ifstream statsFile("StarScoutStats.txt");
    std::string line;
    if (statsFile.is_open()) {
        while (getline(statsFile, line)) {
            if (line.find("Public IP Address:") != std::string::npos) {
                hostIP = line.substr(line.find(":") + 2);
            } else if (line.find("Location:") != std::string::npos) {
                auto locDelim = line.find(":") + 2;
                city = line.substr(locDelim, line.find(",", locDelim) - locDelim);
                auto regionDelim = line.find(",", locDelim) + 2;
                region = line.substr(regionDelim, line.find(",", regionDelim) - regionDelim);
                country = line.substr(line.rfind(",") + 2);
            } else if (line.find("Coordinates:") != std::string::npos) {
                location = line.substr(line.find(":") + 2);
            } else if (line.find("Download:") != std::string::npos) {
                download = line.substr(line.find(":") + 2);
            }else if (line.find("Upload:") != std::string::npos) {
                upload = line.substr(line.find(":") + 2);
            }
        }
        statsFile.close();
    }

    // Read iperf results
    std::ifstream iperfResultsFile(".iperf_results");
    std::string iperfResults((std::istreambuf_iterator<char>(iperfResultsFile)), std::istreambuf_iterator<char>());

    std::ifstream file("home.html");
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string content = buffer.str();
    
    std::string placeholders[] = {"{{hostIP}}", "{{scoutID}}", "{{city}}", "{{region}}", "{{country}}", "{{location}}", "{{download}}","{{upload}}"};
    std::string values[] = {hostIP, scoutID, city, region, country, location, download, upload};

    for (size_t i = 0; i < sizeof(placeholders)/sizeof(placeholders[0]); ++i) {
        size_t pos = content.find(placeholders[i]);
        while (pos != std::string::npos) {
            content.replace(pos, placeholders[i].length(), values[i]);
            pos = content.find(placeholders[i], pos + values[i].length());
        }
    }

  

    return content;
});
    
    

    app.port(8080).multithreaded().run();
}
