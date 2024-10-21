# Graphics Intercept and Trace Solution (GITS)

**Graphics Intercept and Trace Solution** is a capture-replay tool for [Vulkan](https://vulkan.org/), [OpenCL](https://www.khronos.org/opencl/), [Intel oneAPI Level Zero](https://spec.oneapi.io/level-zero/latest/core/INTRO.html) and [OpenGL](https://www.khronos.org/opengl/).

**GITS** allows you to record sequences of API calls. You can either serialize them into binary traces called **streams** that can be replayed later or you can generate a _C++ project_ from them (we call it CCode). 

**GITS** has been used for years to help develop and validate Intel GPU drivers. The development is mainly focused on enabling applications, not APIs or extensions. We implement support for API calls when we encounter apps using them.

**GITS** is a collection of command line tools originally aimed at driver developers, but we think it can be useful to other users as well. If you are a game developer who wants to analyze frames using a graphical tool, [GPA](https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html) or a similar tool is what you should look at.

## Distribution

Currently we do _not_ provide prebuilt binaries. To build **GITS** you need a c++ compiler + buildsystem as well as python3. The full process is described [here](docs/building.md).

## Usage

To record an application, you will have to inject our _dynamic library_ (called the _interceptor_) into it:
- On _Windows_, this is typically done by copying a DLL into the app directory. 
- On _Linux_, by manipulating loader environment variables. 
- When recording _Vulkan_, it is also possible to use **GITS** as a _Vulkan layer_ instead.

To replay the stream, pass it as an argument to the `gitsPlayer` executable. See the player's built-in help (`gitsPlayer --help`) for usage info.

**GITS** also allows recording streams containing only a subset of API calls made by the application (e.g., only select frames). We call them substreams or subcaptures.

More information can be found in the [Usage section](docs/usage.md) of the documentation.

## Contributing

For information on how to contribute, see [Contributing](CONTRIBUTING.md). All contributions are subject to our [code of conduct](CODE_OF_CONDUCT.md).

If you want to report security issues, see [security](SECURITY.md).

## License

GITS is licensed under the terms in [license](LICENSE.md).

GITS uses third-party software which are available under their own licenses. See [GITS 3rd party programs](GITS_third-party-programs.md) for more info.
