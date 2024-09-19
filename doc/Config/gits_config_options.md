
# Config options

## Basic

- `RecordingMode` - Enables GITS recording. Available modes: 
    - None - recording disabled, useful for logging API calls without recording 
    - Binary - enables recording in the format supported by GITS player
    - CCode - enables recording in C-code format.

- `LogLevel` - Only info with level greater or equal to this will be logged. The levels are: TRACEVERBOSE, TRACE, INFO (default), WARNING, ERROR and OFF. API calls are logged at levels TRACE and lower (lower means more verbose).

- `ExitKeys` - If keys specified, after pressing key combination GITS plugin will be unloaded from the process. Option available on: Windows (all alpha/special keys)

- `ExitAfterAPICall` - Enables forcing dumping of GITS artifacts after specific number of API calls (0 - do not dump)

- `ExitSignal` - Linux only; Signal used to unload GITS recorder. Default signal is SIGTERM(15).

### Basic.Paths

- `LibGL` - Path to library containing actual implementation of OpenGL API.

- `LibEGL` - Path to library containing actual implementation of EGL API.

- `LibGLES1` - Path to library containing actual implementation of OpenGL ES 1 API.

- `LibGLES2` - Path to library containing actual implementation of OpenGL ES 2 API.

- `LibCL` - Path to library containing actual implementation of OpenCL API.

- `LibVK` - Path to library containing actual implementation of Vulkan API.

- `LibOcloc` - Path to library containing actual implementation of Ocloc API.

- `LibL0` - Path to library containing actual implementation of LevelZero API.

- `InstallationPath` - Path to GITS Recorder directory (containing GITS recorder library).

- `DumpDirectoryPath` - Specified directory will contain newly created stream. `%p%` will be substituted with process id of a process that created the stream. On Windows also: `%n%` will be substituted with process name of a process that created the stream.

- `UniqueDumpDirectory` - The dump directory name for each recorder execution will be postfixed with unique timestamp.

## OpenGL

### Capture

- `Mode` - Specifies mode of operation and triggering behaviour of GITS recorder. Valid modes are All, Frames, OglSingleDraw, OglDrawsRange. This also selects option group in OpenGL.Capture that is active.

### Capture.All

- `ExitFrame` - After this frame, recorder will stop operation and write the stream to filesystem.

- `ExitDeleteContext` - Number of context deletion GL function on which to stop recording. Zero means disable feature.

### Capture.Frames

- `StartFrame` - First frame to be recorder by GITS recorder.

- `StopFrame` - The frame number when recording ends (it will be the last frame recorded). If '0' capture will be ended together with tested application.

- `StartKeys` - Key combination beginning capture. If given, capture start and stop are relative to frame in which keypress occurred. If empty, capture start / stop are relative to the beginning of the application. Option available on: Windows (all alpha/special keys).

### Capture.Frames.FrameSeparators

- `glFinish` - Specifies glFinish as an additional frame separator. It makes Start/StopFrame values to be counted as a total number of glFinish and SwapBuffers calls.

- `glFlush` - Specifies glFlush as an additional frame separator. It makes Start/StopFrame values to be counted as a total number of glFlush and SwapBuffers calls.

### Capture.OglSingleDraw

- `Number` - Number of drawcall that is to be recorded. For this draw state minimization will be performed.

### Capture.OglDrawsRange

- `StartDraw` - Number of drawcall that will begin stream capture.

- `StopDraw` - Number of last draw to be included in the capture.

- `Frame` - When set to zero, StartDraw and StopDraw are relative to the beginning of the application and draws range may span many frames. Otherwise it's the number of the frame, whose draws will be captured and StartDraw/StopDraw are relative to the beginning of that frame (only draws belonging to it will be recorded).

### Utilities

- `TraceGLError` - If enabled, trace will be amended with error code if any logged function caused an error status. This causes error checking function to be invoked after every logged API call.

- `ForceGLVersion` - Overrides the OpenGL version string or number returned to the application. This is useful for changing the behavior of applications.

- `SuppressExtensions` - List of extensions to be removed from extension string during recording.

- `SuppressProgramBinary` - If enabled, recorded applications that use binary programs/shaders will be forced to fail submitting the binaries to driver, by overriding binary length to 1. This is meant to force application to provide textual shaders definition so that the stream can be played back on other drivers.

- `EndFrameSleep` - Sleep this many milliseconds after each frame.

- `RestoreDefaultFB` - Option removing restoration of default frame buffer (back or front) content.

- `DoNotRemoveWindow` - Option skip scheduling Window removing.

- `MultiApiProtectBypass` - Multi API application (for ie those using ES1 and ES2 contexts) are not supported by GITS. Recording such an application will cause an error in gits_xxx.log. Nevertheless it is sometimes possible that such an application may be recorded.

- `CArrayMemCmpType` - Specifies the mode of client arrays memory comparison. 
    - 0 - All. Create diff from whole used memory range.
    - 1 - One Range. Create smallest possible one memory range containing all diffs.
    - 2 - Multi Range. Create many memory range with diffs.

- `StripIndicesValues` - Ignore specified value of index, when scanning index buffers. For 8 and 16 bit indices, lower order bits are used only.

- `OptimizeBufferSize` - If application is using buffers mappings this option causes only changed portion of the buffer to be dumped. Disabling this option causes entire buffer to be dumped on every buffer unmap api call but skips compare operations. From measurements done so far on Intel HW enabling this option improves: buffer size, recording performance and player performance so it should be always on. On other HWs user may consider disabling this option what may improve recorder performance.

- `RetryFunctionLoads` - If a pointer to an OpenGL function in the driver cannot be loaded (e.g. because a context has not been created yet), reattempt to load it the next time application calls that function.

- `DetectRecursion` - Enable/disable detection of recursion when LibDirectory points to gits library instead of system library.

- `BuffersState` - Use to specify buffers restoration type. Set to CaptureAlways, Restore or Mixed.
    - CaptureAlways - Calls related with buffers are recorded before first frame recording.
    - Restore - Attempt to read data written to write only buffer object mappings. This also allows for state restore of buffer objects in GLES (but is technically undefined). That means calls related with buffers are restored (using state restore), and they are not recorded. Advantage of this method is shorter stream, but this method cannot be used when pixel buffer objects (textures loaded from buffers) are used.
    - Mixed - Merge of restoration type 0 and 1. This method should be work with Transform Feedback objects, and with pixel buffer objects.

- `TexturesState` - Use to specify textures restoration model. Set to CaptureAlways, Restore or Mixed.
    - CaptureAlways - Calls related with textures are recorded before first frame recording.
    - Restore - Enables textures restoration in OpenGLES instead of recording all texture related calls from the beginning. The downside of this option is that it changes texture format what can cause differences in rendered image and fails of texsubimage like calls.
    - Mixed - Merge of restoration type 0 and 1. With this option content of renderable textures with format GL_RGBA and type GL_UNSIGNED_BYTE is being updated in state restoration process.

- `CoherentMapUpdatePerFrame` - If set to true, this option enforces updating of the content of buffers, that have been mapped with MAP_COHERENT_BIT flag present, only once per frame. Otherwise updates are performed per every drawcall.

- `ScheduleFboEXTAsCoreWA` - Windows only; Schedule EXT framebuffers calls (extensions: GL_EXT_framebuffer_object, NV_geometry_program4) as Core calls.

- `UseGlGetTexImageAndRestoreBuffersWhenPossibleES` - Windows only; Try to use glGetTexImage on OpenGL ES when its exposed by the API.

- `TrackTextureBindingWA` - Windows only; If set to true, gits relies on tracking of currently bound textures via consecutive glBindTexture calls, instead of querying that information from driver with glGetIntegerv.

- `ForceBuffersStateCaptureAlwaysWA` - Windows only; Affects state restoration process. If enabled, calls related to buffers are recorded before first frame recording.

- `RestoreIndexedTexturesWA` - Windows only; If enabled, textures, whose format is GL_COLOR_INDEX and internal format is GL_RGBA, are restored with glGetTexImage as being of GL_RGBA format.

### Images

- `DumpScreenshots` - Rangespec with frames to be captured during the recording.

- `DumpDrawsFromFrames` - Rangespec with frames, from which all drawcalls will be captured.

## OpenCL

### Capture

- `Mode` - Mode of operation of OpenCL capture, allowed values are All, OclSingleKernel, OclKernelsRange.

### OclSingleKernel

- `Number` - A clEnqueueNDRangeKernel call number to be captured. Indexing starts at 1.

### OclKernelsRange

- `StartKernel` - A clEnqueueNDRangeKernel call number to start capture Indexing starts at 1.

- `StopKernel` - A clEnqueueNDRangeKernel call number to stop capture. Indexing starts at 1.

### Utilities

- `DumpKernels` - Rangespec with kernels to be captured during the recording.

- `DumpImages` - Enables capture OCL images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.

- `OmitReadOnlyObjects` - Omits dumping for objects created with CL_MEM_READ_ONLY.

- `BufferResetAfterCreate` - Nullifies Buffer, Image, USM and SVM memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.

- `NullIndirectPointersInBuffer` - Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.

## Vulkan

### Capture

- `Mode` - Specifies mode of operation and triggering behaviour of GITS recorder. Valid modes are All, Frames, QueueSubmit, CommandBuffersRange, RenderPassRange, DrawsRange, DispatchRange, BlitRange. This also selects option group in Vulkan.Capture that is active.

### Capture.All

- `ExitFrame` - After this frame, recorder will stop operation and write the stream to filesystem.

### Capture.Frames

- `StartFrame` - First frame to be recorder by GITS recorder.

- `StopFrame` - The frame number when recording ends (it will be the last frame recorded). If '0' capture will be ended together with tested application.

- `StartKeys` - Key combination beginning capture. When given, capture start/stop frames are relative to frame in which keypress occurred. If empty, capture start/stop are relative to the beginning of the application. Option available on: Windows (all alpha/special keys)

### Capture.QueueSubmit

- `Number` - Only this queue submit will be recorded. The format is "queue_submit_number".

### Capture.CommandBuffersRange

- `Range` - Only these command buffers will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffers_range"

### Capture.RenderPassRange

- `Range` - Only these render passes will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_range"

### Capture.DrawsRange

- `Range` - Only these draws will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_number/draws_range"

### Capture.DispatchRange

- `Range` - Only these dispatches will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/dispatch_range"

### Capture.BlitRange

- `Range` - Only these blits will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/blit_range"

### Utilities

- `TraceVKStructs` - Trace values of Vulkan structs instead of tracing pointers.

- `MemorySegmentSize` - If application is using memory mappings this option causes only changed portion of the memory to be dumped. Disabling this option causes entire memory to be dumped on every memory unmap api call or before vkQueueSubmit, when memory is mapped or used in some submitted CommandBuffer (depending on Vulkan.Utilities.MemoryUpdateState setting), but skips compare operations. This option increase RAM usage during recording and used standalone (without Vulkan.Utilities.MemoryAccessDetection) is very slow in some apps (e.g. Talos Principle). In contrast to Vulkan.Utilities.MemoryAccessDetection this option compare memory byte by byte instead of updating all touched pages (one page has 4096 KB, so this option can better optimize update size).

- `MemoryTrackingMode`
    - External - Windows only. When this option is enabled, GITS replaces memory allocations provided by a driver with its own, host memory allocations by using VK_EXT_external_memory_host extension. This is done only on memory objects which can be mapped so, after mapping, applications write directly to the GITS recorder's own memory, which makes memory updates tracking simpler. This way, GITS does not have to allocate additional memory (so memory usage shouldn't increase too much) and at the same time it is still possible to mark those memory areas as READ_ONLY. Enabling this option may impact rendering perfomance, though, but the influence varies depending on each, specific application.
    - ShadowMemory - When this option is enabled, GITS creates its own copy (a shadow memory) of each mapped memory object and passes replaced pointers to recorded applications so they write to the provided memory. If an application is using memory mappings, this option causes GITS to dump only memory pages that were written into. Memory changes are tracked by setting a READ_ONLY flag on mapped memory objects in conjunction with special exception handling routines, allowing GITS to track which pages are used for writing. This option increases RAM usage, so if a platform doesn't have a lot of memory then shadow memory should not be used together with `Vulkan.Utilities.MemorySegmentSize`. (Exact value depends on the app's memory usage, but if it uses 2GB of RAM, it is recommended to have at least 6GB of RAM to use these two options together).
    - WriteWatch - Windows only. When this option is enabled, GITS creates its own copy (a shadow memory) of each mapped memory object and passes replaced pointers to recorded applications so they write to the provided memory. If an application is using memory mappings, this option causes GITS to dump only memory pages that were written into. Memory changes are tracked using the WriteWatch mechanism, which allows GITS to efficiently monitor which pages are written. This option increases RAM usage, so if a platform doesn't have a lot of memory then shadow memory should not be used together with `Vulkan.Utilities.MemorySegmentSize`. (Exact value depends on the app's memory usage, but if it uses 2GB of RAM, it is recommended to have at least 6GB of RAM to use these two options together).
    - FullMemoryDump - When this option is enabled, GITS dumps the entire memory content before `vkQueueSubmit`. If used together with `Vulkan.Utilities.MemorySegmentSize`, GITS performs a binary comparison byte by byte to reduce the memory content that needs to be dumped. This option ensures that all memory changes are captured, but it can significantly increase the amount of data being recorded and is very slow due to the binary comparison process. It is recommended to use this option only when other options did not work.

- `ForceUniversalRecording` - GITS recorder checks if it's attached to a GITS Player to use internal methods for detecting memory changes and improving recording performance. Setting this option to true causes recorder to disable these internal mechanisms and act as if it is recording any other application.

- `MemoryUpdateState` - If application is using memory mappings in coherent mode we need to synchronize memory before vkQueueSubmit. There is two available modes:
    - AllMapped - this option cause to update all mapped memory at this moment
    - OnlyUsed - this option cause to update only mapped memory used in submitting CommandBuffers, so we track information about used buffers and images.

- `DelayFenceChecksCount` - This value postpones fence's signaled state to be returned to application; application has to check fence's status or wait for a given fence provided  number of times before the signaled state is returned.

- `ShortenFenceWaitTime` - Number of nanoseconds to shorten the time application waits on fences; this value is subtracted from the timeout value provided to vkWaitForFences() function.

- `SuppressExtensions` - Array of extensions names to be removed from both Instance and/or Device extenstions reported to recorded application by the driver.

- `SuppressLayers` - Array of layers names to be removed from both Instance and/or Device layers reported to recorded application by the driver.

- `AddImageUsageFlags` - Single value representing bit flags which should be added to all images at creation time.

- `AddBufferUsageFlags` - Single value representing bit flags which should be added to all buffers at creation time.

- `ScheduleCommandBuffersBeforeQueueSubmitWA` - writing commandBuffers operation to stream just before QueueSubmit, instead of doing it in the same way as app/game/benchmark.

- `MinimalStateRestore` - Restore only objects which are actually used in the substream. Option available only for modes QueueSubmit and CommandBuffersRange.

- `ReusableStateRestoreResourcesCount` - Number of separate resource sets which are concurrently used to restore contents of images and buffers. It cannot be smaller than 2.

- `ReusableStateRestoreBufferSize` - The size (in megabytes) of temporary buffers used to restore contents of images and buffers. The greater the size, the less copy operations will be performed (and the state restore phase in a recorded substream will be faster) but the memory consumption will be higher.

- `SuppressPhysicalDeviceFeatures` - Array of physical device features names to be removed from features reported by the driver. Physical device features are reported with vkGetPhysicalDeviceFeatures() and vkGetPhysicalDeviceFeatures2KHR() function in the VkPhysicalDeviceFeatures structure. Name of each member of this structure can be provided in the option. I.e. to suppress 'geometry shaders' and 'cubemap arrays' specify: SuppressPhysicalDeviceFeatures ["imageCubeArray", "geometryShader"]

- `MemoryRestoration` - Controls restoration of contents of memory objects. The following options are available:
    - None - contents of memory objects are not restored
    - HostVisible - restores contents of all memory objects allocated from a host-visible memory

- `RestoreMultisampleImagesWA` - Restoration of multisample images via vkCmdCopyImageToBuffer/vkCmdCopyBufferToImage is forbidden by spec, but it is supported by Intel's driver at the time of this writing. Enabling this option fixes corruptions in some titles, when stream recorded on Gen9 is played back on Gen12.

- `MaxArraySizeForCCode` - Number that defines the maximum size of the array that will be written to CCode in one scope. Applies to: vkCmdPipelineBarrier(), vkCmdCopyBufferToImage(), vkCmdCopyBuffer(), vkUpdateDescriptorSets().

### Utilities.IncreaseImageMemorySizeRequirement

- `FixedAmount` - Amount of bytes by which the size of image memory requirement reported to application should be increased.

- `Percent` -  Percentage by which the size of image memory requirement reported to application should be (relatively) increased.

### Utilities.MemoryOffsetAlignmentOverride

- `Images` - defines the memory offset alignment that is returned to the application through a vkGetImageMemoryRequirements() function call in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 4096. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

- `Buffers` - defines the memory offset alignment that is returned to the application through a vkGetBufferMemoryRequirements() function call in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

- `Descriptors` - defines the memory offset alignment that is returned to the application through a vkGetPhysicalDeviceProperties() function call in minTexelBufferOffsetAlignment, minUniformBufferOffsetAlignment and minStorageBufferOffsetAlignment members of a VkPhysicalDeviceProperties::VkPhysicalDeviceLimits structure. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

### Utilities.CrossPlatformStateRestoration

- `Images` - Allows for proper image contents restoration; this is performed through appropriate image layout transitions and data transfer between temporary staging buffer and the image; this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Buffers SHOULD allow playing recorded substreams on various hardware platforms (though the compatibility isn't guaranteed) for the sake of bigger stream size and longer state restore time.

- `Buffers` - Allows for proper buffer contents restoration; this is performed through data transfer between temporary staging buffer and the target; this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Images SHOULD allow playing recorded substreams on various hardware platforms (though the compatibility isn't guaranteed) for the sake of bigger stream size and longer state restore time. The following options for buffer memory restoration are available:
    - None - don't restore buffers' memory contents
    - WithNonHostVisibleMemoryOnly - restore contents of buffers with non-host-visible memory bound (buffers with device-local-only memory)
    - All - restore contents of all buffers

### Images

- `DumpScreenshots` - Rangespec with frames to be captured during the recording.

- `DumpSubmits` - Rangespec with QueueSubmits, from which all submits will be captured.

## LevelZero

### Capture

- `Mode` - Mode of operation of LevelZero capture, allowed values are All, Kernel

- `All` - Mode of operation of LevelZero capture. Captures whole stream

- `Kernel` - Mode of operation of LevelZero capture. Captures a range of kernels. If chosen one kernel in the range it acts like a single subcapture. Format: CommandQueueSubmitRange/CommandListRange/AppendKernelsRange. Each range must exist and may not go out of boundries. For proper numbering see the recorder's log with LogLevel set to at least TRACE. Format examples:
    - 5/5/5
    - 5,7,8-10/5,7,8-10/5,7,8-10
    - 1-10:2/1-10:2/1-10:2

### Utilities

- `DumpKernels` - Format QueueSubmitRange/CommandListRange/AppendKernelsRange. For proper numbering understanding look at recorder's log with LogLevel set to at least TRACE. Injects reads with synchronization points. Injected reads are appended into existing command lists. It may cause the host running out of memory, as reads writes into GITS reserved memory space(which GITS holds until queue submit), however, the true gain is the NDRangeBuffer after each executed kernel. If that specific verification is not needed, it is recommended to run this option with DumpAfterSubmit set to True.

- `DumpAfterSubmit` - odifies the way kernel arguments are dumped. Injects immediate command lists, reads and synchronization points after queue submit, instead of injecting read after every appendKernel call. This way dumping process is after command queue execution containing N command lists. If kernel argument is overwritten by many kernels, the output buffer will have the value after last executed kernel that used it. All of other same buffers are skipped. Option reduces allocated memory by GITS.

- `DumpImages` - Enables capture L0 images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.

- `BufferResetAfterCreate` - Nullifies USM Buffer and Image memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.

- `NullIndirectPointersInBuffer` - Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.


## Extras

### Optimizations

- `TokenBurstLimit` - The amount of tokens to be used for each recording 'burst'. This many tokens will be accumulated before any IO happens. Packs of that many tokens recorder will be periodically written to HDD in separate thread of execution. Increasing the value will have direct impact on memory used by the recorder. GITS will keep a few packs of that many tokens to facilitate hiding of IO latency.

- `BufferMapAccessMask` - This mask may be used to remove undesired options from buffer mapping access bitfield specified by glMapBufferRange for example. By default GITS removes GL_MAP_INVALIDATE_BUFFER_BIT and GL_MAP_INVALIDATE_RANGE_BIT to improve performance of the recorder. Mask removes those options in recorder so it has an influence on how application and further also stream behaves.

- `BufferStorageFlagsMask` - This mask may be used to add options to storage flags bitfield specified by glBufferStorage or glNamedBufferStorageEXT. By default GITS adds GL_MAP_READ_BIT.

### Utilities

- `ExtendedDiagnostic` - Enables gathering of system diagnostic info during stream recording. Turn off if Gits causes application to crash during startup.

- `ForceDumpOnError` - Enables forcing dumping of GITS artifacts when an error is detected. May lead to abort or other serious problem.

- `ZipTextFiles` - If enabled, shaders, kernels and other text files produced by GITS will be stored in a zip archives instead of directories.

- `HighIntegrity` - Forces recorder to operate in high integrity mode, which can heavily impact recorder performance, but will allow for creation of usable stream, even if recorded application crashes before end of recording. Stream is effectively persisted to filesystem after each API call.

- `NullIO` - Testing mode which does not perform any IO, thus useful only for performance testing of GITS recorder itself.

- `EventScript` - Absolute path to Lua event script to be used during the recording.

- `RemoveAPISharing` - Removes sharing API calls and replaces it with its 'equivalent' in the pure API. Supported APIs to remove in OpenCL: 'DX', 'OGL'. If specified 'All' all API sharing is removed.

- `CoherentMapBehaviorWA` - WA for Wolfenstein The Old Blood. This game use extension ARB_buffer_storage improperly. It behaves like MAP_COHERENT_BIT flag is active, but only MAP_PERSISTENT_BIT exist in flags. This WA cause GITS to treat buffer mapping as COHERENT also in case of only MAP_PERSISTENT_BIT is active (by default all buffers mapped with MAP_COHERENT_BIT is updated in each frame by GITS).

- `MTDriverWA` - Windows only; Some functions like wglDescribePixelFormat, wglGetPixelFormat, wglSetPixelFormat is used in threaded drivers off the 'main' thread and causes deadlock due to GITS api mutex. This WA cause skipping record of these functions when deadlock occurs. In recorder log in this situation you can see warning: "Deadlock detected in ...". Deadlock detection is not perfect and theoretically may cause an unexpected behaviour. Alternative to this WA is disabling multithreading in driver.

- `CloseAppOnStopRecording` - Forces application, which is being recorded, to be closed immediately after the recording process is finished.

- `WindowsKeyHandling` - Select keyboard handling method on Windows:
    - MessageLoop - key presses are detected as events returned by GetMessage WinAPI function
    - AsyncKeyState - key presses are checked with GetAsyncKeyState WinAPI function

### Performance

- `Benchmark` - Causes GITS to produce csv file with system clock times of each frame.