## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Copyright (C) 2023 Intel Corporation
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; GITS CONFIG
;
; Settings description in the latter part of the file.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; BASIC SETTINGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Basic {
  RecordingEnabled      True
  LogLevel              "INFO" ;Possible values: TRACEVERBOSE, TRACE, INFO, WARNING, ERROR, OFF

  BinaryDump            True
  CCodeDump             False

  ExitKeys              "" ;You may need to hold the key down until the next frame to make sure it works
  ExitAfterAPICall      0
  ##
  %if platform in ["lnx_32", "lnx_64", "lnx_arm"]:
  ExitSignal            12 ;To stop recording type kill -12 <pid> in terminal
  %endif
  ##

  Paths {
    %if platform == "win32":
    LibGL                  "C:\\Windows\\System32\\OpenGL32.dll"
    LibEGL                 "C:\\Windows\\System32\\libEGL.dll"
    LibGLES1               "C:\\Windows\\System32\\libGLESv1_CM.dll"
    LibGLES2               "C:\\Windows\\System32\\libGLESv2.dll"
    LibCL                  "C:\\Windows\\System32\\OpenCL.dll"
    LibVK                  "C:\\Windows\\System32\\vulkan-1.dll"
    LibOcloc               "C:\\Windows\\System32\\ocloc.dll"
    LibL0                  "C:\\Windows\\System32\\ze_loader.dll"

    InstallationPath       "${install_path}\\Recorder"
    DumpDirectoryPath      "${install_path}\\dump\\%n%_%p%"
    UniqueDumpDirectory    True
    ##
    %elif platform == "lnx_32":
    LibGL                  "/usr/lib/i386-linux-gnu/libGL.so.1"
    LibEGL                 "/usr/lib/i386-linux-gnu/libEGL.so.1"
    LibGLES1               "/usr/lib/i386-linux-gnu/libGLESv1_CM.so.1"
    LibGLES2               "/usr/lib/i386-linux-gnu/libGLESv2.so.2"
    LibCL                  "/usr/lib/i386-linux-gnu/libOpenCL.so"
    LibVK                  "/usr/lib/i386-linux-gnu/libvulkan.so.1"
    LibOcloc               "/usr/lib/i386-linux-gnu/libocloc.so"
    LibL0                  "/usr/lib/i386-linux-gnu/libze_loader.so.1"

    InstallationPath       "${install_path}/Recorder"
    DumpDirectoryPath      "${install_path}/dump/stream-%p%"
    UniqueDumpDirectory    True
    ##
    %elif platform == "lnx_64":
    LibGL                  "/usr/lib/x86_64-linux-gnu/libGL.so.1"
    LibEGL                 "/usr/lib/x86_64-linux-gnu/libEGL.so.1"
    LibGLES1               "/usr/lib/x86_64-linux-gnu/libGLESv1_CM.so.1"
    LibGLES2               "/usr/lib/x86_64-linux-gnu/libGLESv2.so.2"
    LibCL                  "/usr/lib/x86_64-linux-gnu/libOpenCL.so.1"
    LibVK                  "/usr/lib/x86_64-linux-gnu/libvulkan.so.1"
    LibOcloc               "/usr/lib/x86_64-linux-gnu/libocloc.so"
    LibL0                  "/usr/lib/x86_64-linux-gnu/libze_loader.so.1"

    InstallationPath       "${install_path}/Recorder"
    DumpDirectoryPath      "${install_path}/dump/stream-%p%"
    UniqueDumpDirectory    True
    ##
    %elif platform == "lnx_arm":
    LibGL                  "/usr/lib/aarch64-linux-gnu/libGL.so.1"
    LibEGL                 "/usr/lib/aarch64-linux-gnu/libEGL.so.1"
    LibGLES1               "/usr/lib/aarch64-linux-gnu/libGLESv1_CM.so.1"
    LibGLES2               "/usr/lib/aarch64-linux-gnu/libGLESv2.so.2"
    LibCL                  "/usr/lib/aarch64-linux-gnu/libOpenCL.so.1"
    LibVK                  "/usr/lib/aarch64-linux-gnu/libvulkan.so.1"
    LibOcloc               "/usr/lib/aarch64-linux-gnu/libocloc.so"
    LibL0                  "/usr/lib/aarch64-linux-gnu/libze_loader.so.1"

    InstallationPath       "${install_path}/Recorder"
    DumpDirectoryPath      "${install_path}/dump/stream-%p%"
    UniqueDumpDirectory    True
    ##
    %else:
    InstallationPath       "${install_path}/Recorder"
    DumpDirectoryPath      "${install_path}/dump/%p%"
    %endif
  }
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OPENGL SETTINGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

OpenGL {
  Capture {
    Mode                  All               ; All / Frames / OglSingleDraw / OglDrawsRange

    All {
      ExitFrame           1000000
      ExitDeleteContext   0
    }

    Frames {
      StartFrame          1
      StopFrame           1000000
      StartKeys           ""
      FrameSeparators {
        glFinish          False
        glFlush           False
      }
    }

    OglSingleDraw {
      Number              1
    }

    OglDrawsRange {
      StartDraw           1
      StopDraw            100000
      Frame               0
    }
  }

  Utilities {
    TraceGLError          False
    ForceGLVersion        "" ; For example "4.3" or "3.1".
    SuppressExtensions    "GL_ARB_get_program_binary"
    SuppressProgramBinary True
    EndFrameSleep         0
    RestoreDefaultFB      True
    DoNotRemoveWindow     False
    MultiApiProtectBypass False
    CArrayMemCmpType      1
    StripIndicesValues    0xFFFFFFFF
    OptimizeBufferSize    True
    RetryFunctionLoads    True
    ##
    %if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
    DetectRecursion       True
    BuffersState          Mixed
    TexturesState         Mixed
    %endif
    ##
    %if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
    CoherentMapUpdatePerFrame True
    %endif
    ##
    %if platform in ["win32"]:
    ScheduleFboEXTAsCoreWA False
    %endif
    ##
    %if platform in ["win32"]:
    UseGlGetTexImageAndRestoreBuffersWhenPossibleES True
    %endif
    ##
    %if platform == "win32":
    TrackTextureBindingWA             False
    ForceBuffersStateCaptureAlwaysWA  False
    RestoreIndexedTexturesWA          False
    %endif
    ##
  }

  Performance {
    Benchmark             True
  }

  Images {
    DumpScreenshots       -
    DumpDrawsFromFrames   -
  }
}
##
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OPENCL SETTINGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

OpenCL {
  Capture {
    Mode                  All          ; All / OclSingleKernel / OclKernelsRange

    OclSingleKernel {
      Number              1
    }

    OclKernelsRange {
      StartKernel         1
      StopKernel          10
    }
  }

  Utilities {
    DumpKernels                   -
    DumpImages                    False
    OmitReadOnlyObjects           False
    BufferResetAfterCreate        False
    NullIndirectPointersInBuffer  True
  }
}
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; VULKAN SETTINGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Vulkan {
  Capture {
    Mode                  All              ; All / Frames / QueueSubmit / CommandBuffersRange

    All {
      ExitFrame           1000000
    }

    Frames {
      StartFrame          1
      StopFrame           1000000
      StartKeys           ""
    }

    QueueSubmit {
      Number              1                ; QueueSubmitNumber
    }

    CommandBuffersRange {
      Range               1/0/0            ; QueueSubmitNumber/CommandBufferBatchNumber/CommandBuffersRange
    }
  }

  Utilities {
    TraceVKStructs                      True
    MemorySegmentSize                   512
    ##
    %if platform == "win32":
    ShadowMemory                        True
    MemoryAccessDetection               True
    %elif platform in ["lnx_32", "lnx_64", "lnx_arm"]:
    ShadowMemory                        False
    MemoryAccessDetection               False
    %endif
    ##
    ForceUniversalRecording             False
    ##
    %if platform == "win32":
    UseExternalMemoryExtension          False
    MemoryUpdateState                   OnlyUsed
    %elif platform in ["lnx_32", "lnx_64", "lnx_arm"]:
    MemoryUpdateState                   AllMapped
    %endif
    ##
    DelayFenceChecksCount               0
    ShortenFenceWaitTime                0
    SuppressExtensions                  "VK_EXT_debug_marker,VK_EXT_shader_module_identifier,VK_EXT_graphics_pipeline_library"
    SuppressLayers                      ""
    AddImageUsageFlags                  0x0
    AddBufferUsageFlags                 0x1
    ScheduleCommandBuffersBeforeQueueSubmitWA    False
    MinimalStateRestore                 True
    ReusableStateRestoreResourcesCount  3              ; must be at least 2
    ReusableStateRestoreBufferSize      80             ; in megabytes, must be greater than 0
    IncreaseImageMemorySizeRequirement {
      FixedAmount 0                                    ; in bytes
      Percent     0
    }
    SuppressPhysicalDeviceFeatures      ""
    MemoryOffsetAlignmentOverride {
      Images      0                                    ; in bytes
      Buffers     0                                    ; in bytes
      Descriptors 0                                    ; in bytes
    }
    CrossPlatformStateRestoration {
      Images            True
      Buffers           WithNonHostVisibleMemoryOnly ; None / WithNonHostVisibleMemoryOnly / All
    }
    MemoryRestoration   HostVisible                  ; None / HostVisible
    RestoreMultisampleImagesWA   False
    MaxArraySizeForCCode         400
    UseCaptureReplayFeaturesForBuffersAndAccelerationStructures   False
    UseCaptureReplayFeaturesForRayTracingPipelines                True
    UsePresentSrcLayoutTransitionAsAFrameBoundary                 False     ; Dx9onVk workaround
  }

  Performance {
    Benchmark             True
  }

  Images {
    DumpScreenshots       -
    DumpSubmits           -
  }
}
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; LEVELZERO SETTINGS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LevelZero {
  Capture {
    Mode                  All                ; All / Kernel

    Kernel {
      Range               1/1/1
    }
  }

  Utilities {
    DumpKernels             -/-/-
    DumpAfterSubmit         False
    DumpImages              False
    BufferResetAfterCreate  False
    NullIndirectPointersInBuffer  True
  }
}
%endif
##

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; EXTRAS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Extras {
  Optimizations {
    TokenBurstLimit         1000000
    ##
    %if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
    HashType                XxCrc32
    %else:
    HashType                Crc32ish
    %endif
    ##
    AsyncBufferWrites       2000000
    HashPartially           False
    PartialHashCutoff       8192
    PartialHashChunks       10
    PartialHashRatio        20
    BufferMapAccessMask     0xFFFFFFF3
    BufferStorageFlagsMask  1
    RemoveResourceHash      False
  }

  Utilities {
    ExtendedDiagnostic      True
    ForceDumpOnError        True
    ZipTextFiles            True
    HighIntegrity           False
    NullIO                  False
    EventScript             ""
    RemoveAPISharing        ""
    ##
    %if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
    CoherentMapBehaviorWA   False
    %endif
    ##
    %if platform == "win32":
    MTDriverWA              True
    CloseAppOnStopRecording True
    WindowsKeyHandling      MessageLoop
    %endif
    ##
    %if platform == "and_arm":
    DlsymOffset             0
    %endif
    ##
  }
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Setting description
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Basic.RecordingEnabled          - Enables GITS recording. If set to False, GITS will be loaded to
;                                    the application but won't perform any recording. This is useful
;                                    for logging API calls without recording.
;
;  Basic.LogLevel                  - Only info with level greater or equal to this will be logged.
;                                    The levels are: TRACEVERBOSE, TRACE, INFO (default), WARNING, ERROR and OFF.
;                                    API calls are logged at levels TRACE and lower (lower means more verbose).
;
;  Basic.BinaryDump                - Enable/disable recording in the format supported by
;                                    GITS player.
;
;  Basic.CCodeDump                 - Enable/disable recording in C-code format.
;                                    Its possible to have both Basic.BinaryDump and Basic.CCodeDump
;                                    set to False but the resulting stream won't be very useful.
;
;  Basic.ExitKeys                  - If keys specified, after pressing key combination
;                                    GITS plugin will be unloaded from the process
;                                    Option available on:
;                                      - Windows (all alpha/special keys)
;
;  Basic.ExitAfterAPICall          - Enables forcing dumping of GITS artifacts after specific
;                                    number of API calls (0 - do not dump)
;
##
%if platform != "win32":
;  Basic.ExitSignal                - Signal used to unload GITS recorder. Default signal is SIGTERM(15).
;
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  Basic.Paths.LibGL               - Path to library containing actual implementation of OpenGL
;                                    API.
;
;  Basic.Paths.LibEGL              - Path to library containing actual implementation of EGL API.
;
;  Basic.Paths.LibGLES1            - Path to library containing actual implementation of OpenGL
;                                    ES 1 API.
;
;  Basic.Paths.LibGLES2            - Path to library containing actual implementation of OpenGL
;                                    ES 2 API.
;
;  Basic.Paths.LibCL               - Path to library containing actual implementation of OpenCL
;                                    API.
;
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  Basic.Paths.LibVK               - Path to library containing actual implementation of Vulkan
;                                    API.
;
;  Basic.Paths.LibOcloc            - Path to library containing actual implementation of Ocloc
;                                    API.
;
;  Basic.Paths.LibL0               - Path to library containing actual implementation of LevelZero
;                                    API.
;
%endif
##
;  Basic.Paths.InstallationPath    - Path to GITS Recorder directory (containing GITS recorder library).
;
##
%if platform == "win32":
;  Basic.Paths.DumpDirectoryPath   - Specified directory will contain newly created stream. %p%
;                                    will be substituted with process id of a process that created
;                                    the stream. %n% will be substituted with process name of a process
;                                    that created the stream.
;
%else:
;  Basic.Paths.DumpDirectoryPath   - Specified directory will contain newly created stream. %p%
;                                    will be substituted with process id of a process that created
;                                    the stream.
;
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  Basic.Paths.UniqueDumpDirectory - The dump directory name for each recorder execution
;                                    will be postfixed with unique timestamp.
;
%endif
##
;  OpenGL.Capture.Mode                                   - Specifies mode of operation and triggering behaviour of GITS
;                                                          recorder. Valid modes are All, Frames, OglSingleDraw,
;                                                          OglDrawsRange . This also selects option group in OpenGL.Capture
;                                                          that is active.
;
;  OpenGL.Capture.All.ExitFrame                          - After this frame, recorder will stop operation and write the
;                                                          stream to filesystem.
;
;  OpenGL.Capture.All.ExitDeleteContext                  - Number of context deletion GL function on which
;                                                          to stop recording. Zero means disable feature.
;
;  OpenGL.Capture.Frames.StartFrame                      - First frame to be recorder by GITS recorder.
;
;  OpenGL.Capture.Frames.StopFrame                       - The frame number when recording ends (it will be the
;                                                          last frame recorded). If '0' capture will be ended
;                                                          together with tested application.
;
;  OpenGL.Capture.Frames.StartKeys                       - Key combination beginning capture. If given, capture
;                                                          start and stop are relative to frame in which keypress
;                                                          occurred. If empty, capture start / stop are relative
;                                                          to the beginning of the application
;                                                          Option available on:
;                                                            - Windows (all alpha/special keys)
;
;  OpenGL.Capture.Frames.FrameSeparators.glFinish        - Specifies glFinish as an additional frame separator.
;                                                          It makes Start/StopFrame values to be counted
;                                                          as a total number of glFinish and SwapBuffers calls.
;
;  OpenGL.Capture.Frames.FrameSeparators.glFlush         - Specifies glFlush as an additional frame separator.
;                                                          It makes Start/StopFrame values to be counted
;                                                          as a total number of glFlush and SwapBuffers calls.
;
;  OpenGL.Capture.OglSingleDraw.Number                   - Number of drawcall that is to be recorded. For this draw state
;                                                          minimization will be performed.
;
;  OpenGL.Capture.OglDrawsRange.StartDraw                - Number of drawcall that will begin stream capture.
;
;  OpenGL.Capture.OglDrawsRange.StopDraw                 - Number of last draw to be included in the capture.
;
;  OpenGL.Capture.OglDrawsRange.Frame                    - When set to zero, StartDraw and StopDraw are relative
;                                                          to the beginning of the application and draws range may
;                                                          span many frames. Otherwise it's the number of the frame,
;                                                          whose draws will be captured and StartDraw/StopDraw are
;                                                          relative to the beginning of that frame (only draws belonging
;                                                          to it will be recorded).
;
;  OpenGL.Utilities.TraceGLError                         - If enabled, trace will be amended with error code if
;                                                          any logged function caused an error status. This causes error
;                                                          checking function to be invoked after every logged API call.
;
;  OpenGL.Utilities.ForceGLVersion                       - Overrides the OpenGL version string or number returned to the
;                                                          application. This is useful for changing the behavior of
;                                                          applications.
;
;  OpenGL.Utilities.SuppressExtensions                   - List of extensions to be removed from extension string during
;                                                          recording.
;
;  OpenGL.Utilities.SuppressProgramBinary                - If enabled, recorded applications that use binary programs/shaders
;                                                          will be forced to fail submitting the binaries to driver, by overriding
;                                                          binary length to 1. This is meant to force application to
;                                                          provide textual shaders definition so that the stream can be
;                                                          played back on other drivers.
;
;  OpenGL.Utilities.EndFrameSleep                        - Sleep this many milliseconds after each frame.
;
;  OpenGL.Utilities.RestoreDefaultFB                     - Option removing restoration of default frame buffer (back or
;                                                          front) content.
;
;  OpenGL.Utilities.DoNotRemoveWindow                    - Option skip scheduling Window removing
;
;  OpenGL.Utilities.MultiApiProtectBypass                - Multi API application (for ie those using ES1 and ES2 contexts) are not supported by GITS.
;                                                          Recording such an application will cause an error in gits_xxx.log.
;                                                          Nevertheless it is sometimes possible that such an application may be recorded.
;
;  OpenGL.Utilities.CArrayMemCmpType                     - Specifies the mode of client arrays memory comparison.
;                                                            0 - All. Create diff from whole used memory range.
;                                                            1 - One Range. Create smallest possible one memory range containing all diffs.
;                                                            2 - Multi Range. Create many memory range with diffs.
;
;  OpenGL.Utilities.StripIndicesValues                   - Ignore specified value of index, when scanning index buffers.
;                                                          For 8 and 16 bit indices, lower order bits are used only.
;
;  OpenGL.Utilities.OptimizeBufferSize                   - If application is using buffers mappings this option causes
;                                                          only changed portion of the buffer to be dumped. Disabling
;                                                          this option causes entire buffer to be dumped on every buffer
;                                                          unmap api call but skips compare operations. From measurements done
;                                                          so far on Intel HW enabling this option improves: buffer size,
;                                                          recording performance and player performance so it should be always on.
;                                                          On other HWs user may consider disabling this option what may
;                                                          improve recorder performance.
;
;  OpenGL.Utilities.RetryFunctionLoads                   - If a pointer to an OpenGL function in the driver cannot be loaded
;                                                          (e.g. because a context has not been created yet), reattempt to load it
;                                                          the next time application calls that function.
;
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  OpenGL.Utilities.DetectRecursion                      - Enable/disable detection of recursion when LibDirectory points
;                                                          to gits library instead of system library.
;
;  OpenGL.Utilities.BuffersState                         - Use to specify buffers restoration type. Set to CaptureAlways, Restore or Mixed.
;                                                            CaptureAlways - Calls related with buffers are recorded before first frame recording.
;                                                                            This method doesn't work with Transform Feedback objects.
;                                                            Restore       - Attempt to read data written to write only buffer object mappings. This also allows
;                                                                            for state restore of buffer objects in GLES (but is technically undefined).
;                                                                            That means calls related with buffers are restored (using state restore), and they are not recorded.
;                                                                            Advantage of this method is shorter stream, but this method cannot be used when
;                                                                            pixel buffer objects (textures loaded from buffers) are used.
;                                                            Mixed         - Merge of restoration type 0 and 1. This method should be work with Transform Feedback objects,
;                                                                            and with pixel buffer objects.
;
;  OpenGL.Utilities.TexturesState                        - Use to specify textures restoration model. Set to CaptureAlways, Restore or Mixed.
;                                                           CaptureAlways - Calls related with textures are recorded before first frame recording.
;                                                           Restore       - Enables textures restoration in OpenGLES instead of recording all texture related calls from the beginning.
;                                                                           The downside of this option is that it changes texture format what can cause differences in rendered image
;                                                                           and fails of texsubimage like calls.
;                                                           Mixed         - Merge of restoration type 0 and 1. With this option content of renderable textures with format GL_RGBA
;                                                                           and type GL_UNSIGNED_BYTE is being updated in state restoration process.
;
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  OpenGL.Utilities.CoherentMapUpdatePerFrame            - If set to true, this option enforces updating of the content of buffers,
;                                                          that have been mapped with MAP_COHERENT_BIT flag present,
;                                                          only once per frame. Otherwise updates are performed per every drawcall.
;
%endif
##
%if platform in ["win32"]:
;  OpenGL.Utilities.ScheduleFboEXTAsCoreWA               - Schedule EXT framebuffers calls (extensions: GL_EXT_framebuffer_object,
;                                                          NV_geometry_program4) as Core calls.
;
%endif
##
%if platform in ["win32"]:
;  OpenGL.Utilities.UseGlGetTexImageAndRestoreBuffersWhenPossibleES - Try to use glGetTexImage on OpenGL ES when its exposed by the API.
;
%endif
##
%if platform == "win32":
;  OpenGL.Utilities.TrackTextureBindingWA                - If set to true, gits relies on tracking of currently bound textures
;                                                          via consecutive glBindTexture calls, instead of querying
;                                                          that information from driver with glGetIntegerv.
;
;  OpenGL.Utilities.ForceBuffersStateCaptureAlwaysWA     - Affects state restoration process. If enabled, calls related to
;                                                          buffers are recorded before first frame recording.
;
;  OpenGL.Utilities.RestoreIndexedTexturesWA             - If enabled, textures, whose format is GL_COLOR_INDEX and internal format
;                                                          is GL_RGBA, are restored with glGetTexImage as being of GL_RGBA format.
;
%endif
##
;  OpenGL.Performance.Benchmark                          - Causes GITS to produce csv file with system clock times of each frame.
;
;  OpenGL.Images.DumpScreenshots                         - Rangespec with frames to be captured during the recording.
;
;  OpenGL.Images.DumpDrawsFromFrames                     - Rangespec with frames, from which all drawcalls will be captured.
;
##
;  OpenCL.Capture.Mode                                   - Mode of operation of OpenCL capture, allowed values are All,
;                                                          OclSingleKernel, OclKernelsRange.
;
;  OpenCL.OclSingleKernel.Number                         - A clEnqueueNDRangeKernel call number to be captured.
;                                                          Indexing starts at 1.
;
;  OpenCL.OclKernelsRange.StartKernel                    - A clEnqueueNDRangeKernel call number to start capture
;                                                          Indexing starts at 1.
;
;  OpenCL.OclKernelsRange.StopKernel                     - A clEnqueueNDRangeKernel call number to stop capture.
;                                                          Indexing starts at 1.
;
;  OpenCL.Utilities.DumpKernels                          - Rangespec with kernels to be captured during the recording.
;
;  OpenCL.Utilities.DumpImages                           - Enables capture OCL images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.
;
;  OpenCL.Utilities.OmitReadOnlyObjects                  - Omits dumping for objects created with CL_MEM_READ_ONLY.
;
;  OpenCL.Utilities.BufferResetAfterCreate               - Nullifies Buffer, Image, USM and SVM memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.
;
;  OpenCL.Utilities.NullIndirectPointersInBuffer         - Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  Vulkan.Capture.Mode                                        - Specifies mode of operation and triggering behaviour of GITS recorder. Valid modes are All, Frames, QueueSubmit, CommandBuffersRange.
;                                                               This also selects option group in Vulkan.Capture that is active.
;
;  Vulkan.Capture.All.ExitFrame                               - After this frame, recorder will stop operation and write the stream to filesystem.
;
;  Vulkan.Capture.Frames.StartFrame                           - First frame to be recorder by GITS recorder.
;
;  Vulkan.Capture.Frames.StopFrame                            - The frame number when recording ends (it will be the last frame recorded).
;                                                               If '0' capture will be ended together with tested application.
;
;  Vulkan.Capture.Frames.StartKeys                            - Key combination beginning capture. When given, capture start/stop frames are relative to frame in which keypress occurred.
;                                                               If empty, capture start/stop are relative to the beginning of the application.
;                                                               Option available on:
;                                                                 - Windows (all alpha/special keys)
;
;  Vulkan.Capture.QueueSubmit.Number                          - Only this queue submit will be recorded. The format is "queue_submit_number".
;
;  Vulkan.Capture.CommandBuffersRange.Range                   - Only these command buffers will be recorded. The format is "queue_submit_number/command_buffer_batch_number/command_buffers_range"
;
;  Vulkan.Utilities.TraceVKStructs                            - Trace values of Vulkan structs instead of tracing pointers.
;
;  Vulkan.Utilities.MemorySegmentSize                         - If application is using memory mappings this option causes only changed portion of the memory to be dumped.
;                                                               Disabling this option causes entire memory to be dumped on every memory unmap api call or before vkQueueSubmit,
;                                                               when memory is mapped or used in some submitted CommandBuffer (depending on Vulkan.Utilities.MemoryUpdateState setting),
;                                                               but skips compare operations. This option increase RAM usage during recording and used standalone (without Vulkan.Utilities.MemoryAccessDetection)
;                                                               is very slow in some apps (e.g. Talos Principle). In contrast to Vulkan.Utilities.MemoryAccessDetection this option compare memory byte by byte
;                                                               instead of updating all touched pages (one page has 4096 KB, so this option can better optimize update size).
;
;  Vulkan.Utilities.ShadowMemory                              - When this option is enabled, GITS creates its own copy (a shadow memory) of each mapped memory object and passes replaced pointers to recorded
;                                                               applications so they write to the provided memory. This way GITS can support Vulkan.Utilities.MemoryAccessDetection option when a graphics driver
;                                                               does not allow changing access of it's own memory to READ_ONLY. This option increases RAM usage, so if a platform doesn't have a lot of memory
;                                                               then shadow memory should not be used together with Vulkan.Utilities.MemorySegmentSize. (Exact value depends on app's memory usage, but if
;                                                               it uses 2GB of RAM, it is recommended to have at least 6GB of RAM to use these two options together). This option should be used with
;                                                               Vulkan.Utilities.MemoryAccessDetection, because it decreases performance on its own.
;
;  Vulkan.Utilities.MemoryAccessDetection                     - If an application is using memory mappings, this option causes GITS to dump only memory pages that were written into.
;                                                               It is possible by setting a READ_ONLY flag on mapped memory objects in conjunction with special exception handling routines so GITS can track,
;                                                               which pages are used for writing. Then it updates only parts of memory (with touched pages) on every memory unmap or before vkQueueSubmit,
;                                                               when memory is mapped or used in submitted command buffers (depending on Vulkan.Utilities.MemoryUpdateState setting).
;                                                               Some Vulkan graphics drivers may not allow setting READ_ONLY flags on their own memory, so if the recorder log contains a relevant error,
;                                                               then this option should be used with Vulkan.Utilities.ShadowMemory. It is possible to use this option with Vulkan.Utilities.MemorySegmentSize.
;                                                               This option doesn't work in Frames mode.
;
;  Vulkan.Utilities.ForceUniversalRecording                   - GITS recorder checks if it's attached to a GITS Player to use internal methods for detecting memory changes and improving recording performance.
;                                                               Setting this option to true causes recorder to disable these internal mechanisms and act as if it is recording any other application.
;
##
%if platform == "win32":
;  Vulkan.Utilities.UseExternalMemoryExtension                - If true, GITS replaces memory allocations provided by a driver with its own, host memory allocations by using VK_EXT_external_memory_host extension.
;                                                               This is done only on memory objects which can be mapped so, after mapping, applications write directly to the GITS recorder's own memory, which makes
;                                                               memory updates tracking simpler. This way, GITS does not have to allocate additional memory (so memory usage shouldn't increase too much) and at the
;                                                               same time it is still possible to mark those memory areas as READ_ONLY. Enabling this option may impact rendering perfomance, though, but the influence
;                                                               varies depending on each, specific application.
;
%endif
##
;  Vulkan.Utilities.MemoryUpdateState                         - If application is using memory mappings in coherent mode we need to synchronize memory before vkQueueSubmit. There is two available modes:
;                                                                 AllMapped - this option cause to update all mapped memory at this moment
;                                                                 OnlyUsed  - this option cause to update only mapped memory used in submitting CommandBuffers, so we track information about used buffers and images.
;
;  Vulkan.Utilities.DelayFenceChecksCount                     - This value postpones fence's signaled state to be returned to application; application has to check fence's status or wait for a given fence provided
;                                                               number of times before the signaled state is returned.
;
;  Vulkan.Utilities.ShortenFenceWaitTime                      - Number of nanoseconds to shorten the time application waits on fences; this value is subtracted from the timeout value provided to vkWaitForFences() function.
;
;  Vulkan.Utilities.SuppressExtensions                        - Comma separated (,) list of extensions names to be removed from both Instance and/or Device extenstions reported to recorded application by the driver.
;
;  Vulkan.Utilities.SuppressLayers                            - Comma separated (,) list of layers names to be removed from both Instance and/or Device layers reported to recorded application by the driver.
;
;  Vulkan.Utilities.AddImageUsageFlags                        - Single value representing bit flags which should be added to all images at creation time.
;
;  Vulkan.Utilities.AddBufferUsageFlags                       - Single value representing bit flags which should be added to all buffers at creation time.
;
;  Vulkan.Utilities.ScheduleCommandBuffersBeforeQueueSubmitWA - writing commandBuffers operation to stream just before QueueSubmit, instead of doing it in the same way as app/game/benchmark.
;
;  Vulkan.Utilities.MinimalStateRestore                       - Restore only objects which are actually used in the substream. Option available only for modes QueueSubmit and CommandBuffersRange.
;
;  Vulkan.Utilities.ReusableStateRestoreResourcesCount        - Number of separate resource sets which are concurrently used to restore contents of images and buffers. It cannot be smaller than 2.
;
;  Vulkan.Utilities.ReusableStateRestoreBufferSize            - The size (in megabytes) of temporary buffers used to restore contents of images and buffers. The greater the size, the less copy operations will be performed
;                                                               (and the state restore phase in a recorded substream will be faster) but the memory consumption will be higher.
;
;  Vulkan.Utilities.IncreaseImageMemorySizeRequirement.FixedAmount - Amount of bytes by which the size of image memory requirement reported to application should be increased.
;
;  Vulkan.Utilities.IncreaseImageMemorySizeRequirement.Percent - Percentage by which the size of image memory requirement reported to application should be (relatively) increased.
;
;  Vulkan.Utilities.SuppressPhysicalDeviceFeatures            - Comma separated (,) list of physical device features names to be removed from features reported by the driver. Physical device features are reported with
;                                                               vkGetPhysicalDeviceFeatures() and vkGetPhysicalDeviceFeatures2KHR() function in the VkPhysicalDeviceFeatures structure. Name of each member of this structure
;                                                               can be provided in the option. I.e. to suppress 'geometry shaders' and 'cubemap arrays' specify:
;                                                               SuppressPhysicalDeviceFeatures "imageCubeArray,geometryShader"
;
;  Vulkan.Utilities.MemoryOffsetAlignmentOverride.Images      - defines the memory offset alignment that is returned to the application through a vkGetImageMemoryRequirements() function call
;                                                               in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and
;                                                               must be a power of 2, 0 is disabled, recommended value to use is 4096. By using this option the recorded stream can be played
;                                                               on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.
;
;  Vulkan.Utilities.MemoryOffsetAlignmentOverride.Buffers     - defines the memory offset alignment that is returned to the application through a vkGetBufferMemoryRequirements() function call
;                                                               in a VkMemoryRequirements::alignment structure member. Selected value must be a multiple of the original alignment and
;                                                               must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played
;                                                               on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior
;
;  Vulkan.Utilities.MemoryOffsetAlignmentOverride.Descriptors - defines the memory offset alignment that is returned to the application through a vkGetPhysicalDeviceProperties() function call
;                                                               in minTexelBufferOffsetAlignment, minUniformBufferOffsetAlignment and minStorageBufferOffsetAlignment members
;                                                               of a VkPhysicalDeviceProperties::VkPhysicalDeviceLimits structure. Selected value must be a multiple of the original alignment
;                                                               and must be a power of 2, 0 is disabled, recommended value to use is 256. By using this option the recorded stream can be played
;                                                               on platforms with higher alignment requirements but with a cost of a bigger memory usage and slightly different behavior.
;
;  Vulkan.Utilities.CrossPlatformStateRestoration.Images      - Allows for proper image contents restoration; this is performed through appropriate image layout transitions and data transfer
;                                                               between temporary staging buffer and the image; this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement
;                                                               and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Buffers SHOULD allow playing recorded substreams on various hardware
;                                                               platforms (though the compatibility isn't guaranteed) for the sake of bigger stream size and longer state restore time.
;
;  Vulkan.Utilities.CrossPlatformStateRestoration.Buffers     - Allows for proper buffer contents restoration; this is performed through data transfer between temporary staging buffer and the target;
;                                                               this option together with Vulkan.Utilities.IncreaseImageMemorySizeRequirement and Vulkan.Capture.Frames.CrossPlatformStateRestoration.Images
;                                                               SHOULD allow playing recorded substreams on various hardware platforms (though the compatibility isn't guaranteed) for the sake of bigger
;                                                               stream size and longer state restore time. The following options for buffer memory restoration are available:
;                                                                 None                         - don't restore buffers' memory contents
;                                                                 WithNonHostVisibleMemoryOnly - restore contents of buffers with non-host-visible memory bound (buffers with device-local-only memory)
;                                                                 All                          - restore contents of all buffers
;
;  Vulkan.Utilities.MemoryRestoration                         - Controls restoration of contents of memory objects. The following options are available:
;                                                                 None        - contents of memory objects are not restored
;                                                                 HostVisible - restores contents of all memory objects allocated from a host-visible memory
;
;  Vulkan.Utilities.RestoreMultisampleImagesWA                - Restoration of multisample images via vkCmdCopyImageToBuffer/vkCmdCopyBufferToImage is forbidden
;                                                               by spec, but it is supported by Intel's driver at the time of this writing. Enabling this option
;                                                               fixes corruptions in some titles, when stream recorded on Gen9 is played back on Gen12.
;
;  Vulkan.Utilities.MaxArraySizeForCCode                      - Number that defines the maximum size of the array that will be written to CCode in one scope.
;                                                               Applies to: vkCmdPipelineBarrier(), vkCmdCopyBufferToImage(), vkCmdCopyBuffer(), vkUpdateDescriptorSets().
;
;  Vulkan.Performance.Benchmark                               - Causes GITS to produce csv file with system clock times of frame.
;
;  Vulkan.Images.DumpScreenshots                              - Rangespec with frames to be captured during the recording.
;
;  Vulkan.Images.DumpSubmits                                  - Rangespec with QueueSubmits, from which all submits will be captured.
;
%endif
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  LevelZero.Capture.Mode                                     - Mode of operation of LevelZero capture, allowed values are All, Kernel
;
;  LevelZero.Capture.All                                      - Mode of operation of LevelZero capture. Captures whole stream
;
;  LevelZero.Capture.Kernel                                   - Mode of operation of LevelZero capture. Captures a range of kernels. If chosen one kernel in the range it acts like a single subcapture.
;                                                               Format: CommandQueueSubmitRange/CommandListRange/AppendKernelsRange. Each range must exist and may not go out of boundries.
;                                                               For proper numbering see the recorder's log with LogLevel set to at least TRACE.
;                                                               Format examples:
;                                                               5/5/5
;                                                               5,7,8-10/5,7,8-10/5,7,8-10
;                                                               1-10:2/1-10:2/1-10:2
;
;  LevelZero.Utilities.DumpKernels                            - Format QueueSubmitRange/CommandListRange/AppendKernelsRange. For proper numbering understanding look at recorder's log with LogLevel set to at least TRACE
;                                                               Injects reads with synchronization points. Injected reads are appended into existing command lists. It may cause the host running out of memory,
;                                                               as reads writes into GITS reserved memory space(which GITS holds until queue submit), however, the true gain is the NDRangeBuffer after each executed kernel.
;                                                               If that specific verification is not needed, it is recommended to run this option with DumpAfterSubmit set to True.
;
;  LevelZero.Utilities.DumpAfterSubmit                        - Modifies the way kernel arguments are dumped. Injects immediate command lists, reads and synchronization points after queue submit,
;                                                               instead of injecting read after every appendKernel call. This way dumping process is after command queue execution containing N command lists.
;                                                               If kernel argument is overwritten by many kernels, the output buffer will have the value after last executed kernel that used it.
;                                                               All of other same buffers are skipped. Option reduces allocated memory by GITS
;
;  LevelZero.Utilities.DumpImages                             - Enables capture L0 images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.
;
;  LevelZero.Utilities.BufferResetAfterCreate                 - Nullifies USM Buffer and Image memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.
;
;  LevelZero.Utilities.NullIndirectPointersInBuffer           - Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.
;
%endif
##
;  Extras.Optimizations.TokenBurstLimit                       - The amount of tokens to be used for each recording 'burst'.
;                                                               This many tokens will be accumulated before any IO happens.
;                                                               Packs of that many tokens recorder will be periodically written
;                                                               to HDD in separate thread of execution. Increasing the
;                                                               value will have direct impact on memory used by the recorder.
;                                                               GITS will keep a few packs of that many tokens to facilitate hiding
;                                                               of IO latency.
;
;  Extras.Optimizations.HashType                              - Controls hashing behavior of binary resources. Takes value of
;                                                               Crc32ish , Murmurhash , Xxhash , XxCrc32, IncrementalNumber . This
;                                                               is the hash function that is used to identify all binary resources
;                                                               that GITS handles. Setting this to IncrementalNumber will
;                                                               make hash calculations very fast and effectively eliminate risk
;                                                               of hash collisions at the cost of lack of buffer/texture data deduplication
;                                                               which may result in excessive stream size. XxCrc32 combines XXHash with Crc32Ish
;                                                               and it may slightly affect performance, but it is recommended for recording long
;                                                               streams due to smaller chance of collisions.
;
;  Extras.Optimizations.AsyncBufferWrites                     - Maximum number of bytes of binary data that will be queued
;                                                               to write to hdd in separate thread. Increasing can improve
;                                                               recording performance at cost of memory usage. Specify 0 to
;                                                               disable the feature (data will be written to hdd in main recorder
;                                                               thread).
;
;  Extras.Optimizations.HashPartially                         - When resource hashing is enabled (see 'DontHashResources')
;                                                               the hash is derived from only about ~10% of data to
;                                                               minimize the cost of hash computation. This introduces
;                                                               higher risk of collisions but should improve
;                                                               recording performance.
;                                                               WARNING: This option _will_ in general cause stream corruptions
;                                                                        and will require tweeking accompanying parameters
;                                                                        to obtain useful results. Use with caution.
;
;  Extras.Optimizations.PartialHashCutoff                     - Partial hash is computed only for blocks of data larger
;                                                               then this (in bytes). Smaller blocks are fully hashed.
;
;  Extras.Optimizations.PartialHashChunks                     - Number of evenly distributed chunks within hashed block
;                                                               to be included during hash computation.
;
;  Extras.Optimizations.PartialHashRatio                      - Specified the amount of data to be hashed specified as a divisor.
;                                                                 1 - hash whole block
;                                                                 2 - hash half of the block
;                                                                10 - hash only 10% of the block
;                                                               200 - hash only 0.5% of the block
;                                                               etc ...
;
;  Extras.Optimizations.BufferMapAccessMask                   - This mask may be used to remove undesired options from buffer mapping
;                                                               access bitfield specified by glMapBufferRange for example. By default
;                                                               GITS removes GL_MAP_INVALIDATE_BUFFER_BIT and GL_MAP_INVALIDATE_RANGE_BIT
;                                                               to improve performance of the recorder. Mask removes those options
;                                                               in recorder so it has an influence on how application and further
;                                                               also stream behaves.
;
;  Extras.Optimizations.BufferStorageFlagsMask                - This mask may be used to add options to storage flags bitfield
;                                                               specified by glBufferStorage or glNamedBufferStorageEXT. By default
;                                                               GITS adds GL_MAP_READ_BIT.
;
;  Extras.Optimizations.RemoveResourceHash                    - Removes resource checksum validation. Injects unique value as a hash for
;                                                               every data object.
;
;  Extras.Utilities.ExtendedDiagnostic                        - Enables gathering of system diagnostic info during stream recording.
;                                                               Turn off if Gits causes application to crash during startup.
;
;  Extras.Utilities.ForceDumpOnError                          - Enables forcing dumping of GITS artifacts when an error is detected.
;                                                               May lead to abort or other serious problem.
;
;  Extras.Utilities.ZipTextFiles                              - If enabled, shaders, kernels and other text files produced by
;                                                               GITS will be stored in a zip archives instead of directories.
;
;  Extras.Utilities.HighIntegrity                             - Forces recorder to operate in high integrity mode, which can
;                                                               heavily impact recorder performance, but will allow for creation
;                                                               of usable stream, even if recorded application crashes
;                                                               before end of recording. Stream is effectively persisted to filesystem
;                                                               after each API call.
;
;  Extras.Utilities.NullIO                                    - Testing mode which does not perform any IO, thus useful only
;                                                               for performance testing of GITS recorder itself.
;
;  Extras.Utilities.EventScript                               - Absolute path to Lua event script to be used during the recording.
;
;  Extras.Utilities.RemoveAPISharing                          - Removes sharing API calls and replaces it with its 'equivalent' in the pure API.
;                                                               Supported APIs to remove in OpenCL: 'DX', 'OGL'.
;                                                               If specified 'All' all API sharing is removed.
;
##
%if platform in ["win32", "lnx_32", "lnx_64", "lnx_arm"]:
;  Extras.Utilities.CoherentMapBehaviorWA                     - WA for Wolfenstein The Old Blood. This game use extension ARB_buffer_storage improperly.
;                                                               It behaves like MAP_COHERENT_BIT flag is active, but only MAP_PERSISTENT_BIT exist in flags.
;                                                               This WA cause GITS to treat buffer mapping as COHERENT also in case of only
;                                                               MAP_PERSISTENT_BIT is active (by default all buffers mapped with MAP_COHERENT_BIT is updated in each frame by GITS).
;
%endif
##
%if platform == "win32":
;  Extras.Utilities.MTDriverWA                                - Some functions like wglDescribePixelFormat, wglGetPixelFormat,
;                                                               wglSetPixelFormat is used in threaded drivers off the 'main' thread
;                                                               and causes deadlock due to GITS api mutex. This WA cause skipping record of
;                                                               these functions when deadlock occurs. In recorder log in this situation you can
;                                                               see warning: "Deadlock detected in ...". Deadlock detection is not perfect
;                                                               and theoretically may cause an unexpected behaviour.
;                                                               Alternative to this WA is disabling multithreading in driver.
;
;  Extras.Utilities.CloseAppOnStopRecording                   - Forces application, which is being recorded, to be closed immediately after the recording process is finished.
;
;  Extras.Utilities.WindowsKeyHandling                        - Select keyboard handling method on Windows:
;                                                                 MessageLoop - key presses are detected as events returned by GetMessage WinAPI function
;                                                                 AsyncKeyState - key presses are checked with GetAsyncKeyState WinAPI function
;
%endif
##
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
