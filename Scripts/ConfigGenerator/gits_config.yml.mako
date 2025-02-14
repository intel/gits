## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2025 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
---
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#          Copyright (C) 2023-2025 Intel Corporation           #
#              SPDX-License-Identifier: MIT               #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                    GITS CONFIG                          #
#           Settings description available in             #
#      Documentation/Recorder/gits_config_options.md      #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                   COMMON SETTINGS                       #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

Common:
  Shared:
    LogLevel: 'INFO'  # Possible values: TRACEVERBOSE, TRACE, INFO, WARNING, ERROR, OFF
    LogToConsole: false
<%
  if platform == "lnx_32":
    arch = 'i386'
  elif platform == "lnx_64":
    arch = 'x86_64'
  elif platform == "lnx_arm":
    arch = 'aarch64'
%>
  Recorder:
    RecordingMode: 'Binary'  # Possible values: None / Binary / CCode
    ExitKeys: []  # You may need to hold the key down until the next frame to make sure it works
    ExitAfterAPICall: 0
%if platform in ["lnx_32", "lnx_64", "lnx_arm"]:
    ExitSignal: 12  # To stop recording type kill -12 <pid> in terminal
%endif
    TokenBurstLimit: 10000
    TokenBurstNum: 5
%if platform == "win32":
    LibGL: 'C:\Windows\System32\OpenGL32.dll'
    LibEGL: 'C:\Windows\System32\libEGL.dll'
    LibGLES1: 'C:\Windows\System32\libGLESv1_CM.dll'
    LibGLES2: 'C:\Windows\System32\libGLESv2.dll'
    LibCL: 'C:\Windows\System32\OpenCL.dll'
    LibVK: 'C:\Windows\System32\vulkan-1.dll'
    LibOcloc: 'C:\Windows\System32\ocloc64.dll'
    LibL0: 'C:\Windows\System32\ze_loader.dll'
    InstallationPath: '${install_path}\Recorder'
    DumpDirectoryPath: '${install_path}\dump\%n%_%p%'
%else:
    LibGL: '/usr/lib/${arch}-linux-gnu/libGL.so.1'
    LibEGL: '/usr/lib/${arch}-linux-gnu/libEGL.so.1'
    LibGLES1: '/usr/lib/${arch}-linux-gnu/libGLESv1_CM.so.1'
    LibGLES2: '/usr/lib/${arch}-linux-gnu/libGLESv2.so.2'
  %if platform == "lnx_32":
    LibCL: '/usr/lib/${arch}-linux-gnu/libOpenCL.so'
  %else:
    LibCL: '/usr/lib/${arch}-linux-gnu/libOpenCL.so.1'
  %endif
    LibVK: '/usr/lib/${arch}-linux-gnu/libvulkan.so.1'
    LibOcloc: '/usr/lib/${arch}-linux-gnu/libocloc.so'
    LibL0: '/usr/lib/${arch}-linux-gnu/libze_loader.so.1'
    InstallationPath: '${install_path}/Recorder'
    DumpDirectoryPath: '${install_path}/dump/stream-%p%'
%endif
    UniqueDumpDirectory: true
    EventScript: ''  # Path
    ScriptArgs: ''
    Compression:
    %if is_compute:
      Type: 'ZSTD'  # None / LZ4 / ZSTD
      Level: 5  # 1-10: 1 - fastest, 10 - slowest, stream size better optimized
    %else:
      Type: 'LZ4'  # None / LZ4 / ZSTD
      Level: 10  # 1-10: 1 - fastest, 10 - slowest, stream size better optimized
    %endif
      ChunkSize: 2097152  # grouping small updates in chunks, default chunk size - 2MB
    ExtendedDiagnostic: true
    ForceDumpOnError: true
    ZipTextFiles: true
    HighIntegrity: false
    NullIO: false
    RemoveDXSharing: false
    RemoveGLSharing: false
    Benchmark: true
%if platform == "win32":
    CloseAppOnStopRecording: true
    WindowsKeyHandling: 'MessageLoop'
%endif

  Player:
%if platform == "win32":
    LibGL: 'OpenGL32.dll'
    LibEGL: 'libEGL.dll'
    LibGLES1: 'libGLESv1_CM.dll'
    LibGLES2: 'libGLESv2.dll'
    LibCL: 'OpenCL.dll'
    LibVK: 'vulkan-1.dll'
    LibOcloc: 'ocloc64.dll'
    LibL0: 'ze_loader.dll'
%else:
    LibGL: 'libGL.so.1'
    LibEGL: 'libEGL.so.1'
    LibGLES1: 'libGLESv1_CM.so.1'
    LibGLES2: 'libGLESv2.so.2'
  %if platform == "lnx_32":
    LibCL: 'libOpenCL.so'
  %else:
    LibCL: 'libOpenCL.so.1'
  %endif
    LibVK: 'libvulkan.so.1'
    LibOcloc: 'libocloc.so'
    LibL0: 'libze_loader.so.1'
%endif
    StreamPath: ''
%if platform == "win32":
    SubcapturePath: '${install_path}\dump\%f%_%r%'
%endif
    TokenBurstLimit: 10000
    TokenBurstNum: 5
    ExitFrame: 1000000
    EndFrameSleep: 0
    ExitOnError: false
    EventScript: ''  # Path
    ScriptArgs: ''
    Benchmark: false
    ShowWindowBorder: false
    DontVerifyStream: false
    CaptureFrames: '-'
    OutputDir: ''  # path
    TraceSelectedFrames: '-'
    Interactive: false
    OutputTracePath: ''
    LogFncs: false
    FaithfulThreading: false
    LoadWholeStreamBeforePlayback: false
    SignStream: false
    VerifyStream: false
    ShowWindowsWA: false
    DisableExceptionHandling: false
    CaptureScreenshot: false
    LogLoadedTokens: false
    EscalatePriority: false
    SwapAfterPrepare: false
    StopAfterFrames: '-'
    NullRun: false
    WaitForEnter: false
    CleanResourcesOnExit: false
    RenderOffscreen: false
    ForceOrigScreenResolution: false
    ForceInvisibleWindows: false
    Fullscreen: false
    ForceWindowPos:
      Enabled: false
      x: 0
      y: 0 
    ForceWindowSize:
      Enabled: false
      Width: 0
      Height: 0
    ForceScissor:
      Enabled: false
      x: 0
      y: 0
      Width: 0
      Height: 0
%if platform == "win32":
    ForceDesktopResolution:
      Enabled: false
      Width: 0
      Height: 0
%endif

%if platform == "win32":
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                     DirectX SETTINGS                    #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

DirectX:
  Capture:
    Record: true  # Record the API commands
    ShadowMemory: false
    CaptureIntelExtensions: true  # If false Intel Extensions capture is done at D3D12 level
    CaptureXess: true  # If false XeSS capture is done on Intel Extensions level
    CaptureDirectML: true  # If false DirectML capture is done on D3D12 level
    CaptureDirectStorage: false  # If false DirectStorage capture is done on D3D12 level
    DebugLayer: false  # Enable the DirectX Debug Layer
    Plugins: []
    TokenBurstChunkSize: 5242880  # default size: 5 MB

  Playback:
    Execute: true  # Execute the API commands (driver / null driver)
    DebugLayer: false  # Enable the DirectX Debug Layer
    WaitOnEventCompletion: false  # CPU waits for GPU on SetEventOnCompletion and SetEventOnMultipleFenceCompletion
    UseCopyQueueOnRestore: false  # Use a D3D12_COMMAND_LIST_TYPE_COPY to restore resource states
    UavBarrierAfterCopyRaytracingASWorkaround: false  # Adds UAV barrier for all resources after CopyRaytracingAccelerationStructure
    MultithreadedShaderCompilation: true  # Usa a thread pool for D3D12 methods which cause hardware shader compilation
    Plugins: []
    TokenBurstChunkSize: 5242880 # default size: 5 MB
    AdapterOverride:
      # Select the adapter used on D3D12CreateDevice
      # Set the Vendor and Index for that vendor (or global index if Vendor is empty)
      # For example: Index: 1, Vendor: "Intel" will select the second Intel adapter enumerated 
      Enabled: false
      Index: 0
      Vendor: ''  # Empty / AMD / NVIDIA / Intel

  Features:
    Trace:
      Enabled: false
      FlushMethod: 'ipc'  # off / ipc (flush into shared memory and save to file from a different process) / file (flush directly into a file)
      Print:
        PostCalls: true
        PreCalls: false
        DebugLayerWarnings: false
        GPUExecution: false

    Subcapture:
      Enabled: false
      SerializeAccelerationStructures: false # if true BLASes are recreated with serialize/deserialize mode of CopyRaytracingAccelerationStructure
      RestoreTLASes: false # if false TLASes are not recreated
      Frames: ''  # Frame range. For example '3-6'

    Screenshots:
      Enabled: false
      Frames: ''  # Frame range (supports multiple ranges and strides). For example '3-6:2,9'
      Format: 'png'  # png / jpg

    ResourcesDump:
      Enabled: false
      ResourceKeys: ''  # Comma separated list of resource keys
      CommandKeys: ''  # Comma separated list of command lists call keys
      TextureRescaleRange: ''  # Texture rescaling factor range between 0.0-1.0, if empty no rescaling. For example '0-0.5'
      Format: 'png'  # png / jpg

    RenderTargetsDump:
      Enabled: false
      Frames: ''  # Frame range. For example '3-6'
      Draws: ''  # Draw range (supports multiple ranges and strides). For example '1-100:5'
      Format: 'png'  # png / jpg

    RaytracingDump:
      BindingTablesPre: false  # Dumps binding table buffers before patching for given DispatchRays calls
      BindingTablesPost: false  # Dumps binding table buffers after patching for given DispatchRays calls
      InstancesPre: false  # Dumps instances buffers before patching for given BuildRaytracingAccelerationStructure TLAS calls
      InstancesPost: false  # Dumps instances buffers after patching for given BuildRaytracingAccelerationStructure TLAS calls
      BLASes: false  # Dumps bottom level acceleration structures for given BuildRaytracingAccelerationStructure BLAS calls
      CommandKeys: ''  # Comma separated list of DispatchRays or BuildRaytracingAccelerationStructure call keys, empty list means all such calls

    ExecuteIndirectDump:
      ArgumentBufferPre: false  # Dumps arguments buffers for specified ExecuteIndirect calls before patching
      ArgumentBufferPost: false  # Dumps arguments buffers for specified ExecuteIndirect calls after patching
      CommandKeys: ''  # Comma separated list of ExecuteIndirect call keys

    SkipCalls:
      Enabled: false
      CommandKeys: ''  # Comma separated list of call keys to skip

    Portability:
      Enabled: false
      StorePlacedResourceDataOnCapture : true  # Enable storing placed resources data while capturing
      StorePlacedResourceDataOnPlayback : false  # Request storing placed resources data while playing back

%endif

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                    OPENGL SETTINGS                      #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

OpenGL:
  Shared:
    TraceGLError: false
    ForceGLVersion: ''  # For example '4.3' or '3.1'.

  Recorder:
    Mode: 'All'  # All / Frames / OglSingleDraw / OglDrawsRange
    All:
      ExitFrame: 1000000
      ExitDeleteContext: 0
    Frames:
      StartFrame: 1
      StopFrame: 1000000
      StartKeys: []
      FrameSeparators:
        glFinish: false
        glFlush: false
    OglSingleDraw:
      Number: 1
    OglDrawsRange:
      StartDraw: 1
      StopDraw: 100000
      Frame: 0
    DumpScreenshots: '-'
    DumpDrawsFromFrames: '-'
    SuppressExtensions: ['GL_ARB_get_program_binary']
    SuppressProgramBinary: true
    EndFrameSleep: 0
    RestoreDefaultFB: true
    DoNotRemoveWindow: false
    MultiApiProtectBypass: false
    CArrayMemCmpType: 1
    StripIndicesValues: 0xFFFFFFFF
    OptimizeBufferSize: true
    RetryFunctionLoads: true
    DetectRecursion: true
    BuffersState: 'Mixed'
    TexturesState: 'Mixed'
    CoherentMapUpdatePerFrame: true
    BufferMapAccessMask: 0xFFFFFFF3
    BufferStorageFlagsMask: 1
    CoherentMapBehaviorWA: false
%if platform in ["win32"]:
    ScheduleFboEXTAsCoreWA: false
    UseGlGetTexImageAndRestoreBuffersWhenPossibleES: true
    TrackTextureBindingWA: false
    ForceBuffersStateCaptureAlwaysWA: false
    RestoreIndexedTexturesWA: false
    MTDriverWA: true
%endif
    CCodeRangesWA: false

  Player:
    ForceGLProfile: 'NONE'  # NONE / COMPAT / CORE / ES
    ForceGLNativeAPI: 'NONE'  # NONE / EGL / WGL / GLX
    SkipQueries: false
    ScaleFactor: 1.0
    CaptureFramesHashes: false
    DontForceBackBufferGL: false
    CaptureWholeWindow: false
    Capture2DTexs: '-'
    CaptureDraws2DTexs: '-'
    CaptureDraws: '-'
    CaptureDrawsPre: false
    CaptureFinishFrame: '-'
    CaptureReadPixels: '-'
    CaptureFlushFrame: '-'
    CaptureBindFboFrame: '-'
    KeepDraws: 'all'
    KeepFrames: 'all'
    MinimalConfig: false
    TraceGitsInternal: false
    LinkGetProgBinary: false
    LinkUseProgBinary: false
    AffectedViewport: []
    TraceGLBufferHashes: '-'
    ForceNoMSAA: false
    DestroyContextsOnExit: false
%if platform != "win32":
    ForceWaylandWindow: false
%endif


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                    VULKAN SETTINGS                      #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

Vulkan:
  Shared:
    SuppressPhysicalDeviceFeatures: []
    SuppressExtensions: ['VK_EXT_debug_marker', 'VK_EXT_shader_module_identifier']
    SuppressLayers: []

  Recorder:
    Mode: 'All'  # All / Frames / QueueSubmit / CommandBuffersRange / RenderPassRange / DrawsRange / DispatchRange / BlitRange
    All:
      ExitFrame: 1000000
    Frames:
      StartFrame: 1
      StopFrame: 1000000
      StartKeys: []
    QueueSubmit:
      Number: 1  # QueueSubmitNumber
    CommandBuffersRange:
      Range: '1/0/0'  # QueueSubmitNumber / CommandBufferBatchNumber / CommandBuffersRange
    RenderPassRange:
      Range: '1/0/0/0'  # QueueSubmitNumber / CommandBufferBatchNumber / CommandBufferNumber / RenderPassRange
    DrawsRange:
      Range: '1/0/0/0/0'  # QueueSubmitNumber / CommandBufferBatchNumber / CommandBufferNumber / RenderPassNumber / DrawsRange
    DispatchRange:
      Range: '1/0/0/0'  # QueueSubmitNumber / CommandBufferBatchNumber / CommandBufferNumber / DispatchRange
    BlitRange:
      Range: '1/0/0/0'  # QueueSubmitNumber / CommandBufferBatchNumber / CommandBufferNumber / BlitRange
    DumpScreenshots: '-'
    DumpSubmits: '-'
    TraceVKStructs: true
    MemorySegmentSize: 512
%if platform == "win32":
    MemoryTrackingMode: 'External' # External / ShadowMemory / WriteWatch / FullMemoryDump
    MemoryUpdateState: 'OnlyUsed'
%elif platform in ["lnx_32", "lnx_64", "lnx_arm"]:
    MemoryTrackingMode: 'FullMemoryDump' # ShadowMemory / FullMemoryDump
    MemoryUpdateState: 'AllMapped'
%endif
    ForceUniversalRecording: false
    DelayFenceChecksCount: 0
    ShortenFenceWaitTime: 0
    AddImageUsageFlags: 0x2  # VK_IMAGE_USAGE_TRANSFER_DST_BIT
    AddBufferUsageFlags: 0x1
    ScheduleCommandBuffersBeforeQueueSubmitWA: false
    MinimalStateRestore: true
    ReusableStateRestoreResourcesCount: 3  # must be at least 2
    ReusableStateRestoreBufferSize: 80  # in megabytes, must be greater than 0
    IncreaseImageMemorySizeRequirement:
      FixedAmount: 0  # in bytes
      Percent: 0
    MemoryOffsetAlignmentOverride:
      Images: 0  # in bytes
      Buffers: 0  # in bytes
      Descriptors: 0  # in bytes
    CrossPlatformStateRestoration:
      Images: true
      Buffers: 'WithNonHostVisibleMemoryOnly'  # None / WithNonHostVisibleMemoryOnly / All
    MemoryRestoration: 'HostVisible'  # None / HostVisible
    RestoreMultisampleImagesWA: false
    MaxArraySizeForCCode: 400
    UseCaptureReplayFeaturesForBuffersAndAccelerationStructures: true
    UseCaptureReplayFeaturesForRayTracingPipelines: true
%if platform == "win32":
    UsePresentSrcLayoutTransitionAsAFrameBoundary: false  # Offscreen applications workaround
    RenderDocCompatibility: true  # supress extensions not supported by RenderDoc: VK_EXT_graphics_pipeline_library, VK_EXT_extended_dynamic_state3, VK_EXT_external_memory_host, VK_KHR_map_memory2, VK_EXT_dynamic_rendering_unused_attachments, VK_EXT_host_image_copy, VK_KHR_maintenance5
%endif

  Player:
    ExitOnVkQueueSubmitFail: false
    CaptureVulkanSubmits: '-'
    CaptureVulkanSubmitsResources: '-'
    CaptureVulkanSubmitsGroupType: 'CmdBuffer'  # CmdBuffer / RenderPass
    CaptureVulkanRenderPasses: ''
    CaptureVulkanRenderPassesResources: ''
    CaptureVulkanDraws: ''
    CaptureVulkanResources: ''
    SkipNonDeterministicImages: false
    IgnoreVKCrossPlatformIncompatibilitiesWA: false
    WaitAfterQueueSubmitWA: false
    TraceVKShaderHashes: false
    MaxAllowedVkSwapchainRewinds: 100
    OverrideVKPipelineCache: ''  # path
    OneVulkanDrawPerCommandBuffer: false
    OneVulkanRenderPassPerCommandBuffer: false
    ForcedPhysicalDeviceIndex: 0
    ForcedPhysicalDeviceName: ''
    ForcedPhysicalDeviceType: 'any'  # any / integrated / discrete
    PrintStateRestoreLogsVk: false
    PrintMemUsageVk: false
    ForceMultithreadedPipelineCompilation: false
    ExecCmdBuffsBeforeQueueSubmit: false
    RenderDoc:
      Mode: 'None'  # None / Frames / QueueSubmit
      Range: '-'
      ContinuousCapture: false
      EnableUI: false


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                    OPENCL SETTINGS                      #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

OpenCL:
  Recorder:
    Mode: 'All'  # All / OclSingleKernel / OclKernelsRange
    OclSingleKernel:
      Number: 1
    OclKernelsRange:
      StartKernel: 1
      StopKernel: 10
    DumpKernels: '-'
    DumpImages: false
    OmitReadOnlyObjects: false
    BufferResetAfterCreate: false
    NullIndirectPointersInBuffer: true

  Player:
    CaptureImages: false
    RemoveSourceLengths: false
    CaptureReads: false
    CaptureKernels: '-'
    OmitReadOnlyObjects: false
    DumpLayoutOnly: false
    InjectBufferResetAfterCreate: false
    DisableNullIndirectPointersInBuffer: false
    NoOpenCL: false
    AubSignaturesCL: false


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                  LEVELZERO SETTINGS                     #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

LevelZero:
  Recorder:
    Mode: 'All'  # All / Kernel
    Kernel:
      Range: '1/1/1'
    DumpKernels: '-/-/-'
    DumpAfterSubmit: false
    DumpImages: false
    DumpInputKernels: false
    BufferResetAfterCreate: false
    NullIndirectPointersInBuffer: true
    BruteForceScanForIndirectPointers:
      MemoryType: 0  # -1 (All), 1 (Host), 2 (Device), 4 (Shared)
      Iterations: 0
    DisableAddressTranslation:
      MemoryType: 0  # -1 (All) , 1 (Host), 2 (Device), 4 (Shared)
      VirtualDeviceMemorySize: 1099511627776
      VirtualHostMemorySize: 1099511627776
    DumpLayoutOnly: false

  Player:
    CaptureImages: false
    DumpSpv: false
    CaptureKernels: '-/-/-'
    DumpLayoutOnly: false
    CaptureAfterSubmit: false
    CaptureInputKernels: false
    InjectBufferResetAfterCreate: false
    DisableNullIndirectPointersInBuffer: false
    DisableAddressTranslation: 0
    OmitOriginalAddressCheck: false

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                       OVERRIDES                         #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Override gits_config.yml with settings based on the executable name
# GITS will override the base configuration with the YAML under each entry

# Example entry (disable recording for MyApplication.exe):
#   MyApplication:
#     Common:
#       Recorder:
#         RecordingMode: 'None'

Overrides:
