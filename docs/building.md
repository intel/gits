---
hide:
  - navigation
---
# Building GITS

## Prerequisites

You will need:

- [Git](https://git-scm.com/)
- A **C++ compiler** ([MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/){:target="_blank"} on Windows, [GCC](https://gcc.gnu.org/){:target="_blank"} on Linux; Clang support is planned)
- A **build system** (Make, Visual Studio, Ninja, etc.)
- [CMake](https://cmake.org/){:target="_blank"}
- [Python 3](https://www.python.org/){:target="_blank"}
    - You will also need to install Python modules specified in the requirements.txt file found in the root of the repo
    - Additionally, you may want to use mkdocs-material to work easier with GITS documentation, in which case you would
      also install modules specified in the requirements.txt file found in Scripts/docs directory

### Linux

On Linux you will also need:

- **wget**
- Various development headers:
    - **libx11** ([`libx11-dev`](https://packages.debian.org/sid/libx11-dev){:target="_blank"} on Ubuntu)
    - **libx11-xcb** ([`libx11-xcb-dev`](https://packages.debian.org/sid/libx11-xcb-dev){:target="_blank"} on Ubuntu)
    - **libwayland** ([`libwayland-dev`](https://packages.debian.org/sid/libwayland-dev){:target="_blank"} on Ubuntu)
    - **OpenGL** ([`libgl-dev`](https://packages.debian.org/sid/libgl-dev){:target="_blank"} on Ubuntu)

## Building

### Clone the repo

```bash
git clone https://github.com/intel/gits
cd gits
```

### Generate & build the projects

Below you can find examples how to build **GITS** for the most common cases. For other configurations (e.g. 32-bit binaries, unusual Python locations, different build systems like Ninja...) please check the relevant documentation on how you need to adjust the commands.



=== "Windows 64-bit Rel."
	```batch
	mkdir Release\x64
	cd Release\x64
	cmake -A x64 ..\..\
	cmake --build . --config Release
	```



=== "Linux 64-bit Rel."
	```bash
	mkdir -p Release/x64
	cd Release/x64
	cmake -DARCH=-m64 ../../
	# The `-j` option without an argument sets the number of threads automatically.
	# You can also set it manually, e.g., `-j8` for 8 threads.
	# Lower thread counts decrease RAM usage, which can help with OOM errors.
	cmake --build . --config Release -j
	```

> Note:  
> There may be some warnings; it's a known issue and we're working on fixing them.

### Install GITS

Before you can use **GITS** you need to perform the _install step_. Installing in this case means copying files into their destinations and setting up all paths in the configuration file. 

To install **GITS** in the folder `<gits-root-folder>\build\dist` use the following command:
```bash
cmake --install . --config Release --prefix dist
```

In the folder you'll find multiple subfolders (one per API, aptly named `FilesToCopy<API>`) containing the files needed to capture and replay applications using the API. 

> Note:
> 
> - If you move the install folder somewhere else, you will need to manually adjust paths in config files.
> - On **Windows**, it's recommended for the install path to be *relatively short* (e.g., `C:\Users\username\Code\gits\Release\x64\dist`) to avoid problems with building **CCode projects**, which are created inside the GITS install folder. As they use Boost it needs to be build and the process can produce very long paths, which may exceed the default path length limit of Windows, failing the build.
