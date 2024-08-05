Python wrappers for PICSL Convert3D Tools
=========================================

This project provides a Python interface for the [**Convert3D** tool](http://www.itksnap.org/pmwiki/pmwiki.php?n=Convert3D.Convert3D) from the [Penn Image Computing and Science Laboratory](picsl.upenn.edu), developers of [ITK-SNAP](itksnap.org).

**Convert3D** is a format conversion and multi-functional processing tool for 3D (also 2D and 4D) medical images. Complex pipelines can be created by putting commands on **Convert3D** command line in the correct order. Please see [Convert3D Reference](https://github.com/pyushkevich/c3d/blob/master/doc/c3d.md) for complete documentation.

This project makes it possible to interface with **Convert3D** from Python code. You can execute pipelines as you would on the command line, and you access images on the **Convert3D** stack as [SimpleITK](https://simpleitk.org/) image objects.

Quick Start
-----------
Install the package:

```sh
pip install picsl_c3d
```

Download an image to play with (optional of course)

```sh
curl -L http://www.nitrc.org/frs/download.php/750/MRI-crop.zip -o MRI-crop.zip
unzip MRI-crop.zip
```

Do some processing in Python

```python
from picsl_c3d import Convert3D

c = Convert3D()

c.execute('MRIcrop-orig.gipl MRIcrop-seg.gipl -lstat')
```

SimpleITK Interface
-------------------
You can read/write images from disk using the command line options passed to the `execute` command. But if you want to mix Python-based image processing pipelines and Convert3D pipelines, using the disk to store images creates unnecessary overhead. For the users of [SimpleITK](https://simpleitk.org/), additional commands are available to add images to the **Convert3D** stack and to read images from the stack.

```python
from picsl_c3d import Convert3D
import SimpleITK as sitk

c = Convert3D()

img = sitk.ReadImage('MRIcrop-orig.gipl')       # Load image with SimpleITK
c.push(img)                                     # Put image on the stack
c.execute('-smooth 1vox -resample 50% -slice z 50%')  # Make a thumbnail
img_slice = c.peek(-1)                          # Get the last image on the stack
```

Capturing and Parsing Text Output
---------------------------------
Sometimes you want to capture parts of the output from a **Convert3D**. This example redirects the output of the `execute` command to a `StringIO` buffer, and then uses regular expressions to parse out a piece of information that we want:

```python
from picsl_c3d import Convert3D
import io
import re
import numpy as np

c = Convert3D()
s = io.StringIO()                         # Create an output stream

# Run c3d "pipeline" capturing output
img='MRIcrop-orig.gipl'
c.execute(f'{img} -probe 50%', out=s)     # Run command, capturing output to s
print(f'c3d output: {s.getvalue()}')      # Print the output of the command, returns
                                          # 'Interpolated image value at -39 -55 32 is 21'

# Parse the c3d output
line = s.getvalue().split('\n')[0]        # Take first line from output
matches = re.split('( at | is )', line)   # Split the line on words "at" and "is"
pos = np.fromstring(matches[2], sep=' ')  # Store the position -39 -55 32 as Numpy array
val = float(matches[4])                   # Store the intensity 21
```
