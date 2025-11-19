---
icon: simple/compilerexplorer
title: Build GITS
---
# Prerequisites

You will need:

- [Git](https://git-scm.com/)
- [Git LFS](https://git-lfs.com/)
- A **C++ compiler** ([MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/) on Windows, [GCC](https://gcc.gnu.org/) on Linux; Clang support is planned)
- A **build system** (Make, Visual Studio, Ninja, etc.)
- [CMake](https://cmake.org/)
- [Python 3](https://www.python.org/)

## :simple-linux: Linux

On Linux you will also need:

- **wget**
- Various development headers:
  - **libx11** ([`libx11-dev`](https://packages.debian.org/sid/libx11-dev) on Ubuntu)
  - **libx11-xcb** ([`libx11-xcb-dev`](https://packages.debian.org/sid/libx11-xcb-dev) on Ubuntu)
  - **libwayland** ([`libwayland-dev`](https://packages.debian.org/sid/libwayland-dev) on Ubuntu)
  - **OpenGL** ([`libgl-dev`](https://packages.debian.org/sid/libgl-dev) on Ubuntu)

# Clone the Repository

```bash
git clone https://github.com/intel/gits
git lfs pull
cd gits
```

# Install Python Modules

The required Python modules are specified in the `requirements.txt` file (found on the root of the repository):

```bash
pip install -r requirements.txt
```

Additionally, you may want to use mkdocs-material to work easier with GITS documentation, in which case you would also install modules specified in the requirements.txt file found in Scripts/docs directory:

```bash
pip install -r Scripts/docs/requirements.txt
```

# Generate and Build the Project

Below you can find examples how to build **GITS** for the most common cases. For other configurations (e.g. 32-bit binaries, unusual Python locations, different build systems like Ninja...) please check the relevant documentation on how you need to adjust the commands.

=== "Windows 64-bit Rel."
	```batch
	mkdir build
	cd build
	cmake -A x64 ..\
	cmake --build . --config Release
	```

=== "Linux 64-bit Rel."
	```bash
	mkdir -p build
	cd build
	cmake -DARCH=-m64 ../
	# The `-j` option without an argument sets the number of threads automatically.
	# You can also set it manually, e.g., `-j8` for 8 threads.
	# Lower thread counts decrease RAM usage, which can help with OOM errors.
	cmake --build . --config Release -j
	```

# Install Binaries

Before you can use **GITS** you need to perform the _install step_. Installing in this case means copying files into their destinations and setting up all paths in the configuration file.

To install **GITS** in the folder `<gits-root-folder>\build\dist` use the following command:

```bash
cd build
cmake --install . --config Release --prefix dist
```

> Note:
> If you move the install folder somewhere else, you will need to manually adjust paths in config files.

Find how to use GITS [here](usage.md).
