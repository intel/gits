---
icon: material/microsoft
title: Sub-capture
---

# Sub-capture

Sub-capture is used to trim a stream. It is possible to generate a sub-capture of a single frame or even a single Command List.

## Frame sub-capture

Frame sub-capture is done with `gitsPlayer.exe`. The player typically runs **twice** for a given range: once for analysis, once to emit the sub-capture.

Configure sub-capture in `gits_config.yml` (next to the player) or override on the command line:

1. Set `Common.Features.Subcapture.Enabled` to `true`.
2. Set `Common.Features.Subcapture.Frames` to the desired range (for example `1800-1803`).

Run `gitsPlayer.exe` twice; use `--exitFrame` to stop right after the sub-capture range finishes.

> **Notes**
>
> - To pick a range, use screenshots, or `--showWindowBorder --showFrameNumberInTitle` so the window title shows the frame number (`--showWindowBorder` is required for the title bar to be visible).
> - Output streams are written under `Common.Player.SubcapturePath` (see `gits_config.yml`).

## Serialized sub-capture

A **serialized** stream is one where command lists are encoded and submitted **one at a time**, in GPU execution order. That layout is required before you can sub-capture individual **command list executions**. Producing this stream does **not** require a tracefile; tracing is only used when you plan [command list sub-capture](#command-list-sub-capture) and need execution indices from a tracefile.

Serialized streams are easier to debug because GITS drives a single command list at a time.

### Produce a serialized trace (one playback)

Point the player at your original trace. Enable sub-capture and **execution serialization**.

Example **1-frame source trace** (target frame 1):

```text
gitsPlayer.exe --Common.Features.Subcapture.Enabled --Common.Features.Subcapture.DirectX.ExecutionSerialization --Common.Features.Subcapture.Frames 1 --exitFrame 1 C:\path\to\1frame_trace.gits2
```

Example **3-frame source trace** (serialize executions for frames 1–2, exit after frame 2):

```text
gitsPlayer.exe --Common.Features.Subcapture.Enabled --Common.Features.Subcapture.DirectX.ExecutionSerialization --Common.Features.Subcapture.Frames 1-2 --exitFrame 2 C:\path\to\3frame_trace.gits2
```

The serialized `*.gits2` is created under the dump directory implied by `Common.Player.SubcapturePath` in `gits_config.yml` (same convention as other sub-captures).

## Command list sub-capture

Command list sub-capture applies only to **serialized-execution** streams. Build a serialized `*.gits2` as in [Serialized sub-capture](#serialized-sub-capture) first. Then follow the steps below: generate a tracefile from that serialized stream, count `Execute #` lines to pick `CommandListExecutions`, and run the player **twice per execution index** (analysis, then sub-capture).

### 1. Generate the trace / dump stream from the serialized trace

Play the **serialized** trace with tracing enabled:

```text
gitsPlayer.exe --Common.Shared.Trace.Enabled C:\path\to\serialized_trace.gits2
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

Example for **1-frame serialized trace** (`--Common.Features.Subcapture.Frames 1`, `--exitFrame 1`):

```text
gitsPlayer.exe --Common.Features.Subcapture.Enabled --Common.Features.Subcapture.Frames 1 --Common.Features.Subcapture.DirectX.CommandListExecutions 7-11 --exitFrame 1 --showWindowBorder --showFrameNumberInTitle C:\path\to\serialized_trace.gits2
```


Replace `7-11` with the range of Command Lists you want to sub-capture.

If subsection **2** reports **50** executions on the target frame, you can generate **50** Command List sub-captures. Example for the 1-frame case:

```text
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 1 --exitFrame 1 ...   # analysis
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 1 --exitFrame 1 ...   # sub-capture
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 2 --exitFrame 1 ...   # analysis
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 2 --exitFrame 1 ...   # sub-capture
# ... repeat the analysis/sub-capture pair for executions 3 through 49 ...
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 50 --exitFrame 1 ...  # analysis
gitsPlayer.exe ... --Common.Features.Subcapture.DirectX.CommandListExecutions 50 --exitFrame 1 ...  # sub-capture
```

Each pair emits a separate `*.gits2` under `Common.Player.SubcapturePath`.

## Command list split (Draw call sub-capture)

**Command list split** rewrites a serialized stream so that chosen commands (typically draws or dispatches) each become their own command list execution in the output. You can then run the usual [Command list sub-capture](#command-list-sub-capture) workflow on that **split** stream and sub-capture at draw  granularity.

### Behavior

- **Input:** A `*.gits2` that already uses serialized executions (one command list per `ExecuteCommandLists`), from [Serialized sub-capture](#serialized-sub-capture).
- **Configuration:** `Common.Features.Subcapture.DirectX.CommandListSplit` in `gits_config.yml`, or `--Common.Features.Subcapture.DirectX.CommandListSplit` on the command line, with `Common.Features.Subcapture.Enabled` set to `true`.
- **Mutual exclusion:** In a split pass, do **not** enable `ExecutionSerialization` or `CommandListExecutions`. The player runs in a dedicated **split** mode and **does not execute** the GPU workload.
- **Split points:** Values are **stream command keys**. Use a trace of the **serialized** stream to pick keys of commands you want to isolate (for example `ID3D12GraphicsCommandList::DrawIndexedInstanced`). Syntax: a single key (`42`), an inclusive range (`10-20`), comma-separated tokens (`10-20,50,60-70`), or `all` to split at every GPU work command. Intervals must not overlap, and a split interval must not span more than one serialized execution.
- **Output:** A new `*.gits2` under `Common.Player.SubcapturePath`, using a directory whose name ends with `split`.

### Workflow: draw-level sub-capture via split, then [Command list sub-capture](#command-list-sub-capture)

1. Produce a serialized stream as in [Serialized sub-capture](#serialized-sub-capture).
2. Trace that serialized file (`--Common.Shared.Trace.Enabled`) and note the **command keys** on the draw or dispatch lines you want as separate executions.
3. Play the **serialized** file once with sub-capture enabled and `CommandListSplit` set to those keys (or ranges).
4. Trace the **split** output and count **`Execute #`** for your target frame, as in [Command list sub-capture](#command-list-sub-capture).
5. **Clear** `CommandListSplit`. On the **split** `*.gits2`, run the normal two-pass-per-index `CommandListExecutions` sub-capture from [Command list sub-capture](#command-list-sub-capture), using the new execution indices from step 4.

Example split pass (adjust keys and paths):

```text
gitsPlayer.exe --Common.Features.Subcapture.Enabled --Common.Features.Subcapture.DirectX.CommandListSplit 100-105,200 C:\path\to\serialized_trace.gits2
```