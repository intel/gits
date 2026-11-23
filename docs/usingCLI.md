---
icon: octicons/command-palette-24
title: GITS CLI
---

This page aims to provide a general overview of how to use GITS via command-line. For an in-depth look please see the comprehensive [documentation section](documentation/terminology.md).

# GITS Binaries

**GITS** consists of three parts:

- **Recorder**: Libraries used to intercept (and record) API calls into a stream
- **Player**: Executable to playback a stream
- **Plugins, Utilities and Tools**: Miscellaneous binaries for specialized use cases

You can find all the binaries in the output folders of GITS. After completing the [build & install process](building.md) you will have multiple folders in the installation directory (default location: `<gits-root-folder>\<build-folder>\<install-folder>`).

# Record

The **Recorder** is split into multiple folders one per supported API:

| API                                | Windows                   | Unix                |
| ---------------------------------- | ------------------------- | ------------------- |
| :material-microsoft: DirectX12     | `FilesToCopyDirectX`      | ---                 |
| :simple-opengl: OpenGL             | `FilesToCopyOGL`          | `OpenGL`            |
| :simple-vulkan: Vulkan             | `FilesToCopyVulkan`       | `Vulkan`            |
| :simple-vulkan: Vulkan Layer       | `VulkanLayer`             | `VulkanLayer`       |

The **OpenGL** backend is new and experimental. Streams recorded with the removed OpenGLLegacy, VulkanLegacy, OpenCL and Level Zero backends can only be replayed with an older GITS release. OpenGL ES applications were recorded with OpenGLLegacy and can no longer be recorded.

--8<-- "recorder_steps.md"

For information on how to adjust parameters please see the [configuration section](#configuration).

## Per-OS notes

- **Windows**: the Vulkan layer is registered automatically at install time (see the `REGISTER_VULKAN_LAYER` [CMake option](building.md#optional-cmake-options)).
- **Linux**: point the loader at the layer:

  ``` bash
  export VK_LAYER_PATH=<install>/Recorder/VulkanLayer
  export VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder
  ```

# Playback

--8<-- "player_steps.md"

For information on how to adjust parameters please see the [configuration section](#configuration).

# Configuration

Aside from various binaries there's also the pre-configured `gits_config.yml` inside the folder containing all default values. This file is used by both `gitsRecorder` as well as `gitsPlayer` and exposes all the dials and knobs the respective application offers. As YAML files are hierarchically structured finding the values/settings you need to customize should not be too dificult.

## Further Information

You can find details of the configuration file options in the [Configuration](configuration/how_to.md) section of the documentation:

- General/shared options can be found in [Common](configuration/CommonAuto.md) as well as
- API specific subsections such as [DirectX 12](configuration/DirectXAuto.md), [Vulkan](configuration/VulkanAuto.md) or others.
