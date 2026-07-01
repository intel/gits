---
icon: octicons/command-palette-24
title: Using GITS
---

This page aims to provide a general overview of how to use gits. For an in-depth look please see the comprehensive [documentation section](documentation/terminology.md).

# GITS Binaries

**GITS** consists of three parts:

- **Recorder**: Libraries used to intercept (and record) API calls into a stream
- **Player**: Executable to playback a stream
- **Plugins, Utilities and Tools**: Miscalaneus binaries for specialized use cases

You can find all the binaries in the output folders of GITS. After completing the [build & install process](building.md) you will have multiple folders in the installation directory (default location: `<gits-root-folder>\<build-folder>\<install-folder>`).

The **Recorder** is split into multiple folders depending on the API you wish to record:

| API                            | Windows              | Unix          |
| ------------------------------ | -------------------- | ------------- |
| :material-microsoft: DirectX12 | `FilesToCopyDirectX` | ---           |
| :simple-intel: LevelZero       | `FilesToCopyL0`      | `LevelZero`   |
| :simple-opengl: OpenGL         | `FilesToCopyOGL`     | `OpenGL`      |
| :simple-opengl: OpenGL ES      | `FilesToCopyES`      | `OpenGL`      |
| :simple-opengl: OpenCL         | `FilesToCopyOCL`     | `OpenCL`      |
| :simple-vulkan: VulkanLegacy       | `FilesToCopyVulkanLegacy` | `VulkanLegacy` |
| :simple-vulkan: VulkanLegacy Layer | `VulkanLayerLegacy`  | `VulkanLayerLegacy` |
| :simple-vulkan: Vulkan             | `FilesToCopyVulkan`  | `Vulkan`      |
| :simple-vulkan: Vulkan Layer       | `VulkanLayer`        | `VulkanLayer` |

# Record

To record the API-calls from an application you first need to identify the *correct folder* for your endeavor by choosen the right API and OS and use it in the recording process.

- On **Windows**, this is typically done by copying a DLL into the app directory.
- On **Linux**, by manipulating loader environment variables.
- When recording **Vulkan**, it is also possible to use **GITS** as a **Vulkan layer** instead. This method works on both OSs.

Once the files are in place you simply start the application normally while GITS initiates the recording. For information on how to adjust parameters of the recording please see the [configuration section](#configuration).

## Vulkan: VulkanLegacy vs Vulkan backends

GITS ships two Vulkan capture/replay backends that are installed side by side:

- **VulkanLegacy** (`VulkanLegacy/` module): the original backend.
- **Vulkan** (`Vulkan/` module): the newer backend.

Both the VulkanLegacy and Vulkan GITS recorder layers are registered and available by default. They are **explicit** layers, so neither records until you enable it. Enable **exactly one** per application run:

- In **Vulkan Configurator**, set one layer to On and leave the other Application-Controlled/Off.
- Or set `VK_INSTANCE_LAYERS` to a single layer name.

Never force-enable both layers, and do not combine a GITS layer with the GITS `vulkan-1.dll` interceptor - either combination chains two recorders and produces a corrupt stream.

The layer names are `VK_LAYER_INTEL_vulkan_GITS_recorder_legacy` (VulkanLegacy, in `Recorder/VulkanLayerLegacy`) and `VK_LAYER_INTEL_vulkan_GITS_recorder` (Vulkan, in `Recorder/VulkanLayer`).

Per-OS notes:

- **Windows**: both layers are registered automatically at install time. To register the VulkanLegacy layer only, configure with `-DREGISTER_VULKAN_LAYER=OFF` before building, or untick the *"Register the Vulkan GITS recorder layer"* task in the installer. (`Scripts/Installer/install_vulkan_layer.bat <Win32|x64> <path-to>\Recorder\VulkanLayer\` registers the Vulkan layer manually.)
- **Linux**: point the loader at the layer you want, for example Vulkan:

```bash
export VK_LAYER_PATH=<install>/Recorder/VulkanLayer
export VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder
```

> **Replay must match the recording backend.** A stream is tagged in its header with the producing API (`API_VULKAN_LEGACY` for VulkanLegacy, `API_VULKAN` for Vulkan). `gitsPlayer` reads this tag and automatically dispatches a VulkanLegacy stream to the VulkanLegacy replay path and a Vulkan stream to the Vulkan replay path, so you do not select the backend at playback time.

# Playback

To playback a binary stream you can use the commandline to run `gitsPlayer` with the arguments as needed.

```batch
gitsPlayer BasicSample_7016_24_06_13_16_57_00.297
```

Use player's --help option to see basic usage info, or --hh all to list all available options.

Here are some of the most commonly used arguments:

| Argument                        | Windows                                                    |
| ------------------------------- | ---------------------------------------------------------- |
| `--help`                        | Basic usage info                                           |
| `--stats`                       | Show stream statistics                                     |
| `--captureFrames`               | Dump rendered frames to disk                               |
| `--captureFrames 23,420-450:10` | Dump rendered frames `#23, #420, #430, #440, #450` to disk |
| `--captureDraws`                | Dump rendered draw calls to disk                           |

For information on how to adjust parameters of the playback please see the [configuration section](#configuration).

# Configuration

Aside from various binaries there's also the pre-configured `gits_config.yml` inside the folder containing all default values. This file is used by both `gitsRecorder` as well as `gitsPlayer` and exposes all the dials and knobs the respective application offers. As YAML files are hierarchically structured finding the values/settings you need to customize should not be too dificult.

> <span style="font-size:30px;">:writing_hand: TODO</span>  
> Talk about the most important settings

## Further Information

You can find details of the configuration file options in the **Configuration** section of the documentation: e.g [Common](configuration/CommonAuto.md), [DirectX](configuration/DirectXAuto.md)
