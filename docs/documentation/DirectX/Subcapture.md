---
icon: material/microsoft
title: Sub-capture
---

# Sub-capture

Sub-capture is used to trim a stream. It is possible to generate a sub-capture of a single frame or even a single Command List.

## Frame sub-capture

Frame sub-capture is done with `gitsPlayer.exe`. The player typically runs **twice** for a given range: once for analysis, once to emit the sub-capture.

Configure sub-capture in `gits_config.yml` (next to the player) or override on the command line:

1. Set `DirectX.Features.Subcapture.Enabled` to `true`.
2. Set `DirectX.Features.Subcapture.Frames` to the desired range (for example `1800-1803`).

Run `gitsPlayer.exe` twice; use `--exitFrame` to stop right after the sub-capture range finishes.

> **Notes**
>
> - To pick a range, use screenshots, or `--showWindowBorder` so the window title shows the frame number.
> - Output streams are written under `Common.Player.SubcapturePath` (see `gits_config.yml`).

## Serialized sub-capture

A **serialized** stream is one where command lists are encoded and submitted **one at a time**, in GPU execution order. That layout is required before you can sub-capture individual **command list executions**. Producing this stream does **not** require a tracefile; tracing is only used when you plan [command list sub-capture](#command-list-sub-capture) and need execution indices from a tracefile.

Serialized streams are easier to debug because GITS drives a single command list at a time.

### Produce a serialized trace (one playback)

Point the player at your original trace. Enable sub-capture and **execution serialization**.

Example **1-frame source trace** (target frame 1):

```text
gitsPlayer.exe --DirectX.Features.Subcapture.Enabled --DirectX.Features.Subcapture.ExecutionSerialization --DirectX.Features.Subcapture.Frames 1 --exitFrame 1 C:\path\to\1frame_trace.gits2
```

Example **3-frame source trace** (serialize executions for frames 1–2, exit after frame 2):

```text
gitsPlayer.exe --DirectX.Features.Subcapture.Enabled --DirectX.Features.Subcapture.ExecutionSerialization --DirectX.Features.Subcapture.Frames 1-2 --exitFrame 2 C:\path\to\3frame_trace.gits2
```

The serialized `*.gits2` is created under the dump directory implied by `Common.Player.SubcapturePath` in `gits_config.yml` (same convention as other sub-captures).

## Command list sub-capture

Command list sub-capture applies only to **serialized-execution** streams. Build a serialized `*.gits2` as in [Serialized sub-capture](#serialized-sub-capture) first. Then follow the steps below: generate a tracefile from that serialized stream, count `Execute #` lines to pick `CommandListExecutions`, and run the player **twice per execution index** (analysis, then sub-capture).

### 1. Generate the trace / dump stream from the serialized trace

Play the **serialized** trace with tracing enabled:

```text
gitsPlayer.exe --DirectX.Features.Trace.Enabled C:\path\to\serialized_trace.gits2
```

This produces a trace file whose name ends with `tracefile.txt`, plus related dump stream artifacts, for the next subsection.

### 2. Count command list executions from the tracefile

The tracefile records API activity, including **`Execute #N from frame #F`** on `ID3D12CommandQueue::ExecuteCommandLists` and **`Draw #N from frame #F`** on draw calls.

Example lines:

```text
E6294 T0 O23 ID3D12CommandQueue::ExecuteCommandLists(1, [O2374]) Frame #2 Frame Execute #10
57080383 T12464 O2926 ID3D12GraphicsCommandList::DrawIndexedInstanced(48, 1, 0, 0, 0) Frame #2 Frame Draw #57
57081721 T12464 O2375 ID3D12GraphicsCommandList::DrawIndexedInstanced(3, 1, 6, 0, 0) Frame #2 Frame Draw #59
E6298 T0 O23 ID3D12CommandQueue::ExecuteCommandLists(1, [O2926]) Frame #2 Frame Execute #11
```

**Which frame to read**

| Source | Inspect lines for |
|--------|-------------------|
| 1-frame workflow | frame **#1** |
| 3-frame workflow | frame **#2** |

To get the **total number of command list executions** for that frame, find the **`Execute #`** line with the **largest** execution index for the chosen frame (the last such tag for that frame).

### 3. Sub-capture each command list execution (analysis + sub-capture per index)

For each execution index **1 … N** from subsection **2**, the player is run **twice** with the same `CommandListExecutions` value: first pass performs **analysis**, second pass writes the **sub-capture** (same pattern as frame-only sub-capture).

Use the **serialized** trace from [Serialized sub-capture](#serialized-sub-capture) as input. **Do not** pass `ExecutionSerialization` here.

Example for **1-frame serialized trace** (`--DirectX.Features.Subcapture.Frames 1`, `--exitFrame 1`):

```text
gitsPlayer.exe --DirectX.Features.Subcapture.Enabled --DirectX.Features.Subcapture.Frames 1 --DirectX.Features.Subcapture.CommandListExecutions 7-11 --exitFrame 1 --showWindowBorder C:\path\to\serialized_trace.gits2
```


Replace `7-11` with the range of Command Lists you want to sub-capture.

If subsection **2** reports **50** executions on the target frame, you can generate **50** Command List sub-captures. Example for the 1-frame case:

```text
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 1 --exitFrame 1 ...   # analysis
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 1 --exitFrame 1 ...   # sub-capture
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 2 --exitFrame 1 ...   # analysis
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 2 --exitFrame 1 ...   # sub-capture
# ... repeat the analysis/sub-capture pair for executions 3 through 49 ...
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 50 --exitFrame 1 ...  # analysis
gitsPlayer.exe ... --DirectX.Features.Subcapture.CommandListExecutions 50 --exitFrame 1 ...  # sub-capture
```

Each pair emits a separate `*.gits2` under `Common.Player.SubcapturePath`.
