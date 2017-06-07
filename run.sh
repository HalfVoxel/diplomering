#!/bin/bash
# Make sure the script exits if any command fails
set -e
# Create 2 temporary files
brief="$(mktemp)"
detailed="$(mktemp)"
# Take the input (from stdin) and write 2 files with different subsets of the information
# (because it is annoying to have to parse a lot of information in c++ that isn't used by that program)
python3 dipl.py $1 $brief $detailed
# Compile the c++ program
g++ -std=c++11 -Wall -Wextra -Ofast dipl.cpp
# Read the config file and the brief data file and pipe that to the c++ program
# Write the output to another temporary file.
groups="$(mktemp)"
./a.out < $brief > $groups
# Run a final python program to combine the detailed data file (which has the actual names of everyone)
# with the output from the c++ file which has their order
python3 finalize.py $detailed < $groups