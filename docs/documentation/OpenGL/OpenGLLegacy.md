---
icon: simple/opengl
title: OpenGL Legacy
---

# OpenGL vs. OpenGL Legacy

This page discusses various details about the two GITS OpenGL backends:

- **OpenGL** (`OpenGL/` module) is the *new*, experimental backend. It is still a work in progress: calls it does not support yet are forwarded to the driver but not recorded (GITS logs a warning once per command), so a stream of an application using them will not replay faithfully. Its stream format may still change.
- **OpenGL Legacy** (`OpenGLLegacy/` module) is the *original* backend. It records OpenGL and OpenGL ES applications using WGL, GLX or EGL.

!!! warning "Compatibility"
    The two backends are **not compatible** and operate differently. **Streams** recorded with one backend can not be played back with the other one.

## Build

OpenGL Legacy is always built. The new OpenGL backend is controlled by the `WITH_OPENGL` CMake option (see [Build GITS](../../building.md#optional-cmake-options)). It is 64-bit only and built by default on Windows x64 and x86-64 Linux.

## Recording

!!! warning "Only select one backend at a time"
    Both backends intercept the same libraries (e.g. `OpenGL32.dll` on Windows, `libGL.so.1` on Linux). Use the recorder files of *exactly one* backend per run.

| Backend                     | Windows                         | Linux                   |
| --------------------------- | ------------------------------- | ----------------------- |
| OpenGL                      | `Recorder/FilesToCopyOGL`       | `Recorder/OpenGL`       |
| OpenGL Legacy               | `Recorder/FilesToCopyOGLLegacy` | `Recorder/OpenGLLegacy` |
| OpenGL ES (OpenGL Legacy)   | `Recorder/FilesToCopyES`        | `Recorder/OpenGLLegacy` |

- **Windows**: copy the contents of the folder into the directory of the application executable.
- **Linux**: point the loader at the folder, for example for OpenGL Legacy:

  ``` bash
  export LD_LIBRARY_PATH=<install>/Recorder/OpenGLLegacy:$LD_LIBRARY_PATH
  ```

!!! note "Install folders of older GITS versions"
    Older GITS versions installed OpenGL Legacy into `FilesToCopyOGL` (Windows) and `Recorder/OpenGL` (Linux), and the new backend into `FilesToCopyOGL2` and `Recorder/OpenGL2`. Update scripts that copy recorder files from these folders.

## Configuration

Both backends use `gits_config.yml`. The [OpenGL configuration](../../configuration/OpenGLAuto.md) section applies to OpenGL Legacy; of its options, the new backend only reads `OpenGL.Recorder.SuppressExtensions` and `OpenGL.Recorder.SuppressProgramBinary`. The new backend supports [plugins](../Plugins.md), which install under `Plugins/OpenGL/` and are enabled with `Common.Recorder.Plugins` and `Common.Player.Plugins`.

## Playback streams

A stream is tagged in its header with the backend used during capture, e.g. `API_OPENGL_LEGACY` for OpenGL Legacy, `API_OPENGL` for OpenGL. `gitsPlayer` reads this tag and automatically dispatches a stream to the respective replay backend. There is no command-line switch to choose the OpenGL backend at playback time; a stream recorded with one backend replays with the same backend. Replaying an OpenGL stream requires a `gitsPlayer` built with `WITH_OPENGL`.

Streams recorded with OpenGL Legacy by older GITS versions keep their tag and still replay with OpenGL Legacy. Streams recorded with the new backend are not guaranteed to replay with other GITS versions while its stream format is still changing.
