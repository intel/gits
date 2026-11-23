---
icon: octicons/video-24
title: Player
---
# General information

The GITS player binary allows to playback a GITS stream passed via command line. It contains the default configuration values and only needs to be passed the stream as a commandline argument. 

Streams recorded with GITS 3 and later are named `stream.gits`. The player also plays `.gits2` streams recorded with GITS 2.x. You can pass either the stream file or the folder that contains it; a folder must contain exactly one stream file.

There are multiple ways how the GITS player can be configured:

- via command line argument,
- via environment argument and
- via config file `gits_config.yml`.

The details of the options as well as how to use each method is described in the documentation's [configuration section](../configuration/how_to.md).

For replay requirements of streams recorded with OpenGL and Vulkan interop, see [OpenGL and Vulkan interop](interop.md).

# Example usage

--8<-- "player_steps.md"
