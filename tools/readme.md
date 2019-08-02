# Tools for galaxy debugging, etc.

- ```plot_vectors.py``` Given a csv file of vectors, plot them. 

    - example: ```python plot_vectors.py vectors.csv```

    - The file is a properly formatted ```csv``` file, with the following values (the vector is plotted from ```origin``` to ```origin + delta```)

```
x origin, y origin, z origin, x delta, y delta, z delta
```

