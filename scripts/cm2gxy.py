import json
import sys
import os.path

if (len(sys.argv) < 4): 
  print('\nTranslate a colormap from Color Moves\'s JSON format to Galaxy\'s RGBPoints format.\n'
    + 'Usage: ' + sys.argv[0] + ' < xml file > < data min > < data max >\n')
  exit()

infile = sys.argv[1]
json_file = open(infile)
json_data = json.load(json_file)

data_min = float(sys.argv[2])
data_max = float(sys.argv[3])
scale = data_max - data_min

cmap_name, ext = os.path.splitext(infile) 
outfile = "cmap-" + infile

with open(outfile, 'w') as out_fp:
  out_fp.write('[ { "ColorSpace" : "RGB","Name" : "' + cmap_name + '", "RGBPoints" : [ \n')
  last_point=''  
  for point in (json_data['colormaps'][0]['points']):
    x = str.format('{0:.6f}', float(point["x"]) * scale)
    r = str.format('{0:.6f}', point['r'])
    g = str.format('{0:.6f}', point['g'])
    b = str.format('{0:.6f}', point['b'])
    if last_point != '': 
      out_fp.write(last_point + ',\n')
    last_point = x + ',' + r + ',' + g + ',' + b
  out_fp.write(last_point + '\n ], \n"Points" : [ ' + str(data_min) + ', 0.0, 0.5, 0.0, ' + str(data_min + 0.25 * scale) + ', 0.0, 0.5, 0.0, ' + str(data_max) + ', 0.05, 0.5, 0.0 ] } ]')


