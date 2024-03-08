#include "crow_all.h"
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
#include "json.hpp"

std::string getGeolocation() {
    std::array<char, 128> buffer;
    std::string result = " ";
    try{
         std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("wget -qO - ipinfo.io", "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    catch (const std::runtime_error& e) {
    std::cerr << "Error fetching geolocation: " << e.what() << '\n';
    // Handle error, such as setting a default value or exiting.
    }
   
    return result;
}

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

// Function to get the first non-loopback IP address of the host machine
std::string getHostIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ipStr = "";

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;  
        if (ifa->ifa_addr->sa_family == AF_INET) { // Check it is IP4
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

// Function to read the Scout ID from file
std::string readScoutID() {
    std::string scoutID;
    std::ifstream scoutIDFile(".starscout_id");
    std::getline(scoutIDFile, scoutID);
    return scoutID;
}

// Helper function to get current datetime as string
std::string currentDateTime() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string s(30, '\0');
    std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return s.substr(0, s.find('\0'));
}

// Function to run iperf test and update files
void runIperfTest() {
    std::string hostIP = getHostIPAddress(); // Assume getHostIPAddress() is defined elsewhere in your code

    while (true) {
        // Execute the iperf test and capture its output
        std::string command = "iperf -c lon.speedtest.clouvider.net -p 5201 2>&1";
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        // Extract the bandwidth from the iperf output
        std::regex bandwidthRegex(R"(\s+(\d+\sMbits/sec))");
        std::smatch matches;
        std::string bandwidth = "(no result)";
        if (std::regex_search(result, matches, bandwidthRegex) && matches.size() > 1) {
            bandwidth = matches[1].str();
            std::cout << "Extracted bandwidth: " << bandwidth << std::endl;
        } 
        else {
            std::cout << "Bandwidth extraction failed." << std::endl;
        }

        std::string geolocationInfo = "";
        geolocationInfo = getGeolocation(); // Fetch geolocation information in real-time
        
        try {
           
            auto jsonGeo = nlohmann::json::parse(geolocationInfo);

            // Use a lambda to simplify null or missing check and retrieval
            auto getJsonStringValue = [&jsonGeo](const std::string& key) {
                return jsonGeo.contains(key) && !jsonGeo[key].is_null() ? jsonGeo[key].get<std::string>() : "Unavailable";
            };

            // Extract fields safely
            std::string ip,hostname,city, region,country,location = "";

            ip = getJsonStringValue("ip");
            hostname = getJsonStringValue("hostname");
            city = getJsonStringValue("city");
            region = getJsonStringValue("region");
            country = getJsonStringValue("country");
            location = getJsonStringValue("loc"); // Contains both latitude and longitude

            // Write to StarScoutStats.txt - overwrites file each time with latest info
            {
                std::ofstream statsFile("StarScoutStats.txt", std::ios::trunc);
                statsFile << "Last IPERF Test Result: " << bandwidth << "\n"
                          << "Public IP Address: " << ip << "\n"
                          << "Hostname: " << hostname << "\n"
                          << "Location: " << city << ", " << region << ", " << country << "\n"
                          << "Coordinates: " << location << std::endl;
            }

            // Update StarScoutDatapoints.csv as before...
            std::ifstream testEmpty("StarScoutDatapoints.csv");
            bool isEmpty = testEmpty.peek() == std::ifstream::traits_type::eof();
            testEmpty.close();

            std::ofstream datapointsFile("StarScoutDatapoints.csv", std::ios::app);
            if (isEmpty) {
                datapointsFile << "DateTime,IP Address,Bandwidth,Hostname,City,Region,Country,Coordinates\n";
            }

            // Append new data
            datapointsFile << currentDateTime() << ","
                           << ip << ","
                           << bandwidth << ","
                           << hostname << ","
                           << city << ","
                           << region << ","
                           << country << ","
                           << location << "\n";

        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << '\n';
        } catch (const nlohmann::json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cerr << "Standard exception: " << e.what() << '\n';
        }

        // Wait for 30 minutes before running the test again
        std::this_thread::sleep_for(std::chrono::minutes(30));
    }
}


int main() {
    struct stat buffer;
    if (stat(".starscout_setup", &buffer) != 0) { // Check if setup file exists
        initialSetup();
    }

    // Start iperf test thread
    std::thread iperfThread(runIperfTest);
    iperfThread.detach(); // Detach the thread so the application doesn't wait for it

    crow::SimpleApp app;


    CROW_ROUTE(app, "/")([]() {
    std::string hostIP = getHostIPAddress();
    std::string scoutID = readScoutID();
    std::string bandwidth = "Unknown"; // Default value
    std::string hostname, city, region, country, location;

    // Read and extract data from StarScoutStats.txt
    std::ifstream statsFile("StarScoutStats.txt");
    std::string line;
    if (statsFile.is_open()) {
        while (getline(statsFile, line)) {
            if (line.find("Public IP Address:") != std::string::npos) {
                hostIP = line.substr(line.find(":") + 2);
            } else if (line.find("Hostname:") != std::string::npos) {
                hostname = line.substr(line.find(":") + 2);
            } else if (line.find("Location:") != std::string::npos) {
                auto locDelim = line.find(":") + 2;
                city = line.substr(locDelim, line.find(",", locDelim) - locDelim);
                auto regionDelim = line.find(",", locDelim) + 2;
                region = line.substr(regionDelim, line.find(",", regionDelim) - regionDelim);
                country = line.substr(line.rfind(",") + 2);
            } else if (line.find("Coordinates:") != std::string::npos) {
                location = line.substr(line.find(":") + 2);
            } else if (line.find("Last IPERF Test Result:") != std::string::npos) {
                bandwidth = line.substr(line.find(":") + 2);
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
    
    std::string placeholders[] = {"{{hostIP}}", "{{scoutID}}", "{{hostname}}", "{{city}}", "{{region}}", "{{country}}", "{{location}}", "{{bandwidth}}"};
    std::string values[] = {hostIP, scoutID, hostname, city, region, country, location, bandwidth};

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
