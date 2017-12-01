"""
This program converts from the vague and fuzzily defined text based representation of which people are to be considered for the seating configuration to a better representation
"""
import sys
import json
import os

school = ""
program = ""
schoolIndex = -1
programIndex = -1
nextLineIsProgramName = True
programs = []
program2school = []
schools = []
output1 = []
output2 = []

if len(sys.argv) != 4 or "" in sys.argv:
    print("Usage: ./run.sh input-directory input")
    print(" input-directory: path to the input directory which should contain a config.json file and an input.txt file")
    exit(1)

config = json.loads(open(os.path.join(sys.argv[1], "config.json")).read())
inp = open(os.path.join(sys.argv[1], "input.txt"))


def getline():
    line = inp.readline()
    if line == "":
        return None
    line = line.strip()
    return line


while True:
    line = getline()
    if line is None:
        break

    if line == "":
        program = ""
        nextLineIsProgramName = True
        continue

    if line.startswith("Skolan för"):
        school = line
        schoolIndex += 1
        schools.append(school)
        continue

    if "ansvarig:" in line or "ansvarig :" in line:
        continue

    nameComponents = [x.strip() for x in line.strip().split('\t')]
    if len(nameComponents) >= 2 and "" not in nameComponents and ":" not in line:
        if nextLineIsProgramName:
            programIndex += 1
            nextLineIsProgramName = False
            programs.append(program)
            program2school.append(schoolIndex)

        if schoolIndex == -1 or programIndex == -1:
            print("Missing school or program names in input file")
            exit(1)

        first, last = nameComponents
        output1.append(str(programIndex))
        output2.append(str(programIndex) + "\n" + first + "\n" + last)
        continue

    if nextLineIsProgramName and ":" not in line:
        if program != "":
            program += " - "
        program += line
        continue

    print("Could not parse line '" + line + "' in input file")
    exit(1)

f1 = open(sys.argv[2], "w")
f2 = open(sys.argv[3], "w")
print(config["rows"], file=f1)
print(config["columns"], file=f1)
print(config["reservedSeats"], file=f1)
print(config["groups"], file=f1)
print(len(config["weights"]), file=f1)
for weight in config["weights"]:
    print(weight["program"], file=f1)
    print(weight["school"], file=f1)
    print(weight["size"], file=f1)
    print(weight["order"] if "order" in weight else 1, file=f1)

print(str(len(schools)), file=f1)
print(str(len(programs)), file=f1)
print(" ".join([str(x) for x in program2school]), file=f1)
print(str(len(output1)), file=f1)
print("\n".join(output1), file=f1)

print(str(len(schools)), file=f2)
print(str(len(schools)), file=f2)
print("\n".join(schools), file=f2)
print(str(len(programs)), file=f2)
print(" ".join([str(x) for x in program2school]), file=f2)
print("\n".join(programs), file=f2)
print(str(len(output2)), file=f2)
print("\n".join(output2), file=f2)
