# Graphics Intercept and Trace Solution (GITS)

**Graphics Intercept and Trace Solution** is a capture-replay tool for [Vulkan](https://vulkan.org/), [OpenCL](https://www.khronos.org/opencl/), [Intel oneAPI Level Zero](https://spec.oneapi.io/level-zero/latest/core/INTRO.html) and [OpenGL](https://www.khronos.org/opengl/).

GITS allows you to record sequences of API calls. You can either serialize them into binary traces that can be replayed later (we call them streams), or you can generate a C++ project from them (we call it CCode). See the [Usage section](#usage) for more info.

GITS has been used for years to help develop and validate Intel GPU drivers. It is a collection of command line tools aimed at driver developers, but we think it can be useful to other users as well. If you are a game developer who wants to analyze frames using a graphical tool, [GPA](https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html) or a similar tool is what you should look at.

## Installation

Currently we do not provide prebuilt binaries. You will have to build it yourself.

## Building

GITS is written in C++. You will need a compiler and a build system.

We use [CMake](https://cmake.org/) to generate the build files. You will need to specify a generator, architecture, and install prefix.

We use [Python 3](https://www.python.org/) and [Mako Templates](https://www.makotemplates.org/) for code generation, make sure you have them installed.

For more info, see [BUILDING.md](BUILDING.md)

## Usage

To record an application, you will have to inject our dynamic library (called the interceptor) into it. On Windows, this is typically done by copying a DLL into the app directory. On Linux, by manipulating loader environment variables. When recording Vulkan, it is also possible to use GITS as a Vulkan layer instead.

To play back the stream, pass it as an argument to the gitsPlayer executable. See the player's built-in help (`gitsPlayer --help`) for usage info.

GITS allows recording streams containing only a subset of API calls made by the application (e.g., only select frames). We call them substreams or subcaptures.

GITS development is focused on enabling applications, not APIs or extensions. We implement support for API calls when we encounter apps using them.

For more info, see [USAGE.md](USAGE.md)

## Contributing

For information on how to contribute, see [CONTRIBUTING.md](CONTRIBUTING.md). Contributions are subject to our [code of conduct](CODE_OF_CONDUCT.md).

If you want to report security issues, see [SECURITY.md](SECURITY.md).

## License

GITS is licensed under the terms in [LICENSE.md](LICENSE.md).

GITS uses third-party software which are available under their own licenses. See [doc/GITS_third-party-programs.txt](doc/GITS_third-party-programs.txt) for more info.
