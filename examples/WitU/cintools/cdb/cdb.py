import os.path 

class CDB:

    def __init__(self, path):
        self.path = path
        self.datafile = os.path.join(path, "data.csv") 
        self.keys = []
        self.entries = []

    def create(self):
        result = True

        os.makedirs(self.path)

        return result

    def add_entry(self, parameters):
        for param in parameters:
            if not param in self.keys:
                self.keys.append(param)
        self.entries.append(parameters)

    def __write_entry(self, parameters):
        print(parameters)

    def finalize(self):
        with open( self.datafile, "w+") as dfile:
            notFirst = False
            for key in self.keys:
                if notFirst:
                    dfile.write(",")
                else:
                    notFirst = True
                dfile.write("{}".format(key))
            dfile.write("\n")

            for entry in self.entries: 
                notFirst = False
                for key in self.keys:
                    if notFirst:
                        dfile.write(",")
                    else:
                        notFirst = True
                    if key in entry:
                        dfile.write("{}".format(entry[key]))
                dfile.write("\n")


