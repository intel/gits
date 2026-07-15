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

> **New vs. legacy backend.** The older **VulkanLegacy** backend trims a stream
> **during recording** instead (copy the recorder into the app directory and set
> a recorder mode such as `Vulkan.Recorder.Mode = Frames`). That recorder-side
> flow does **not** apply to the new backend, and the player-side flow described
> here does **not** apply to legacy streams. You never pick the backend at
> playback time — see
> [VulkanLegacy vs Vulkan backends](../../usage.md#vulkan-vulkanlegacy-vs-vulkan-backends)
> for how streams are routed.

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

> **Notes**
>
> - `Frames` accepts a single frame (`5`) or an inclusive range (`3-6`). A frame
>   boundary is a `vkQueuePresentKHR`; frame *N* is the *N*-th presented frame
>   (1-based).
> - The player restores the full Vulkan state (memory contents, image layouts,
>   descriptor sets, pipelines, synchronization state, ...) at the start of the
>   range, then records the in-range commands, so the trimmed stream stands on
>   its own.
> - Output-stream compression is controlled by
>   `Common.Player.Subcapture.CompressionType` (`ZSTD` by default, or `LZ4`).

### Two-pass workflow (optimized, default)

When `Common.Player.Subcapture.Optimize` is `true` (the default) and no analysis
file exists yet, the player performs two passes:

1. **Analysis pass** — the first run replays the stream, tracks state, and
   records which objects the requested range actually references. It writes an
   analysis file next to the working directory named
   `<streamName>_frames-<range>_analysis.yml` and logs:

   ```text
   SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.
   ```

   No output stream is produced in this pass.

2. **Recording pass** — running the player again (with the same options and the
   analysis file now present) produces the trimmed `*.gits2`, restoring **only
   the objects the analysis marked as needed**. This keeps the sub-capture small.

Example (analysis, then recording):

```text
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 C:\path\to\full_trace.gits2   # analysis pass
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 C:\path\to\full_trace.gits2   # recording pass
```

> An incomplete or corrupt analysis file (for example from an interrupted first
> run) is ignored and regenerated automatically on the next run.

### Single-pass workflow (restore everything)

Set `Common.Player.Subcapture.Optimize` to `false` to skip the analysis pass.
A single run then produces the trimmed stream, restoring **every** live object
regardless of whether the range uses it. This is simpler and needs only one
run, at the cost of a larger sub-capture.

```text
gitsPlayer.exe --Common.Player.Subcapture.Enabled --Common.Player.Subcapture.Frames 3-6 --Common.Player.Subcapture.Optimize false C:\path\to\full_trace.gits2
```

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

> **DirectX-only options do not apply to Vulkan.** The
> `Common.Player.Subcapture.DirectX.*` options (`ExecutionSerialization`,
> `CommandListExecutions`, `CommandListSplit`, `SerializeAccelerationStructures`,
> `RestoreTLASes`) are used only by the DirectX backend and are ignored by the
> Vulkan backend, which supports frame-range sub-capture only.
