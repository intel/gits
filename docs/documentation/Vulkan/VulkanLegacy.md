---
icon: simple/vulkan
title: Vulkan Legacy
---

# Vulkan vs. Vulkan Legacy

This page discusses various details about the two GITS Vulkan backends:

- **Vulkan** is the *new* backend which is strongly recommended and
- **Vulkan Legacy** is the *old* backend.

Both backends are build and installed by default.

!!! warning "Compatability"
    The two backends are **not compatible** and operate differently. **Streams** recorded with one backend can not be played back with the other one.

## Install Layers

To register the VulkanLegacy layer only, configure with `-DREGISTER_VULKAN_LAYER=OFF` before building, or untick the `Register the Vulkan GITS recorder layer` task in the installer. (`Scripts/Installer/install_vulkan_layer.bat <Win32|x64> <path-to>\Recorder\VulkanLayer\` registers the Vulkan layer manually.)

## Recording

!!! warning "Only select one backend at a time"
    Both backends are available by default and you should only enable *exactly one* per run - never both as that will lead to correupted streams.

### Vulkan layers selection

Both the VulkanLegacy and Vulkan GITS recorder layers are registered and available by default. They are **explicit** layers, so neither records until you enable it. The layer names are

- `VK_LAYER_INTEL_vulkan_GITS_recorder_legacy` (VulkanLegacy, in `Recorder/VulkanLayerLegacy`) and
- `VK_LAYER_INTEL_vulkan_GITS_recorder` (Vulkan, in `Recorder/VulkanLayer`).

Enable **exactly one** per application run:

- In **Vulkan Configurator**, set **one** layer to `On` and leave the other `Application-Controlled/Off` or
- set `VK_INSTANCE_LAYERS` to a single layer name.

## Playback streams

A stream is tagged in its header with the backend used during capture, e.g. `API_VULKAN_LEGACY` for VulkanLegacy, `API_VULKAN` for Vulkan. `gitsPlayer` reads this tag and automatically dispatches a stream to the respective replay backend. There is no command-line switch to choose the Vulkan backend at playback time; a stream recorded with one backend replays with the same backend.

## Sub-Capturing

The older **VulkanLegacy** backend trims a stream **during recording** instead (copy the recorder into the app directory and set a recorder mode such as `Vulkan.Recorder.Mode = Frames`). That recorder-side flow does **not** apply to the new backend, and the player-side flow described here does **not** apply to legacy streams. You never pick the backend at playback time.
