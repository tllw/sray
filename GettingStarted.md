S-ray should compile and run on any UNIX system with an X display.

# Getting the code #
There's no official release at the moment, just grab the code from our subversion repository (obviously you need a subversion client for that):
```
$ svn co http://sray.googlecode.com/svn/trunk sray
```

# Dependencies #
  * GNU make
  * expat (http://expat.sourceforge.net)
  * libvmath (http://gfxtools.sourceforge.net)
  * libimago (http://gfxtools.sourceforge.net)
  * kdtree (http://code.google.com/p/kdtree)
  * pkg-config (http://pkg-config.freedesktop.org)

# Compiling #
First download and install all dependencies listed above. Then just change into the s-ray directory and type make (or gmake on non-GNU systems).

# Usage #
Type ./sray -h to get usage information.
A few [test scenes](http://goat.mutantstargoat.com/~nuclear/tmp/sray-moreexamples.tar.gz) are provided to get you started.


# Tools #
In the s-ray source tree there are also some supporting tools that you can compile separately:
  * `prev` is an interactive OpenGL scene file previewer.
  * `obj2sray` is a converter from Wavefront OBJ files to the native s-ray XML scene description.
  * `3ds2sray` is a converter from Autodesk 3DS files to the native s-ray XML scene description.