#!/bin/bash

# Update package lists
sudo apt-get update
sudo apt-get install libboost-all-dev

#Download Crow C Library
wget https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h