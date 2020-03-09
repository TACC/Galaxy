import sys
import xml.etree.ElementTree as ET
import json

if len(sys.argv) != 2 or sys.argv[1][-3:] != 'xml':
  print('syntax: python', sys.argv[0], 'colormap.xml')
  sys.exit(1)

name = sys.argv[1].split('/')[-1][:-4]

tree = ET.parse(sys.argv[1])
root = tree.getroot()

cmap = []
omap = []

for t in root.findall('ColorMap/Point'):
  cmap = cmap + [float(t.get('x')), float(t.get('r')), float(t.get('g')), float(t.get('b'))]
  omap = omap + [float(t.get('x')), 0.0, float(t.get('o')), 0.0]

d = [{ 'ColorSpace': 'RGB',
      'Name': name,
      'Points': omap,
      'RGBPoints': cmap
    }]


o = open(name + '.json', 'w')
o.write(json.dumps(d))
o.close()
  


