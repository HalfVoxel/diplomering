import sys

f = open(sys.argv[1])
f.readline()
schools = int(f.readline())
for i in range(0, schools):
    f.readline()
programs = int(f.readline())
f.readline()
for i in range(0, programs):
    f.readline()
numPersons = int(f.readline())
persons = []
for i in range(0, numPersons):
    program = int(f.readline().strip())
    firstName = f.readline().strip()
    lastName = f.readline().strip()
    persons.append(firstName + " " + lastName)

longestName = max([len(s) for s in persons])

while True:
    s = sys.stdin.readline()
    if s == "":
        break
    if s.strip() == "":
        continue
    line = s.split()
    assert(line[0] == "Group")
    groupNumber = int(line[1])
    side = line[2] == "right"
    groupSize = int(line[3])
    remaining = groupSize
    print("Group " + str(groupNumber) + " (enters from the " + ("right" if side else "left") + ")")
    while remaining > 0:
        nums = [int(x) for x in sys.stdin.readline().strip().split()]
        remaining -= len(nums)
        names = [persons[num].ljust(longestName) for num in nums]
        print(" ".join(names))
    print()
