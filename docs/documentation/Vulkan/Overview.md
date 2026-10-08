---
icon: simple/vulkan
title: Overview
---

# Overview

This page covers the **new Vulkan backend**. GITS records Vulkan API calls made
by an application into a stream and replays them with `gitsPlayer`. Common
features such as [frame sub-capture](Subcapture.md), resource dumping, and
screenshots are supported.

# Recording a stream

## Method 1: Vulkan layer (recommended, Windows and Linux)

GITS registers as a Khronos **explicit layer** named
`VK_LAYER_INTEL_vulkan_GITS_recorder` (installed under `Recorder/VulkanLayer`).
Because it is explicit, it records nothing until you enable it:

- **Windows:** the installer registers the layer, so enabling it in **Vulkan
  Configurator** (set this layer's control to *Enable*) or setting
  `VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder` is enough. If you use a
  portable build that was not installed (so the layer isn't registered), also set
  `VK_LAYER_PATH=<install>/Recorder/VulkanLayer`, as on Linux.
- **Linux:** point the loader at the layer directory and enable it:

  ``` bash
  export VK_LAYER_PATH=<install>/Recorder/VulkanLayer
  export VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder
  ```

!!! note
    The **Vulkan Configurator** GUI (`vkconfig`) is part of the [LunarG Vulkan SDK](https://vulkan.lunarg.com/), so enabling the layer that way requires the SDK to be installed. The `VK_INSTANCE_LAYERS` (and, on Linux, `VK_LAYER_PATH`) environment-variable method needs only the Vulkan loader/runtime that ships with the GPU driver and does **not** require the SDK.

The layer works even when replacing the loader DLL is not feasible, and it can
coexist with other layers (for example validation).

!!! note "Instance-level suppression caveat"
    `Vulkan.Shared.SuppressExtensions` and `Vulkan.Recorder.SuppressLayers` are **not** applied to *instance*-level extensions and layers in layer mode. The Vulkan loader answers the pre-instance queries (`vkEnumerateInstanceExtensionProperties` / `vkEnumerateInstanceLayerProperties`) itself and does not route them through an explicit layer, so GITS cannot hide them during capture this way. Instance-level entries are instead filtered at **replay** time via `Vulkan.Player.SuppressLayers` (independent from the recorder-side list, so a layer that must stay loaded while capturing to avoid crashing the app can still be dropped on replay, or vice versa); capturing through the [interceptor](#method-2-copy-the-interceptor-dll-windows) additionally suppresses `Vulkan.Recorder.SuppressLayers` entries during capture. Device-level extension suppression and `SuppressPhysicalDeviceFeatures` work in both layer and interceptor modes.

## Method 2: Copy the interceptor DLL (Windows)

The interceptor is a drop-in replacement for the system Vulkan loader
(`vulkan-1.dll`). Copy the contents of `FilesToCopyVulkan` into the directory of
the executable you want to capture; the bundled `vulkan-1.dll` is then loaded in
place of the real loader and records the application's calls.

## Steps

1. Enable interception with **one** of the methods above.
2. Update `gits_config.yml` to point to the right directories for
   `Common.Recorder.InstallationPath` and `Common.Recorder.DumpDirectoryPath`.
3. Run the application.
4. Exit the application normally.
5. A stream should be present under `Common.Recorder.DumpDirectoryPath`.
6. Rename `gitsPlayer.exe` to the desired executable name. This enables
   driver-side optimizations based on the executable name.
7. Play it back with `gitsPlayer.exe PATH/TO/STREAM`.

Learn more about the configuration file options
[here](../../configuration/VulkanAuto.md).

## Plugins

Plugins can extend the **Vulkan** backend during capture and replay (Windows today). See [Plugins](../Plugins.md) for shared configuration and the [Vulkan plugin catalog](Plugins.md).

# Sub-capture

Sub-capture trims a stream down to a frame range, producing a smaller,
self-contained stream. See the [sub-capture section](Subcapture.md) for details.

# Host-side commands using `VkDeviceOrHostAddress`

A handful of entry points address their payload through `VkDeviceOrHostAddress[Const]KHR`. On the
`vkCmd*` forms the live union member is `deviceAddress`, which GITS tracks and relocates. On the
host-side forms it is `hostAddress` - a raw capture-process pointer. `vk.xml` attaches no
`selector` to the union, so the generated coder cannot tell which member is live and copies the
union verbatim; the capture-process pointer reaches the player unchanged and dereferencing it
faults inside the driver.

The player therefore skips these commands rather than crash, logging the reason once per command:

| Command | Replay behaviour |
|---------|------------------|
| `vkConvertCooperativeVectorMatrixNV` | A size query (`dstData` null) executes when `srcData` is null too, since then no host pointer can be dereferenced; one carrying a `srcData` is skipped instead, as nothing stops the driver reading it. The conversion form is skipped - its only output is `dstData`, and when that is mapped memory the driver's write is already captured as a mapped-memory update that replays right after, so the converted data still arrives. |
| `vkCopyAccelerationStructureToMemoryKHR`, `vkCopyMicromapToMemoryEXT` | Skipped when `dst` is a host address. Same reasoning: the destination contents, if they matter, come from the recorded memory update. |
| `vkBuildAccelerationStructuresKHR`, `vkBuildMicromapsEXT`, `vkCopyMemoryToAccelerationStructureKHR`, `vkCopyMemoryToMicromapEXT` | Skipped, but **not** repaired - these *read* through the host pointer, and a mapped-memory update cannot supply one, so the target keeps whatever content it had. The recorder reports this at capture time so the problem surfaces before a long capture is finished. |

The asymmetry between the two groups is the direction of the transfer, not whether the data was
captured. `MappedDataMeta` is addressed by memory key and offset, so it can reproduce *contents* at
whatever host address the replay process happens to use. It cannot reproduce a *pointer*. A command
whose host address is a destination is therefore recoverable - the driver's write was captured and
replays into the equivalent memory - while a command whose host address is a source needs that
pointer to be valid, and nothing relocates it even when the bytes themselves are in the stream.

Prefer the device-side `vkCmd*` variants when the application can be configured to use them.


