---
icon: simple/vulkan
title: Sub-capture
---

# Sub-capture (Vulkan)

Sub-capture is used to trim a stream down to a small frame range, producing a
new, self-contained `*.gits2` stream that reproduces the same rendering as the
original over that range.

This page covers the **new Vulkan backend**. It uses the same player-side
workflow as [DirectX sub-capture](../DirectX/Subcapture.md): the trimming
happens while `gitsPlayer.exe` replays the full stream, driven by the shared
`Common.Player.Subcapture` options.

## Frame sub-capture

Frame sub-capture is done with `gitsPlayer.exe` replaying a full Vulkan stream.
With optimization enabled (the default) the player is run **twice** for a given
range: once to **analyze** which objects the range depends on, once to **record**
the trimmed stream.

Configure sub-capture in `gits_config.yml` (next to the player) or override on
the command line:

1. Set `Common.Player.Subcapture.Enabled` to `true`.
2. Set `Common.Player.Subcapture.Frames` to the desired range (for example `5`
   or `3-6`).

The output stream is written under `Common.Player.SubcapturePath` (see
`gits_config.yml`).

!!! notes
    - `Frames` accepts a single frame (`5`) or an inclusive range (`3-6`). A frame boundary is a `vkQueuePresentKHR`; frame *N* is the *N*-th presented frame (1-based).
    - The player restores the full Vulkan state (memory contents, image layouts, descriptor sets, pipelines, synchronization state, ...) at the start of the range, then records the in-range commands, so the trimmed stream stands on its own.
    - Output-stream compression is controlled by `Common.Player.Subcapture.CompressionType` (`ZSTD` by default, or `LZ4`).

### Two-pass workflow (optimized, default)

When `Common.Player.Subcapture.Optimize` is `true` (the default) and no analysis
file exists yet, the player performs two passes:

1. **Analysis pass** — the first run replays the stream, tracks state, and
   records which objects the requested range actually references. It writes an
   analysis file next to the working directory named
   `<streamName>_frames-<range>_analysis.yml` and logs:

   ``` text
   SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.
   ```

   No output stream is produced in this pass.

2. **Recording pass** — running the player again (with the same options and the
   analysis file now present) produces the trimmed `*.gits2`, restoring **only
   the objects the analysis marked as needed**. This keeps the sub-capture small.

Example (analysis, then recording):

``` bash
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 C:\path\to\full_trace.gits2   # analysis pass
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 C:\path\to\full_trace.gits2   # recording pass
```

!!! note
    An incomplete or corrupt analysis file (for example from an interrupted first run) is ignored and regenerated automatically on the next run.

### Single-pass workflow (restore everything)

Set `Common.Player.Subcapture.Optimize` to `false` to skip the analysis pass.
A single run then produces the trimmed stream, restoring **every** live object
regardless of whether the range uses it. This is simpler and needs only one
run, at the cost of a larger sub-capture.

``` bash
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 --Common.Player.Subcapture.Optimize false C:\path\to\full_trace.gits2
```

## Ray tracing and opacity micromaps

Acceleration structure and micromap contents are restored by **replaying the captured build
commands** against re-uploaded input buffers, not by serializing the structures themselves, so
the sub-capture stays replayable on a different GPU and driver. Micromaps are built before the
acceleration structures that reference them, as
`VUID-vkCmdBuildAccelerationStructuresKHR-micromap-11632` requires.

The analysis pass reduces each object's pre-range operations to the minimal set that reproduces
its final contents. Acceleration structures and micromaps get one such chain each, written to
the analysis file as its `BlasChain` and `MicromapChain` sections. A build or copy into an
object supersedes its chain, a run of updates collapses to `{root build, last update}`, and
copies are never collapsed - each one links the source object's chain to the destination's,
which is what makes build-then-compact replayable. Intermediates the application destroyed
after the copy that read them are re-created for the duration of the replay and destroyed
again afterwards.

| Command | Supported |
|---------|-----------|
| `vkCmdBuildAccelerationStructuresKHR` | yes |
| `vkCmdCopyAccelerationStructureKHR` (CLONE/COMPACT) | yes |
| `vkCmdBuildMicromapsEXT` | yes, `VK_BUILD_MICROMAP_MODE_BUILD_EXT` only |
| `vkCmdCopyMicromapEXT` (CLONE/COMPACT) | yes |
| `vkBuildAccelerationStructuresKHR`, `vkBuildMicromapsEXT`, `vkCopyMemoryToAccelerationStructureKHR`, `vkCopyMemoryToMicromapEXT` | no - their inputs are `VkDeviceOrHostAddress` host pointers, which the recorder never follows, so the source data is not in the stream |
| `vkCopyAccelerationStructureKHR`, `vkCopyMicromapEXT` | no - the source is a tracked handle, but a host-side copy runs outside the command buffer timeline, so neither the written contents nor the source dependency is visible |
| `vkCmdCopyMemoryToAccelerationStructureKHR`, `vkCmdCopyMemoryToMicromapEXT` | no - deserialize paths write content GITS cannot see |
| `vkCmdBuildAccelerationStructuresIndirectKHR` | no |

Readers (`vkCmd/vkCopyMicromapToMemoryEXT`, `vkCmd/vkWriteMicromapsPropertiesEXT`,
`vkCmdWriteAccelerationStructuresPropertiesKHR`) write no structure content and are neither
tracked nor refused. An unsupported command aborts the sub-capture with a message naming it,
rather than silently producing a stream that loses the device on its first trace.

> **Micromap `data` over-capture.** A micromap build's payload buffer has no `dataSize` and no
> build-range analogue, and its exact extent is encoded in the `triangleArray` *contents*, which
> are only valid at build execution. Sub-capture therefore captures the whole tail of the
> backing buffer from `dataOffsetInBuffer`. This is always a correct superset, but can be large
> if a title suballocates micromap payloads out of a big shared upload buffer. A warning naming
> the buffer key is logged when the captured tail greatly exceeds what the usage counts account
> for.

> **Micromaps rebuilt in place are refused.** A micromap's restore reproduces exactly one
> content - whatever its last pre-range operation wrote. If an acceleration structure build the
> restore replays read an *earlier* content of that micromap, the structure would be rebuilt
> over the wrong opacity data, which nothing downstream can detect. The analysis pass detects
> that ordering and aborts with a message naming the micromap and both commands. Changing the
> opacity data a live structure sees is anyway only sanctioned through an acceleration structure
> update with `VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_DATA_UPDATE_BIT_EXT`, not
> through an in-place rebuild.

> **Compacted sizes are assumed not to grow.** A `COMPACT` copy is replayed verbatim into a
> destination created with the size the application queried at capture time. A replay driver
> that compacts the same source less tightly would overrun it. This applies equally to
> acceleration structure and micromap compaction.

## Configuration reference

All options live under `Common.Player.Subcapture` and are shared with the
DirectX backend:

| Option | Default | Meaning |
|--------|---------|---------|
| `Enabled` | `false` | Turn sub-capture on. |
| `Frames` | `""` | Frame range to keep, e.g. `5` or `3-6`. |
| `Optimize` | `true` | Use analysis results to restore only necessary objects (enables the two-pass flow). |
| `CompressionType` | `ZSTD` | Output stream compression (`ZSTD` or `LZ4`). |

The output location is `Common.Player.SubcapturePath` (default
`{install_path}\dump\%f%_%r%`).

!!! warning "DirectX-only options do not apply to Vulkan."
    The `Common.Player.Subcapture.DirectX.*` options (`ExecutionSerialization`, `CommandListExecutions`, `CommandListSplit`, `SerializeAccelerationStructures`, `RestoreTLASes`) are used only by the DirectX backend and are ignored by the Vulkan backend, which supports frame-range sub-capture only.


