---
icon: simple/vulkan
title: Overview
---

# Overview

This page covers the **new Vulkan backend**. GITS records Vulkan API calls made
by an application into a stream and replays them with `gitsPlayer`. Common
features such as [frame sub-capture](Subcapture.md), resource dumping, and
screenshots are supported.

> GITS also ships an older **VulkanLegacy** backend, installed side by side.
> Both are available by default and you enable exactly one per run. See
> [VulkanLegacy vs Vulkan backends](../../usage.md#vulkan-vulkanlegacy-vs-vulkan-backends)
> for the differences and how streams are routed at playback time.

# Recording a stream

There are **two interchangeable ways** to intercept an application's Vulkan
calls with the new backend. Use **one** of them per run — never both at once,
and never together with a legacy layer, or the recorders chain and produce a
corrupt stream.

## Method 1: Vulkan layer (recommended, Windows and Linux)

GITS registers as a Khronos **explicit layer** named
`VK_LAYER_INTEL_vulkan_GITS_recorder` (installed under `Recorder/VulkanLayer`).
Because it is explicit, it records nothing until you enable it:

- **Windows:** the layer is registered automatically at install time. Enable it
  in **Vulkan Configurator** (set only this layer to *On*), or set
  `VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder`.
- **Linux:** point the loader at the layer directory and enable it:

```bash
export VK_LAYER_PATH=<install>/Recorder/VulkanLayer
export VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder
```

The layer works even when replacing the loader DLL is not feasible, and it can
coexist with other layers (for example validation).

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

# Sub-capture

Sub-capture trims a stream down to a frame range, producing a smaller,
self-contained stream. See the [sub-capture section](Subcapture.md) for details.


