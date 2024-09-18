# Building GITS

## Prerequisites

You will need:
- [Git](https://git-scm.com/)
- [Git LFS](https://git-lfs.com/)
- A C++ compiler (MSVC on Windows, GCC on Linux; Clang support is planned)
- A build system (Make, Visual Studio, Ninja, etc.)
- [CMake](https://cmake.org/)
- [Python 3](https://www.python.org/)
- [Mako](https://www.makotemplates.org/) (a Python template library, `pip install Mako`)

### Linux

On Linux you will also need:
- wget
- libx11 development headers (`libx11-dev` on Ubuntu)
- libx11-xcb development headers (`libx11-xcb-dev` on Ubuntu)
- libwayland development headers (`libwayland-dev` on Ubuntu)
- OpenGL development headers (`libgl-dev` on Ubuntu)

## Building

### Install Git LFS
- https://docs.github.com/en/repositories/working-with-files/managing-large-files/installing-git-large-file-storage

### Clone the repo

```bash
git clone https://github.com/intel/gits
cd gits
```

### Generate a project and build it

Examples are provided below. You might need to adjust them. (E.g., 64- vs 32-bit, unusual Python locations, different build systems like Ninja...)

Windows 64-bit example:
```batch
mkdir Release\x64
cd Release\x64
cmake -A x64 ..\..\
cmake --build . --config Release
```

Linux 64-bit example:
```bash
mkdir -p Release/x64
cd Release/x64
cmake -DARCH=-m64 ../../
# The `-j` option without an argument sets the number of threads automatically.
# You can also set it manually, e.g., `-j8` for 8 threads.
# Lower thread counts decrease RAM usage, which can help with OOM errors.
cmake --build . --config Release -j
```

Expect some warnings, we still haven't fixed them yet; it's a known issue.

## Post-build steps

### Install

This will install GITS into the folder indicated by the install prefix we set during building. (`dist` in the example.)
```bash
cmake --install . --config Release --prefix dist
```
Installing in this case means copying files into their destinations and setting paths in config. If you move the install folder somewhere else, you will need to manually adjust paths in config files.
