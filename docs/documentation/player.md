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

		$ gitsPlayer "/mnt/repo/my stream/stream.gits" -exitFrame 10

Corresponding response file will have following content:

		/mnt/repo/my stream/stream.gits
		-exitFrame
		10

Options in GITS player are divided into a number of groups. To list the
help on specific option group use `-hh <groupname>` option. Options are
generally assigned to a group based on the functionality of the options,
however some options fit many groups - they will still be assigned to
only one of the following groups:

| group       | description                                          |
| ----------- | ---------------------------------------------------- |
| general     | general GITS behavior options                        |
| playback    | playback control options (like early stream exit)    |
| image       | options for images (png files) gathering             |
| metrics     | options for doing various measurements/statistics    |
| performance | options that can affect GITS playback performance    |
| mutators    | options that modify GITS api stream in some way      |
| workaround  | workarounds needed for stream playback in some cases |
| internal    | options of interest mostly for GITS dev team         |
| all         | all the options (union of prev. mentioned groups)    |


Also note, that not all options are available on all platforms. Consult
'-hh all' option to verify option presence on particular platform (or
just attempt to use one).

