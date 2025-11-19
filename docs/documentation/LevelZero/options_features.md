---
title: Options
icon: simple/intel
---
# Notice {#_notice}

Copyright (c) 2023 Intel Corporation.

# Overview {#_overview}

This section documents features that might be enabled during recording
in GITS configuration file or during playback appending options to the
GITS Player binary.

# Recording Features {#_recording_features}

- `LevelZero.Capture.Mode`  
  Mode of operation of LevelZero capture, allowed values are `All`, `Kernel`

- `LevelZero.Capture.All`  
  Mode of operation of LevelZero capture. Captures whole stream. It is the default behavior.

- `LevelZero.Capture.Kernel`  
  Mode of operation of LevelZero capture. Captures subcapture. See [subcaptures section](subcaptures.md) for more information.

- `LevelZero.Utilities.DumpKernels`  
  Generates kernel buffers and memory layout. May inject synchronization and buffer/image reads API
 calls. The [nomenclature](subcaptures.md#nomenclature) is passed as second argument to the option. Each read is to allocated memory by GITS, which is freed after synchronization.

- `LevelZero.Utilities.DumpAfterSubmit`  
  Used with
  `LevelZero.Utilities.DumpKernels`, modifies the way kernel arguments
  are dumped. Injects immediate command lists, reads and
  synchronization points after command list submission instead of each
  kernel submission. Reduces overhead cost of allocated memory by
  GITS. Must be used with caution. If kernel argument was overwritten
  by many kernels, the output buffer would have the value after the
  last executed kernel that used it. Gathering comparison data during
  playback must be used with equivalent option
  `--l0CaptureAfterSubmit` to match the behavior.

- `LevelZero.Utilities.DumpImages`  
  Used with `LevelZero.Utilities.DumpKernels`. Capture images in addition to buffers. Assumption is that they all are 2D RGBA8 images.

- `LevelZero.Utilities.BufferResetAfterCreate`  
  Nullifies allocations immediately after their creation to produce deterministic results during comparison. Injects writes, moreover it saves

- `LevelZero.Utilities.NullIndirectPointersInBuffer`  
  Nullifies output buffer's indirection pointers in order to produce deterministic results during verification step. It requires `l0_gits_indirect_access_pointers_locations` extension in order for this option to be effective.

- `LevelZero.Utilities.BruteForceScanForIndirectPointers.MemoryType`  
  Setting this to non zero value will cause GITS to scan each specified allocation region for indirect pointers. The algorithm will use multiple threads to scan memory regions byte by byte comparing 8-byte data to any region of any USM allocations before any kernel submission. Once found GITS extension for indirect pointers will be injected and scheduled into new stream. The value of this option is a bitfield that specifies the type of memory to be scanned might be a combination. Supported values are:

  |      |                  |
  | ---- | ---------------- |
  | `-1` | All memory types |
  | `1`  | Host memory      |
  | `2`  | Device memory    |
  | `4`  | Shared memory    |

- `LevelZero.Utilities.BruteForceScanForIndirectPointers.Iterations`  
  Reduces iterations for brute force algorithm to run the scan only `N` times for any allocation.

- `LevelZero.Utilities.DisableAddressTranslation.MemoryType`  
  Setting this to non zero value will cause GITS modify allocations with an intention to work on the same address space during playback. The value of this option is a bitfield that specifies the type of memory to be affected. Valid values are:

  |      |                  |
  | ---- | ---------------- |
  | `-1` | All memory types |
  | `1`  | Host memory      |
  | `2`  | Device memory    |
  | `4`  | Shared memory    |

- `LevelZero.Utilities.DisableAddressTranslation.VirtualDeviceMemorySize`  
  Injects virtual memory reservation of specified size per created context.

- `LevelZero.Utilities.DisableAddressTranslation.VirtualHostMemorySize`  
  Injects virtual memory reservation of specified size on recorder initialization.

# Playback Features {#_playback_features}

- `--l0CaptureKernels <range>`  
  Generates kernel buffers and memory layout. May inject synchronization and buffer/image reads API calls. The [nomenclature](subcaptures.md#nomenclature) is passed as second argument to the option. Each read is to allocated memory by GITS, which is freed after synchronization.

- `--l0CaptureInputKernels`  
  Similarly to `--l0CaptureKernels`, however, injects are before kernel submission to dump input buffers. Works only with `l0CaptureKernels`. Option exists only for debug purposes.

- `--l0DisableAddressTranslation`  
  Tries to use addresses originally used during recording. Supported values are:

  |      |                  |
  | ---- | ---------------- |
  | `-1` | All memory types |
  | `1`  | Host memory      |
  | `2`  | Device memory    |
  | `4`  | Shared memory    |

- `--l0OmitOriginalAddressCheck`  
  Do not throw when expected address is not equal to originally requested. Example `pStart` of `zeVirtualMemReserve`.

- `--l0DumpSpv`  
  (DEPRECATED) Dumps SPIRV files on zeModuleCreate API call directly from the `pInputModule` of size `inputSize` fields in descriptor to the `l0Programs` folder. Requires module format: `ZE_MODULE_FORMAT_IL_SPIRV`.

- `--l0CaptureAfterSubmit`  
  used with `--l0CaptureKernels`, modifies the way kernel arguments are dumped. Injects immediate command lists, reads and synchronization points after command list submission instead of each kernel submission. Reduces overhead cost of allocated memory by GITS. Must be used with caution. If workload has been recorded without this option, created reference buffers could be different than with this option.

- `--l0InjectBufferResetAfterCreate`  
  Nullifies allocations immediately after their creation to produce deterministic results during comparison.

- `--l0DisableNullIndirectPointersInBuffer`  
  used with `--l0CaptureKernels`. Disable nullification of indirect pointers inside dumped buffers. Stream requires `l0_gits_indirect_access_pointers_locations` extension in order for this option to be effective.

- `--l0CaptureImages`  
  Capture images in addition to buffers. Assumption is that they all are 2D RGBA8 images. Has to be used with `--l0CaptureKernels`.
