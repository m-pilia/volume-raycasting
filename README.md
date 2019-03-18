# GPU-accelerated single-pass raycaster

![screenshot](https://user-images.githubusercontent.com/8300317/43370649-35fc6d96-9383-11e8-99b5-885f74435480.png)

This project is a simple visualiser based on GPU-accelerated single-pass
[volumetric raycasting](https://en.wikipedia.org/wiki/Volume_ray_casting),
implemented in
[GLSL](https://www.khronos.org/opengl/wiki/OpenGL_Shading_Language) and C++
with the [Qt](https://www.qt.io/) framework. It aims to provide a basic
skeleton for a visualiser that can be easily extended with a feature-rich GUI.
A brief overview of the design and implementation is described in a [blog
post](https://martinopilia.com/posts/2018/09/17/volume-raycasting.html).

Three simple examples of shaders are provided for
[isosurface](https://en.wikipedia.org/wiki/Isosurface) rendering, front-to-back
[alpha-blending](https://en.wikipedia.org/wiki/Alpha_compositing), and [maximum
intensity
projection](https://en.wikipedia.org/wiki/Maximum_intensity_projection). It
implements a simple reader for the [VTK Structured Point
legacy](http://www.cs.utah.edu/~ssingla/Research/file-formats.pdf) file format,
allowing to load volumes from file out-of-the-box.

# Build

The project can be built with [QtCreator](https://doc.qt.io/qtcreator/) or
from the command line, with [qmake](https://doc.qt.io/qt-5/qmake-manual.html)
```bash
mkdir build
cd build
qmake ../3d_raycaster.pro
make
```

# License

The software is distributed under the MIT license.
