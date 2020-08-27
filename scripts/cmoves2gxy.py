import sys
import json

if len(sys.argv) != 3:
  print("syntax: python cmoves2gxy.py in.json out.json")
  sys.exit(1)

f = open(sys.argv[1])
a = json.load(f)
f.close()

r = a

if 'colormaps' in a:
  b = a['colormaps'][0]
else:
  b = a[0]

if 'points' in b:
  points = b['points']

  cmap = []
  omap = []

  for p in points:
    x = (p['x'] - points[0]['x']) / (points[-1]['x'] - points[0]['x'])
    cmap.append(x)
    cmap.append(p['r'])
    cmap.append(p['g'])
    cmap.append(p['b'])
    omap.append(x)
    omap.append(p['o'])

  oj = {}
  if 'space' in b:
    oj['ColorSpace'] = b['space']
  if 'name' in b:
    oj['Name'] = b['name']
  oj['NanColor'] = [0.0, 0.0, 0.0]
  oj['RGBPoints'] = cmap
  oj['Points'] = omap
  r = [oj]

of = open(sys.argv[2], 'w')
s = json.dumps(r, sort_keys=True, indent=4)
of.write(json.dumps(oj, sort_keys=True, indent=4))
of.close()








