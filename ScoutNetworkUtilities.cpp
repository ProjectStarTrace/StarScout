#include "ScoutNetworkUtilities.h"
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
#include "json.hpp" // Ensure this path is correct based on your project's structure


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

// Helper function to get current datetime as string
std::string currentDateTime() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string s(30, '\0');
    std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return s.substr(0, s.find('\0'));
}

// Function to run iperf test and update files
void grabNetworkInfo() {
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
