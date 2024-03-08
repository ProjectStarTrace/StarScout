#!/bin/bash

# Update package lists
sudo apt-get update
sudo apt-get install libboost-all-dev
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install curl
curl -s https://packagecloud.io/install/repositories/ookla/speedtest-cli/script.deb.sh | sudo bash
sudo apt-get install speedtest

#Download Crow C Library
wget https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h