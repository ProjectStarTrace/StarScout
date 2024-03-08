#ifndef SCOUT_NETWORK_UTILITIES_H
#define SCOUT_NETWORK_UTILITIES_H

#include "external_libraries/json.hpp" // Include the path to the nlohmann/json.hpp library

void grabNetworkInfo();
std::string currentDateTime();
std::string getHostIPAddress();
std::string getGeolocation();


#endif // SCOUT_NETWORK_UTILITIES_H
