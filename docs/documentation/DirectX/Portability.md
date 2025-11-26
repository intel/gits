---
icon: material/microsoft
title: Portability
---
GITS streams may not be portable accross GPU architectures due to different feature sets or allocation sizes / offsets.

# Placed Resources

[Placed resources](https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources#placed-resources) are the main source of incompatibility due to platform specific GPU sizes and alignments.

The Portability Layer can be used to help with resource placement compatibility. It can be used to (1) generate a `resourcePlacementData.dat` file containing resource sizes and offsets and to (2) pre-load a `resourcePlacementData.dat` to resize all the placement heaps set the correct offsets for placed resources.

## Usage

### To playback a stream from **Platform A** (🖥️) on **Platform B** (💻):

1. Generate `resourcePlacementData.dat` on **Platform A** (🖥️)

   - `resourcePlacementData.dat` can be generated on capture or playback:
     - Capture: Set `DirectX.Capture.Portability.ResourcePlacementStorage` to `true`
     - Playback: Set `DirectX.Playback.Portability.ResourcePlacement` to `'store'`
   - `resourcePlacementData.dat` will be written next to `stream.gits2`

2. Playback stream on **Platform B** (💻)

   - Copy `resourcePlacementData.dat` next to `stream.gits2`
   - Enable Portability Layer in `gits_config.yml` by setting `DirectX.Playback.Portability.ResourcePlacement` to `'use'`

### **(Experimental)** To playback a stream on **Platform B** (💻) without accessing **Platform A** (🖥️):

1. Generate `resourcePlacementData.dat` on **Platform B** (🖥️)

   - Playback: Set `DirectX.Playback.Portability.ResourcePlacement` to `'store'` and `DirectX.Playback.Execute` to `false`
   - `resourcePlacementData.dat` will be written next to `stream.gits2`

2. Playback stream on **Platform B** (💻)

   - Same as normal usage

# Ray Tracing (DXR)

Ray Tracing uses acceleration structures which are platform (and driver!) dependendent. The application will query the runtime for the correct sizes to use to store and build such structures (see [GetRayTracingAccelerationStructurePrebuildInfo](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-getraytracingaccelerationstructureprebuildinfo)).

GITS will ask the application to allocate extra memory (2x by default) to ensure that all the D3D12 resources have extra padding to accommodate driver updates.

Use the `DirectX.Capture.Raytracing` options to change the padding multipliers used.

# Notes

The Portability Layer will not be able to solve all the issues that may occur by playing back streams from different GPU vendors. It is recommended that you capture a separate stream on each platform.
