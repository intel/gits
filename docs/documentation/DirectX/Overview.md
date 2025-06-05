# DirectX - Overview

## Features

**GITS** supports capture and replay of full DirectX12 workloads, including **D3D12** and supporting APIs such as **DXGI**, **DirectML**, and **DirectStorage**. Common features such as frame sub-capture, resource dumping, and screenshots are also supported.

### API Support

- Direct3D12
- DXGI
- DirectML
- [DirectStorage](DirectStorage.md)
- Intel XeSS
- Intel GPU Driver Extensions

### Plugins

Plugins can be used to extend the feature set of the **DirectX** backend during both capture and replay. Learn more about plugins [here](Plugins.md).

## Basic Usage

Using **GITS** is straightforward. Copy the API shims into the application directory, tweak the config file, and you are all set.

1. Copy the `FilesToCopyDirectX` into the directory with the executable you want to capture.
2. Update `gits_config.yml` to point to the right directories for `Common.Recorder.InstallationPath` and `DumpDirectoryPath`.
3. Run the application.
4. Exit the game normally.
5. A stream should be present under `Common.Recorder.DumpDirectoryPath`.
6. Rename `gitsPlayer.exe` to the desired executable name. This enables driver-side optimizations based on the executable name.
7. Run `gitsPlayer.exe PATH/TO/STREAM`.

Learn more about the configuration file options [here](../configuration/DirectXAuto.md).

## Sub-capture

Frame sub-capture is supported through `gitsPlayer.exe`. The current implementation relies on playing back the trace multiple times:

1. Analyze the workload (this will generate an analysis file in the directory you are calling `gitsPlayer` from).
2. Record the sub-capture.

For both steps, the `Player/gits_config.yml` file needs to be edited:

1. Set `DirectX12.Subcapture.Enabled` to `true`.
2. Set `DirectX12.Subcapture.Frames` to the desired range (e.g., `'1800-1803'`).

Run `gitsPlayer.exe` twice (you can use `--exitFrame` to exit right after the subcapture range is completed).

> **Notes**:
> - To find the correct range to subcapture, you can either look at the screenshots or use `--showWindowBorder` on playback (which will show the frame number on the window title).
> - The subcapture stream will be stored in the path specified by `Common.Player.SubcapturePath`.

## Application Specific Options

All application-specific configuration overrides can be found under `Overrides` in the configuration file.

The only expected compatibility option to toggle is `DirectX.Capture.ShadowMemory`, which affects how mapped/unmapped resources are tracked and may impact the stability of certain applications. If there is a problem recording (or playing back) a stream for a given application, set `DirectX.Capture.ShadowMemory` to `true` and try again.

## Issues, Comments, and Feedback

Please let us know if you encounter any issues with D3D12 GITS. Send an email to GITS Developers (<gits.developers@intel.com>) with any details you can provide.