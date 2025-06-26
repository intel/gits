---
icon: octicons/video-24
title: Player
---
## General information

### Resource files handling

To optimize binary data handling, GITS uses memory mapped files to
access any GL resources (buffers, textures, etc\...). All this data is
aggregated in .dat files (`gitsTextures.dat` contains all texture data).
This data is referenced through hash value which is translated to
correct offset in right file by the use of separate index file
(`gitsDataIndex.dat`).

All data is fed to GL (if possible) through memory mapping obtained by
mapping a part of correct dat file. This has a consequence, that actual
IO is performed by OS on demand when accessing the data (which, usually,
will be done by the driver). If this is undesirable (for example, given
tool that measures execution time of GL function) then it should be
hedged against by either forcing GITS to load binary data immediately,
or by warming up data files (see performance affecting options on how to
do that).

Non-binary files (like shaders) are kept in separate files. Unlike
binary data, they can be edited for the purpose of debugging without
compromising stream integrity. All text files are loaded by GITS in load
phase, so IO impact is minimized for this type of resources.

Actual tokens replayed by GITS are stored in .gits file. This file is
loaded in separate thread and ran in main thread (assuming single
threaded playback). Up to specified amount of token bursts containing
specified amount of tokens are loaded by the loader thread before
blocking and waiting for the main thread to execute enough stream data.
This is done to minimize total playback time (loading of some of the
stream will generally overlap with stream playback) and to efficiently
support playback of long streams (such that won't fit entirely in memory
during playback).

## Command line options {#sec:PlayerOpts}

Long options can be specified with either single or double dash.

All of the options need to be specified in the command line. If no
options are specified in the command line, and GITS finds a file
`gitsPlayer.rsp` in current working directory, this file will be used as
a response file. Response file will be read and behavior of the GITS
player will be as if the contents of the response file were passed
through the command line. Each line of the response file will be treated
as separate parameter passed to player binary. Be wary of any trailing
whitespace in the response file as it will be passed to GITS player
verbatim.

For sample command line player invocation:

```bash
$ gitsPlayer "/mnt/repo/my stream/stream.gits" -exitFrame 10
```

Corresponding response file will have following content:

```bash
/mnt/repo/my stream/stream.gits
-exitFrame
10
```

Options in GITS player are divided into a number of groups. To list the
help on a specific option group use `-hh <group_name>` option.
Currently the following API-based groups are available:

| group       | description                                          |
| ----------- | ---------------------------------------------------- |
| Common      | Options common to all APIs                           |
| Vulkan      | Vulkan specific options                              |
| OpenGL      | OpenGL specific options                              |
| OpenCL      | OpenCL specific options                              |
| LevelZero   | LevelZero specific options                           |
| all         | all the options (union of prev. mentioned groups)    |

More groups will be coming in the future, and will be based on their intended
target audience and usage.

