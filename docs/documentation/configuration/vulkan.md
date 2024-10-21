---
icon: simple/vulkan
---
# Vulkan configuration options

## Capture

- `Mode`  
	Specifies mode of operation and triggering behaviour of **GITS** recorder. Valid modes are All, Frames, QueueSubmit, CommandBuffersRange, RenderPassRange, DrawsRange, DispatchRange, BlitRange. This also selects option group in Vulkan.Capture that is active.

## Capture.All

- `ExitFrame`  
	After this frame, recorder will stop operation and write the stream to filesystem.

## Capture.Frames

- `StartFrame`  
	First frame to be recorder by GITS recorder.

- `StopFrame`  
	The frame number when recording ends (it will be the last frame recorded). If '0' capture will be ended together with tested application.

- `StartKeys`  
	Key combination beginning capture. When given, capture start/stop frames are relative to frame in which keypress occurred. If empty, capture start/stop are relative to the beginning of the application. Option available on: Windows (all alpha/special keys)

## Capture.QueueSubmit

- `Number`  
	Only this queue submit will be recorded. The format is "queue_submit_number".

## Capture.CommandBuffersRange

- `Range`  
	Only these command buffers will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffers_range"

## Capture.RenderPassRange

- `Range`  
	Only these render passes will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_range"

## Capture.DrawsRange

- `Range`  
	Only these draws will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_number/draws_range"

## Capture.DispatchRange

- `Range`  
	Only these dispatches will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/dispatch_range"

## Capture.BlitRange

- `Range`  
	Only these blits will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffer_number/blit_range"

## Utilities

- `TraceVKStructs`  
	Trace values of Vulkan structs instead of tracing pointers.

- `MemorySegmentSize`  
	If application is using memory mappings this option causes only changed portion of the memory to be dumped. Disabling this option causes entire memory to be dumped on every memory unmap api call or before `vkQueueSubmit`, when memory is mapped or used in some submitted CommandBuffer (depending on `Vulkan.Utilities.MemoryUpdateState` setting), but skips compare operations. This option increase RAM usage during recording and used standalone (without `Vulkan.Utilities.MemoryAccessDetection`) is very slow in some apps (e.g. Talos Principle). In contrast to `Vulkan.Utilities.MemoryAccessDetection` this option compare memory byte by byte instead of updating all touched pages (one page has 4096 KB, so this option can better optimize update size).

- `MemoryTrackingMode`
    - `External` :material-microsoft-windows:  
      When this option is enabled, GITS replaces memory allocations provided by a driver with its own, host memory allocations by using VK_EXT_external_memory_host extension. This is done only on memory objects which can be mapped so, after mapping, applications write directly to the GITS recorder's own memory, which makes memory updates tracking simpler. This way, GITS does not have to allocate additional memory (so memory usage shouldn't increase too much) and at the same time it is still possible to mark those memory areas as READ_ONLY. Enabling this option may impact rendering perfomance, though, but the influence varies depending on each, specific application.
    - `ShadowMemory`  
      When this option is enabled, GITS creates its own copy (a shadow memory) of each mapped memory object and passes replaced pointers to recorded applications so they write to the provided memory. If an application is using memory mappings, this option causes GITS to dump only memory pages that were written into. Memory changes are tracked by setting a READ_ONLY flag on mapped memory objects in conjunction with special exception handling routines, allowing GITS to track which pages are used for writing. This option increases RAM usage, so if a platform doesn't have a lot of memory then shadow memory should not be used together with `Vulkan.Utilities.MemorySegmentSize`. (Exact value depends on the app's memory usage, but if it uses 2GB of RAM, it is recommended to have at least 6GB of RAM to use these two options together).
    - `WriteWatch` :material-microsoft-windows:  
      When this option is enabled, GITS creates its own copy (a shadow memory) of each mapped memory object and passes replaced pointers to recorded applications so they write to the provided memory. If an application is using memory mappings, this option causes GITS to dump only memory pages that were written into. Memory changes are tracked using the WriteWatch mechanism, which allows GITS to efficiently monitor which pages are written. This option increases RAM usage, so if a platform doesn't have a lot of memory then shadow memory should not be used together with `Vulkan.Utilities.MemorySegmentSize`. (Exact value depends on the app's memory usage, but if it uses 2GB of RAM, it is recommended to have at least 6GB of RAM to use these two options together).
    - `FullMemoryDump`  
      When this option is enabled, GITS dumps the entire memory content before `vkQueueSubmit`. If used together with `Vulkan.Utilities.MemorySegmentSize`, GITS performs a binary comparison byte by byte to reduce the memory content that needs to be dumped. This option ensures that all memory changes are captured, but it can significantly increase the amount of data being recorded and is very slow due to the binary comparison process. It is recommended to use this option only when other options did not work.

- `ForceUniversalRecording`  
	GITS recorder checks if it's attached to a GITS Player to use internal methods for detecting memory changes and improving recording performance. Setting this option to true causes recorder to disable these internal mechanisms and act as if it is recording any other application.

- `MemoryUpdateState`  
	If application is using memory mappings in coherent mode we need to synchronize memory before `vkQueueSubmit`. There is two available modes:  

	|     |     |
	| ----------------- | --- |
	| `AllMapped` | Update all mapped memory at this moment|
	| `OnlyUsed` | Update only mapped memory used in submitting CommandBuffers, so we track information about used buffers and images.|

- `DelayFenceChecksCount`  
	This value postpones fence's signaled state to be returned to application; application has to check fence's status or wait for a given fence provided  number of times before the signaled state is returned.

- `ShortenFenceWaitTime`  
	Number of nanoseconds to shorten the time application waits on fences; this value is subtracted from the timeout value provided to vkWaitForFences() function.

- `SuppressExtensions`  
	Array of extensions names to be removed from both Instance and/or Device extenstions reported to recorded application by the driver.

- `SuppressLayers`  
	Array of layers names to be removed from both Instance and/or Device layers reported to recorded application by the driver.

- `AddImageUsageFlags`  
	Single value representing bit flags which should be added to all images at creation time.

- `AddBufferUsageFlags`  
	Single value representing bit flags which should be added to all buffers at creation time.

- `ScheduleCommandBuffersBeforeQueueSubmitWA`  
	writing commandBuffers operation to stream just before QueueSubmit, instead of doing it in the same way as app/game/benchmark.

- `MinimalStateRestore`  
	Restore only objects which are actually used in the substream. Option available only for modes QueueSubmit and CommandBuffersRange.

- `ReusableStateRestoreResourcesCount`  
	Number of separate resource sets which are concurrently used to restore contents of images and buffers. It cannot be smaller than 2.

- `ReusableStateRestoreBufferSize`  
	The size (in megabytes) of temporary buffers used to restore contents of images and buffers. The greater the size, the less copy operations will be performed (and the state restore phase in a recorded substream will be faster) but the memory consumption will be higher.

- `SuppressPhysicalDeviceFeatures`  
	Array of physical device features names to be removed from features reported by the driver. Physical device features are reported with vkGetPhysicalDeviceFeatures() and vkGetPhysicalDeviceFeatures2KHR() function in the VkPhysicalDeviceFeatures structure. Name of each member of this structure can be provided in the option. I.e. to suppress 'geometry shaders' and 'cubemap arrays' specify: SuppressPhysicalDeviceFeatures ["imageCubeArray", "geometryShader"]

- `MemoryRestoration`  
	Controls restoration of contents of memory objects. The following options are available:

	| | |
	|-|-|
	| `None` | contents of memory objects are not restored|
  | `HostVisible` | restores contents of all memory objects allocated from a host-visible memory|

- `RestoreMultisampleImagesWA`  
	Restoration of multisample images via vkCmdCopyImageToBuffer/vkCmdCopyBufferToImage is forbidden by spec, but it is supported by Intel's driver at the time of this writing. Enabling this option fixes corruptions in some titles, when stream recorded on Gen9 is played back on Gen12.

- `MaxArraySizeForCCode`  
	Number that defines the maximum size of the array that will be written to CCode in one scope. Applies to: vkCmdPipelineBarrier(), vkCmdCopyBufferToImage(), vkCmdCopyBuffer(), vkUpdateDescriptorSets().

## Utilities.IncreaseImageMemorySizeRequirement

- `FixedAmount`  
	Amount of bytes by which the size of image memory requirement reported to application should be increased.

- `Percent`  
	 Percentage by which the size of image memory requirement reported to application should be (relatively) increased.

## Utilities.MemoryOffsetAlignmentOverride

- `Images`  
	defines the memory offset alignment that is returned to the application through a vkGetImageMemoryRequirements() function call in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 4096. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

- `Buffers`  
	defines the memory offset alignment that is returned to the application through a vkGetBufferMemoryRequirements() function call in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

- `Descriptors`  
	defines the memory offset alignment that is returned to the application through a vkGetPhysicalDeviceProperties() function call in minTexelBufferOffsetAlignment, minUniformBufferOffsetAlignment and minStorageBufferOffsetAlignment members of a VkPhysicalDeviceProperties::VkPhysicalDeviceLimits structure. Selected value must be a multiple of the original alignment and must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.

## Utilities.CrossPlatformStateRestoration

- `Images`  
	Allows for proper image contents restoration; this is performed through appropriate image layout transitions and data transfer between temporary staging buffer and the image; this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Buffers SHOULD allow playing recorded substreams on various hardware platforms (though the compatibility isn't guaranteed) for the sake of bigger stream size and longer state restore time.

- `Buffers`  
	Allows for proper buffer contents restoration; this is performed through data transfer between temporary staging buffer and the target; this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Images SHOULD allow playing recorded substreams on various hardware platforms (though the compatibility isn't guaranteed) for the sake of bigger stream size and longer state restore time. The following options for buffer memory restoration are available:

	| | |
	|-|-|
	| `None` | don't restore buffers' memory contents |
  | `WithNonHostVisibleMemoryOnly` | restore contents of buffers with non-host-visible memory bound (buffers with device-local-only memory)|
  | `All` | restore contents of all buffers |

## Images

- `DumpScreenshots`  
	Rangespec with frames to be captured during the recording.

- `DumpSubmits`  
	Rangespec with QueueSubmits, from which all submits will be captured.
