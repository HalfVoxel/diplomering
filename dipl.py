import sys

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


def getline():
    line = sys.stdin.readline()
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

    if "ansvarig:" in line:
        continue

    nameComponents = [x.strip() for x in line.split('\t')]
    if len(nameComponents) >= 2 and nameComponents[0] != "" and nameComponents[1] != "":
        if nextLineIsProgramName:
            programIndex += 1
            nextLineIsProgramName = False
            programs.append(program)
            program2school.append(schoolIndex)

        if schoolIndex == -1 or programIndex == -1:
            print("Missing school or program names")
            exit(1)

        first, last = nameComponents
        output1.append(str(programIndex))
        output2.append(str(programIndex) + "\n" + first + "\n" + last)
        continue

    if nextLineIsProgramName:
        if program != "":
            program += " - "
        program += line
        continue

        print("Could not parse line '" + line + "'")
        exit(1)

f1 = open(sys.argv[1], "w")
f2 = open(sys.argv[2], "w")
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
