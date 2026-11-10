---
icon: simple/vulkan
title: ResourceDumping
---

# Resource dumping (Vulkan)

This page covers the **new Vulkan backend**. Both dumps require `Common.Player.Execute` and can be
enabled together.

## ResourcesDump

Dumps the selected buffers and images during replay, after the selected commands. Configure it in
the `Vulkan.Player.ResourcesDump` group:

| Option | Description |
|---|---|
| `enabled` | Enables the dump. |
| `commandKeys` | Keys of the commands after which the resources are dumped. |
| `resourceKeys` | Keys of the buffers and images to dump. Nothing is dumped if empty. |
| `format` | `PNG` or `JPEG` for images that can be converted to 8 bit RGBA. |

Keys are the numbers printed at the start of trace lines (`Common.Shared.Trace.Enabled`). Object
keys may keep their `O` prefix, and keys of state restore calls of sub-captured streams take an
`S` prefix.

### Usage

1. Replay the stream with the trace enabled.
2. Find the keys of the buffers and images in their `vkCreateBuffer`, `vkCreateImage` or
   `vkGetSwapchainImagesKHR` calls, and the keys of the commands after which to dump them.
3. Replay the stream with the dump enabled, for example:

```
gitsPlayer.exe --Vulkan.Player.ResourcesDump.Enabled --Vulkan.Player.ResourcesDump.CommandKeys=13641,13643 --Vulkan.Player.ResourcesDump.ResourceKeys=O22,O7 path\to\stream.gits2
```

Resources are dumped to `stream_dir/resources` or to `OutputDir`.

### Commands

* **`vkQueueSubmit`, `vkQueueSubmit2`:** the resources are dumped after all the work submitted so
  far to the queue, including this submission, has completed. The semaphore and fence signals of
  the submission are delayed until the copies complete. If they cannot be (for example, when a
  later batch of the submission waits for semaphores after an earlier one signals, when the
  semaphore signals are limited to pipeline stages, or when the submission or the earlier work of
  the queue waits for a semaphore value that may be signaled after the submission), an error names
  the reason and nothing is dumped after the submission.
* **`vkCmd*` and `vkEndCommandBuffer`:** the dump is recorded into the command buffer right after
  the command, and the resources are dumped once the command buffer has executed. Commands inside
  a render pass instance cannot be followed by copies, so use the key of `vkCmdEndRenderPass*`,
  `vkCmdEndRendering*` or of the submission instead. A render pass instance that suspends
  (`VK_RENDERING_SUSPENDING_BIT`) continues in a later command buffer, so nothing is dumped after
  the command that suspends it, and a submission that executes a dump after the suspension and
  before the render pass instance that resumes it stops the replay with an error. A command buffer
  that is submitted multiple times dumps the resources after its first execution only. Protected
  command buffers cannot write the staging memory, so use the key of the submission for their
  commands. Commands inside a video coding scope cannot be followed by copies either, so use the
  key of `vkCmdEndVideoCodingKHR` or of the submission instead. A `vkQueueSubmit2` that executes
  the dump and, in the same or a later batch, signals semaphores limited to pipeline stages stops
  the replay with an error, as those signals may not wait for the copies. The copies wait for
  every semaphore wait before them, of the submission and of the earlier submissions to the queue.
  So a submission that executes the dump, signals semaphores after it, and waits before it for a
  semaphore value that may be signaled after the submission and that the submission does not
  signal before, stops the replay with an error, as the value may depend on those signals. So does
  a submission that executes the dump and signals semaphores or a fence after it, when the earlier
  work of the queue waits for such a value. The copies are recorded for the layouts and queue
  family owners that the command buffers recorded before the dump leave. A submission in which
  other command buffers execute before the copies, so that the resources are in other layouts or
  not owned by their queue family, stops the replay with an error.

A semaphore value may be signaled after a submission when no work submitted before it signals that
timeline semaphore value, or when the signal operation that the value or a binary semaphore wait
waits for itself waits for such a value.

### Swapchain images

Unless the replay renders offscreen, a swapchain image is copied only while it is acquired and not
presented or released since, on a queue that has waited for the semaphore of its acquisition, or on
any queue if it was acquired with a fence only. Otherwise, the presentation engine may still use
it: after a submission, the image is skipped with a warning, and a submission that executes a dump
of it recorded into a command buffer stops the replay with an error. Shared presentable images are
copied in `VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR`, which they may not leave.

### File names

1. Command key: `command_Key#`.
2. Resource kind and key: `_buffer_O#` or `_image_O#`.
3. Images only, optional: `_plane_#` or `_depth`/`_stencil`, `_array_#`, `_mip_#`, `_slice_#`.

## RenderTargetsDump

Dumps the color and depth/stencil attachments of the selected render pass instances when they
end. Configure it in the `Vulkan.Player.RenderTargetsDump` group:

| Option | Description |
|---|---|
| `enabled` | Enables the dump. |
| `frames` | Frame range, for example `3-6`. |
| `renderPasses` | Range of render pass instances in each frame, for example `1-100:5`. |
| `format` | `PNG` or `JPEG` for images that can be converted to 8 bit RGBA. |
| `dryRun` | Writes `RenderTargetsDumpDryRun.yaml`, listing the selected render pass instances that have render targets by frame, instead of dumping them. |

Frames are counted from 1 and end at `vkQueuePresentKHR`. The state restore of a sub-captured
stream, with the present it records to show the restored state, is frame 0, so frame 1 is the
first frame of the subcapture. Render pass instances (`vkCmdBeginRenderPass*`, or
`vkCmdBeginRendering*` without `VK_RENDERING_RESUMING_BIT`) are counted from 1 in each frame, in
recording order. A render pass instance that suspends and resumes is dumped once, when its last
part ends.

Render targets are dumped to `stream_dir/render_targets` or to `OutputDir`, for example:

```
gitsPlayer.exe --Vulkan.Player.RenderTargetsDump.Enabled --Vulkan.Player.RenderTargetsDump.Frames=1-2 --Vulkan.Player.RenderTargetsDump.RenderPasses=1-10 path\to\stream.gits2
```

### Render targets

For render passes, the render targets are the attachments that a subpass uses as color, resolve or
depth/stencil attachments, numbered by their index in the render pass. For dynamic rendering, they
are the color attachments, numbered by their index in `VkRenderingInfo`, and the depth and stencil
attachments, each followed by its resolve attachment if it has one. The contents are the ones at
the end of the render pass instance, after the store and resolve operations. Only the layers of the
attachment image view that the render pass instance renders are dumped: the framebuffer layers or
`VkRenderingInfo::layerCount` layers, or with multiview the layers of the views in the view mask.

A render target is skipped with a warning if its store operation is not
`VK_ATTACHMENT_STORE_OP_STORE`, as its contents are then undefined. Multisampled render targets
are skipped; their resolve attachments are dumped instead. Render pass instances in protected
command buffers are skipped, as these cannot write the staging memory.

### File names

1. Frame and render pass instance: `renderpass_f_#_rp_#`.
2. Render target and image key: `_rt_#_O#` (color) or `_ds_O#` (depth/stencil, `_ds_#_O#` if a
   render pass has several). Resolve attachments of dynamic rendering add `_resolve`, as in
   `_rt_0_resolve_O#` or `_ds_resolve_O#`.
3. Image format without the `VK_FORMAT_` prefix, for example `_B8G8R8A8_UNORM`.
4. Optional: `_depth`/`_stencil`, `_array_#`, `_mip_#`, `_slice_#`.

## Limitations

A resource is skipped with a warning if it is sparse, protected, not bound to memory, created
without the transfer source usage, owned by another or an external queue family, shared with
`VK_SHARING_MODE_CONCURRENT` by queue families other than the one copying it, multisampled, or
if its format is unknown. Subresources with undefined contents (layout `VK_IMAGE_LAYOUT_UNDEFINED`
or `VK_IMAGE_LAYOUT_PREINITIALIZED`) are skipped. The transfer source usage is added to swapchains
if their surface supports it. If `VkImageStencilUsageCreateInfo` gives the transfer source usage to
only one aspect of a depth/stencil image, only that aspect is dumped, and only if
`separateDepthStencilLayouts` is enabled.

Images in `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` or `VK_IMAGE_LAYOUT_GENERAL` are copied in their
layout. Images in other layouts are moved to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` for the copies
and back after them, which work of other queues does not wait for, so dump such an image only where
no other queue may access it at the same time.

Nothing is dumped after commands and submissions of queue families without graphics, compute or
transfer support (for example, video-only queue families), and an error is logged. Queue families
without graphics support copy the depth or stencil of an image only if `maintenance10` is enabled
and the format supports such copies on their queues. Other depth/stencil aspects are skipped as
above.

Staging memory for the copies is allocated for each dump. If an allocation fails, the replay
stops: dump fewer resources at once, or dump on a system with more memory. If waiting for the copies
fails, the replay stops as well.

The resources are written once the work they wait for completes, which may wait for semaphore
values that later replayed calls signal. If the replay ends before it signals them, for example at
`ExitFrame` or after an error, the player stops waiting once no resource has been written for 10
seconds, logs the resources that are not written, and fails.

Buffers are written as `.bin` files. Images are written as `.png` or `.jpg` files if their format
can be converted (8 and 16 bit normalized, 16 and 32 bit float, 10 bit packed, depth and
stencil), and as `.bin` files with the tightly packed texels otherwise.
