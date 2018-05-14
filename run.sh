#!/bin/bash
# Make sure the script exits if any command fails
set -e

# Create 2 temporary files
# The weird syntax is to be able to support running on mac and linux
# Really the only thing it does is set $tmpdir to the path of a new temporary directory
tmpdir=`mktemp -d 2>/dev/null || mktemp -d -t 'diplomering'`
brief="$tmpdir/brief"
detailed="$tmpdir/detailed"
# Take the input (from the file given by the first parameter) and write 2 files with different subsets of the information
# (because it is annoying to have to parse a lot of information in c++ that isn't used by that program)
python3 dipl.py $1 $brief $detailed
# Compile the c++ program
g++ -std=c++11 -Wall -Wextra -Ofast dipl.cpp
# Read the config file and the brief data file and pipe that to the c++ program
# Write the output to another temporary file.
groups="$tmpdir/groups"
./a.out < $brief > $groups
# Run a final python program to combine the detailed data file (which has the actual names of everyone)
# with the output from the c++ file which has their order
python3 finalize.py $detailed < $groups
