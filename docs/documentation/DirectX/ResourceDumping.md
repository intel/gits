---
icon: material/microsoft
title: ResourceDumping
---

## RenderTargetsDump

Dumps render targets and depth stencil on draw commands.

### Usage
1. Use trace file to find the desired draw call numbers, which are marked: `Draw #N from frame #M`.
2. Configure the RenderTargetsDump option using the draw and frame numbers.
3. Replay the stream. Resources are dumped to stream_dir/render_targets or to `OutputDir`.

### File names
1. Execution identifying part: `draw_e_Frame#.Execute#.CommandList#.CommandListDraw#`.
2. Trace file identifying part: `_f_Frame#_d_Draw#`.
3. Render target or depth stencil: `_rt_Slot#` or `_ds`.
4. Object key.
5. Format.
6. Optional: plane#, array#, mip#, slice#.

## DispatchOutputsDump

Dumps UAV bound textures on dispatch commands.

Requires an analysis run which analyzes the state of descriptor heaps on execute command lists to determine which resources can be accessed by dispatch calls.

### Usage
1. Use trace file to find the desired dispatch call numbers, which are marked: `Dispatch #N from frame #M`.
2. Configure the DispatchOutputsDump option using the dispatch and frame numbers.
3. Replay the stream. Analysis file is generated in current working directory.
4. Replay the stream. Resources are dumped to stream_dir/dispatch_outputs or to `OutputDir`.

### File names
1. Execution identifying part: `dispatch_e_Frame#.Execute#.CommandList#.CommandListDispatch#`.
2. Trace file identifying part: `_f_Frame#_d_Dispatch#`.
3. Slot#.
4. Object key.
5. Format.
6. Optional: plane#, array#, mip#, slice#.
