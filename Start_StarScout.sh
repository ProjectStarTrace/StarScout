#!/bin/bash

# Start StarScout in the current directory in the background and ignore hangup signals
nohup ./StarScout &> StarScout.log &
