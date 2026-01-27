---
icon: material/microsoft
title: CCode (Export to C++)
---

# Overview

GITS can export a stream to C++. The output will be a standalone CMake project containing all the commands serialized to C++ (and all the resources serialized to a `data.bin` file).

> Note that the implementation of this feature is still work-in-progress. See [Limitations](#limitations) below.

## Usage

### Generate the CCode project

Enable CCode in the configuration by setting `DirectX.Player.CCode.Enabled` to `true` and run `gitsPlayer.exe` to playback the desired stream.

A directory named `CCode/` will be created in the stream directory (or GITS output path).

> Note that CCode is only recommended with Frame or CommandList sub-captures. Otherwise the amount of code and data generated may be too large to be practical to build and use.

## Build the CCode project

Use CMake to build the project.

```batch
mkdir Build
cd Build
cmake -A x64 ..\
cmake --build . --config Release
```

An executable (with the same name as the application the stream was captured from) will be created under `CCode/Build/Release/`.

## Limitations

The initial implementation is limited to `DirectX12` and `DXGI`.

The following APIs are not yet supported:

- DirectX12 Ray Tracing (DXR)
- DirectML
- DirectStorage
- Intel Extensions
- Intel XeSS
- Nvidia NVAPI

Support will be added incrementally as needed.
