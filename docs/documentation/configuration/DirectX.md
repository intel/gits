---
icon: material/microsoft
---

# DirectX configuration options

## DirectX.Capture

- **`Record`**
    Record the API commands
- **`ShadowMemory`**
    Use an extra copy of the resource to track memory updates (Map / Unmap).
- **`CaptureXess`**
    If false XeSS capture is done on Intel Extensions level
- **`CaptureDirectML`**
    If false DirectML capture is done on D3D12 level
- **`DebugLayer`**
    Enable the DirectX Debug Layer
- **`Plugins`**
    List of plugins to enable
- **`TokenBurstChunkSize`**
    Default size: 5 MB

## DirectX.Playback

- **`Execute`**
    Execute the API commands (driver / null driver)
- **`DebugLayer`**
    Enable the DirectX Debug Layer
- **`WaitOnEventCompletion`**
    CPU waits for GPU on SetEventOnCompletion and SetEventOnMultipleFenceCompletion
- **`UseCopyQueueOnRestore`**
    Use a D3D12_COMMAND_LIST_TYPE_COPY to restore resource states
- **`SkipResolveQueryData`**
    Don't replay ID3D12GraphicsCommandList::ResolveQueryData commands
- **`MultithreadedShaderCompilation`**
    Use a thread pool for D3D12 methods which cause hardware shader compilation
- **`Plugins`**
    List of plugins to enable
- **`TokenBurstChunkSize`**
    Default size: 5 MB
- **`AdapterOverride`**
  Select the adapter used on D3D12CreateDevice. Set the Vendor and Index for that vendor (or global index if Vendor is empty). For example: Index: 1, Vendor: "Intel" will select the second Intel adapter enumerated.
  - **`Enabled`**
      Whether adapter override is enabled or not
  - **`Index`**
      Index of the adapter to use
  - **`Vendor`**
      Vendor of the adapter (Empty / AMD / NVIDIA / Intel).

## DirectX.Features

### DirectX.Features.Trace

- **`Enabled`**
    Whether tracing is enabled or not
- **`FlushMethod`**
  - **`off`**
      Flush is turned off
  - **`ipc`**
      Flush the trace buffer into a shared memory and periodically save into a file from a different process
  - **`file`**
      Flush the trace buffer directly into a file
- **`Print`**
  - **`PostCalls`**
      Print information after API calls
  - **`PreCalls`**
      Print information before API calls
  - **`DebugLayerWarnings`**
      Print debug layer warnings
  - **`GPUExecution`**
      Print GPU execution information

### DirectX.Features.Subcapture

- **`Enabled`**
    Whether subcapture is enabled or not
- **`Frames`**
    Frame range. For example '3-6'

### DirectX.Features.Screenshots

- **`Enabled`**
    Whether screenshots are enabled or not
- **`Frames`**
    Frame range (supports multiple ranges and strides). For example '3-6:2,9'
- **`Format`**
    Format of the screenshots (png / jpg)

### DirectX.Features.ResourcesDump

- **`Enabled`**
    Whether resource dumping is enabled or not
- **`ResourceKeys`**
    Comma separated list of resource keys
- **`CommandKeys`**
    Comma separated list of command lists call keys
- **`TextureRescaleRange`**
    Texture rescaling factor range between 0.0-1.0, if empty no rescaling. For example '0-0.5'
- **`Format`**
    Format of the resource dump (png / jpg)

### DirectX.Features.RenderTargetsDump

- **`Enabled`**
    Whether render targets dumping is enabled or not
- **`Frames`**
    Frame range. For example '3-6'
- **`Draws`**
    Draw range (supports multiple ranges and strides). For example '1-100:5'
- **`Format`**
    Format of the render targets dump (png / jpg)

### DirectX.Features.RaytracingDump

- **`BindingTablesPre`**
    Dumps binding table buffers before patching
- **`BindingTablesPost`**
    Dumps binding table buffers after patching
- **`InstancesPre`**
    Dumps instances buffers before patching
- **`InstancesPost`**
    Dumps instances buffers after patching
- **`CommandKeys`**
    Comma separated list of DispatchRays or BuildRaytracingAccelerationStructure call keys, empty list means all such calls

### DirectX.Features.ExecuteIndirectDump

- **`ArgumentBufferPre`**
    Dumps arguments buffers for specified ExecuteIndirect calls before patching
- **`ArgumentBufferPost`**
    Dumps arguments buffers for specified ExecuteIndirect calls after patching
- **`CommandKeys`**
    Comma separated list of ExecuteIndirect call keys

### DirectX.Features.SkipCalls

- **`Enabled`**
    Whether skipping calls is enabled or not
- **`CommandKeys`**
    Comma separated list of call keys to skip