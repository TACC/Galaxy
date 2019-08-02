# Tools for galaxy debugging, etc.

```plot_vectors.py``` Given a csv file of vectors, plot them. 

- example: ```python plot_vectors.py vectors.csv```
- The file is a properly formatted ```csv``` file, with the following values (the vector is plotted from ```origin``` to ```origin + delta```)

```
        x origin, y origin, z origin, x delta, y delta, z delta
```

```paraview_vectors.py``` creates an image of a set of vectors from a ```csv``` file. 
- example: ```python paraview_vectors.py vectors.csv vectors.png```
- Each line of the ```csv``` file defines a vector, with the columns being these values. There is no header line.

```
        x origin, y origin, z origin, x delta, y delta, z delta
```

```vectors2csv.py``` Converts output from GradientSampleVis to ```csv``` format required for above scripts.

- example: ```python vectors2csv.py results.txt > vectors.csv```