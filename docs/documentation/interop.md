---
icon: material/link-variant
title: OpenGL and Vulkan interop
---

# OpenGL and Vulkan interop

GITS records an application that shares memory and semaphores between OpenGL and Vulkan, through
`GL_EXT_memory_object_win32` and `GL_EXT_semaphore_win32` on the OpenGL side and
`VK_KHR_external_memory_win32` and `VK_KHR_external_semaphore_win32` on the Vulkan side, into
one stream. Both APIs are recorded in one command order, so the commands that synchronize them
replay in the order the application called them. This is supported by the new OpenGL and Vulkan
backends, on Windows.

## Recording

Copy the interceptors of both APIs next to the application executable: `OpenGL32.dll` from
`Recorder/FilesToCopyOGL` and `vulkan-1.dll` from `Recorder/FilesToCopyVulkan`, together with one
`gits_config.yml`. Recording is otherwise the same as for one API; see the
[recorder](recorder.md).

The stream header names the API of the first recorded command as the stream's API and lists every
API the stream has commands of. The player replays all of them, and fails when it was built
without one of them.

Recording supports:

- Memory and semaphores that Vulkan exports with `vkGetMemoryWin32HandleKHR` and
  `vkGetSemaphoreWin32HandleKHR` and OpenGL imports with `glImportMemoryWin32HandleEXT` and
  `glImportSemaphoreWin32HandleEXT`, as NT or KMT handles. The application may import a duplicate
  of an exported NT handle.
- Buffers and textures whose storage is an imported memory object, and `glWaitSemaphoreEXT` and
  `glSignalSemaphoreEXT` on imported semaphores.
- Vulkan memory that is mapped while OpenGL reads it. The host writes of the application are
  recorded, and made visible to OpenGL, before each OpenGL command that may read buffer or texture
  memory, such as draws, dispatches, copies, texture uploads and `glSignalSemaphoreEXT`, as well as
  before each Vulkan queue submission.
- OpenGL buffers that are mapped persistently with `GL_MAP_COHERENT_BIT` and written, but not read,
  by the application. Their writes are recorded before the same OpenGL commands, and at unmap.

Not supported:

- Handles that OpenGL imports but Vulkan did not export in the recorded process, such as handles
  of another process or of another API. The recorder logs an error naming the import, and the
  stream fails to replay it.
- File descriptor imports (`glImportMemoryFdEXT`, `glImportSemaphoreFdEXT`), and therefore
  interop on Linux.
- Coherent persistent maps that the application reads (`GL_MAP_READ_BIT`), and coherent
  persistent maps on Linux. The recorder stops with an assertion naming the map.

## Replay

Replay the stream with `gitsPlayer` as usual; see the [player](player.md). The player replays the
Vulkan exports, and the OpenGL imports use the handles the replayed exports returned.

!!! warning "OpenGL and Vulkan must use the same GPU"
    A handle can be imported only on the GPU that exported it. On a system with more than one GPU,
    the Vulkan device the stream selects and the GPU the OpenGL context is created on must be the
    same; the simplest way is to replay on a system with one active GPU. The player does not
    check this.

## Subcapture

Frame subcapture works as for one API; see [Vulkan subcapture](Vulkan/Subcapture.md). For an
interop stream:

- Frames are those of the API that ends the first frame. In an application that renders with
  OpenGL and computes with Vulkan, a frame ends at each OpenGL swap. Subcapture of a stream in
  which more than one API ends frames is not supported and stops with an error.
- The subcapture is one stream with one state restore of both APIs, Vulkan's first.
- Each API writes its own analysis file, `<stream>_frames-<range>_opengl_analysis.yml` and
  `<stream>_frames-<range>_vulkan_analysis.yml`. Both APIs run the analysis pass until the files
  of both are complete, and with `Optimize`, written by an optimized analysis; then both record.

## Troubleshooting

| Symptom | Cause |
|---------|-------|
| The recorder logs `... imports a handle that no recorded command exported` | The application imports a handle that Vulkan did not export in this process. |
| Replay fails with `... imports a handle of command N, which did not export one in this replay` | The recorder could not match the import to an export when the stream was recorded; see the previous row. |
| Replay renders, but imported buffers or textures stay empty or corrupt | OpenGL and Vulkan run on different GPUs during replay; see [Replay](#replay). |
| Subcapture of a stream recorded with an older GITS reports `VUID-vkCmdCopyBuffer-srcBuffer` validation errors | The stream was recorded before the recorder honored `VkBufferUsageFlags2CreateInfo`, so its buffers cannot be copied during the state restore. Re-record the stream. |
