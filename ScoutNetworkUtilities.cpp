#include "FirebaseUploader.h"
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
#include "external_libraries/json.hpp" // Ensure this path is correct based on your project's structure


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

std::string getAccessToken(const std::string& serviceAccountPath) {
    std::string command = "python3 get_access_token.py " + serviceAccountPath;
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Assuming the output is the token directly, trim any newline or whitespace
    result.erase(0, result.find_first_not_of(" \n\r\t")); // Trim left
    result.erase(result.find_last_not_of(" \n\r\t")+1);   // Trim right

    return result;
}


std::string readScoutID() {
    std::string scoutID;
    std::ifstream scoutIDFile(".starscout_id");
    std::getline(scoutIDFile, scoutID);
    return scoutID;
}

// Function to run iperf test and update files
void grabNetworkInfo() {
    std::string hostIP = getHostIPAddress(); // Assume getHostIPAddress() is defined elsewhere in your code

    while (true) {
       // Execute the speedtest-cli test and capture its output
        std::string command = "speedtest --json 2>&1"; // Use JSON format for easier parsing
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        double downloadSpeed = 0; // Initialize with a default value
        double uploadSpeed = 0; // Initialize with a default value

         try {
            auto jsonResult = nlohmann::json::parse(result);

            // Assuming the JSON structure, extract download and upload speeds
            downloadSpeed = jsonResult["download"]; // This is in bytes per second
            uploadSpeed = jsonResult["upload"]; // This is in bytes per second

            // Convert speeds from bytes per second to Mbits/sec for consistency
            downloadSpeed = (downloadSpeed * 8) / (1000 * 10000); // Convert bytes to bits and then to Mbits
            uploadSpeed = (uploadSpeed * 8) / (1000 * 10000); // Convert bytes to bits and then to Mbits

            std::cout << "Download Speed: " << downloadSpeed << " Mbits/sec" << std::endl;
            std::cout << "Upload Speed: " << uploadSpeed << " Mbits/sec" << std::endl;
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "JSON parsing failed: " << e.what() << std::endl;
            // Handle parsing failure
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
            std::string ip,city,region,country,location = "";

            ip = getJsonStringValue("ip");
            city = getJsonStringValue("city");
            region = getJsonStringValue("region");
            country = getJsonStringValue("country");
            location = getJsonStringValue("loc"); // Contains both latitude and longitude

            // Write to StarScoutStats.txt - overwrites file each time with latest info
            {
                std::ofstream statsFile("StarScoutStats.txt", std::ios::trunc);
                statsFile << "Download: " << downloadSpeed << "\n"
                          << "Upload: " << uploadSpeed << "\n"
                          << "Public IP Address: " << ip << "\n"
                          << "Location: " << city << ", " << region << ", " << country << "\n"
                          << "Coordinates: " << location << std::endl;
            }

            // Update StarScoutDatapoints.csv as before...
            std::ifstream testEmpty("StarScoutDatapoints.csv");
            bool isEmpty = testEmpty.peek() == std::ifstream::traits_type::eof();
            testEmpty.close();

            std::ofstream datapointsFile("StarScoutDatapoints.csv", std::ios::app);
            if (isEmpty) {
                datapointsFile << "DateTime,IP Address,downloadSpeed,uploadSpeed,City,Region,Country,Coordinates\n";
            }

            // Append new data
            datapointsFile << currentDateTime() << ","
                           << ip << ","
                           << downloadSpeed << ","
                           << uploadSpeed << ","
                           << city << ","
                           << region << ","
                           << country << ","
                           << location << "\n";


    std::string projectID = "startrace-81336";
    std::string collection = "starscoutData";
    std::string serviceAccountPath = "startrace-81336-ef5a4476c9d1.json";
    std::string accessToken = "";
    
    std::string scoutID = readScoutID();

    //FireStore Upload
    try {
        accessToken = getAccessToken(serviceAccountPath);
        std::cout << "Access Token: " << accessToken << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    FirebaseUploader uploader(projectID, collection, scoutID, currentDateTime());
    

    // Extract latitude and longitude from location
    double latitude = 0.0, longitude = 0.0;

    try {
        std::string delimiter = ",";
        size_t pos = location.find(delimiter);
        latitude = std::stod(location.substr(0, pos)); // Convert first part to double
        longitude = std::stod(location.substr(pos + 1, location.length())); // Convert second part to double
    } catch (const std::exception& e) {
        std::cerr << "Exception when parsing coordinates: " << e.what() << std::endl;
        // Handle error, such as setting a default value or logging
    }

    json data = 
    {
        {"fields", {
            {"ScoutID", {{"stringValue", scoutID}}},
            {"DateTime", {{"stringValue", currentDateTime()}}},
            {"HostIP", {{"stringValue", hostIP}}},
            {"DownloadSpeed", {{"doubleValue", downloadSpeed}}},
            {"UploadSpeed", {{"doubleValue", uploadSpeed}}},
            {"City", {{"stringValue", city}}},
            {"Region", {{"stringValue", region}}},
            {"Country", {{"stringValue", country}}},
            {"geolocation", {{"geoPointValue", {{"latitude", latitude}, {"longitude", longitude}}}}}

        }}
    };

    std::cout << "\n\n Data to be uploaded\n\n\n" << std::endl;
    std::cout << "JSON Payload: " << data.dump() << std::endl;
    uploader.uploadData(data,accessToken);

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
