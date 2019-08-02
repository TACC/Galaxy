import re
import sys

if (len(sys.argv) == 1):
    print("")
    print("vectors2csv.py script")
    print("")
    print("  script will convert output from the GradientSampleVis object to a")
    print("  csv file readable by ParaView and other applications")
    print("")
    print("  USAGE:")
    print("    python vectors2csv.py <input file> > output.csv")
    print("")
    exit(0)

elif (len(sys.argv) < 2):
    print("ERROR: must include: <input filename> ...")
    print("")
    print("  python vectors2csv.py <input>")
    print("")
    exit(1)

inputfile = "results.csv"

vectors = []
newvector = False
newvalues = []
with open(inputfile, "r") as infile:
    for line in infile:
        if re.search("^VECTOR", line):
            newvector = True
            vectors.append(newvalues)
        else:

            # find the valid values
            match = re.search("\[+(.*)\]+", line)
            values = match.group(1).strip().split(",")
            
            valid = []
            for v in values:
                if not re.search("^\(", v): 
                    valid.append(v)

            if newvector:
                newvalues = []
                newvector = False
                for i in range(len(valid)):
                    newvalues.append([])

            for i in range(len(valid)):
                newvalues[i].append(valid[i])

for v in vectors:
    for vec in v:
        print(",".join(vec))

