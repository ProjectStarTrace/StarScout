#!/bin/bash

# Update package lists
sudo apt-get update
sudo apt-get install libboost-all-dev
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install curl

#Download Crow C Library
wget https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h