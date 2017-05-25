#!/bin/bash
python3 dipl.py till_programmet.brief till_programmet.detailed
g++ -std=c++11 -Wall -Wextra -O2 dipl.cpp
cat config.in till_programmet.brief | ./a.out > groups.out
python3 finalize.py till_programmet.detailed < groups.out