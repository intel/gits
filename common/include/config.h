// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configTypes.h"
#include "bit_range.h"
#include "pragmas.h"
#include "log.h"
#include "tools.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

#ifndef BUILD_FOR_CCODE
#include "configUtils.h"
#include "yaml-cpp/yaml.h"
#endif

namespace gits {
struct Config {
  enum TMode {
    MODE_UNKNOWN,
    MODE_RECORDER,
    MODE_PLAYER
  };

  static const std::string CONFIG_FILE_NAME;
  static std::filesystem::path GetConfigPath(const std::filesystem::path& appDir);

  static const Config& Get() {
    return *config;
  }
  static void Set(const Config& cfg);
  static void SetMode(const Config::TMode& mode);
  static bool Set(const std::filesystem::path& cfgPath, const Config::TMode& mode = MODE_RECORDER);

  static bool IsRecorder();
  static bool IsPlayer();
  static bool dumpBinary();
  static bool dumpCCode();

  struct CCode {
    std::filesystem::path outputPath;
    vi_uint benchmarkStartFrame;
  } ccode;

  struct Common {
    TMode mode;
    struct Shared {
      LogLevel thresholdLogLevel;
      vi_bool logToConsole;
      std::set<TraceData> traceDataOpts;
      vi_bool useEvents;
      std::string scriptArgsStr;

      std::filesystem::path libGL;
      std::filesystem::path libEGL;
      std::filesystem::path libGLESv1;
      std::filesystem::path libGLESv2;
      std::filesystem::path libClPath;
      std::filesystem::path libVK;
      std::filesystem::path libOcloc;
      std::filesystem::path libL0Driver;
      std::filesystem::path libL0;
    } shared;

    struct Player {
      std::filesystem::path applicationPath;
      std::string helpGroup;
      vi_bool stats;
      vi_bool version;
      vi_bool diags;

      std::filesystem::path libGL;
      std::filesystem::path libEGL;
      std::filesystem::path libGLESv1;
      std::filesystem::path libGLESv2;
      std::filesystem::path libClPath;
      std::filesystem::path libVK;
      std::filesystem::path libOcloc;
      std::filesystem::path libL0;

      std::filesystem::path streamPath;
      std::filesystem::path streamDir;
      vi_uint tokenBurst;
      vi_uint tokenBurstNum;
      vi_uint exitFrame;
      vi_uint endFrameSleep;
      vi_bool exitOnError;
      vi_bool useEvents;
      std::filesystem::path eventScript;
      std::string scriptArgsStr;
      vi_bool benchmark;
      vi_bool showWindowBorder;
      vi_bool dontVerifyStream;
      BitRange captureFrames;
      std::filesystem::path outputDir;
      BitRange traceSelectedFrames;
      vi_bool interactive;
      std::filesystem::path outputTracePath;
      vi_bool logFncs;
      vi_bool faithfulThreading;
      vi_bool loadWholeStreamBeforePlayback;
      vi_bool signStream;
      vi_bool verifyStream;
      vi_bool showWindowsWA;
      vi_bool disableExceptionHandling;
      vi_bool captureScreenshot;
      vi_bool logLoadedTokens;
      vi_bool escalatePriority;
      vi_bool swapAfterPrepare;
      BitRange stopAfterFrames;
      vi_bool nullRun;
      vi_bool waitForEnter;
      vi_bool cleanResourcesOnExit;
      vi_bool renderOffscreen;
      vi_bool forceOrigScreenResolution;
      vi_bool forceInvisibleWindows;
      vi_bool fullscreen;
      WindowMode windowMode;

      struct ForceWindowPos {
        vi_bool enabled;
        vi_uint x;
        vi_uint y;
      } forceWindowPos;
      struct ForceWindowSize {
        vi_bool enabled;
        vi_uint width;
        vi_uint height;
      } forceWindowSize;
      struct ForceScissor {
        vi_bool enabled;
        vi_uint x;
        vi_uint y;
        vi_uint width;
        vi_uint height;
      } forceScissor;

#ifdef GITS_PLATFORM_WINDOWS
      struct ForceDesktopResolution {
        vi_bool enabled;
        vi_uint width;
        vi_uint height;
      } forceDesktopResolution;
#endif

    } player;

    struct Recorder {
      std::string PrepareDumpPath(const std::string& str, bool uniqueDump);

      std::filesystem::path libGL;
      std::filesystem::path libEGL;
      std::filesystem::path libGLESv1;
      std::filesystem::path libGLESv2;
      std::filesystem::path libClPath;
      std::filesystem::path libVK;
      std::filesystem::path libOcloc;
      std::filesystem::path libL0;

      RecordingModeOpt recordingMode;
      std::vector<std::string> exitKeysStr;
      std::vector<uint32_t> exitKeys;
      vi_uint exitAfterAPICall;
#ifdef GITS_PLATFORM_LINUX
      vi_uint exitSignal;
#endif
      vi_uint tokenBurst;
      vi_uint tokenBurstNum;

      std::filesystem::path installPath;
      std::filesystem::path dumpPath;
      vi_bool uniqueDumpDirectory;
      vi_bool enabled;

      vi_bool useEvents;
      std::filesystem::path eventScript;
      std::string scriptArgsStr;
      struct Compression {
        CompressionTypeOpt type;
        vi_uint level;
        vi_uint chunkSize;
      } compression;
      vi_bool extendedDiagnosticInfo;
      vi_bool forceDumpOnError;
      vi_bool zipTextFiles;
      vi_bool highIntegrity;
      vi_bool nullIO;
      vi_bool removeDXSharing;
      vi_bool removeGLSharing;
      vi_bool benchmark;
#ifdef GITS_PLATFORM_WINDOWS
      vi_bool closeAppOnStopRecording;
      WindowsKeyHandlingOpt windowsKeyHandling;
#endif
    } recorder;
  } common;

  struct OpenGL {
    struct Shared {
      std::string forceGLVersion;
      vi_uint forceGLVersionMajor;
      vi_uint forceGLVersionMinor;
      vi_bool traceGLError;
    } shared;

    struct Player {
      GLProfileOpt forceGLProfile;
      GLNativeApiOpt forceGLNativeAPI;
      vi_bool skipQueries;
      float scaleFactor;
      vi_bool captureFramesHashes;
      vi_bool dontForceBackBufferGL;
      vi_bool captureWholeWindow;
      BitRange capture2DTexs;
      BitRange captureDraws2DTexs;
      BitRange captureDraws;
      vi_bool captureDrawsPre;
      BitRange captureFinishFrame;
      BitRange captureReadPixels;
      BitRange captureFlushFrame;
      BitRange captureBindFboFrame;
      BitRange keepDraws;
      BitRange keepFrames;
      vi_bool minimalConfig;
      vi_bool traceGitsInternal;
      vi_bool linkGetProgBinary;
      vi_bool linkUseProgBinary;
      vi_bool affectViewport;
      std::vector<int> affectedViewport;
      BitRange traceGLBufferHashes;
      vi_bool forceNoMSAA;
      vi_bool destroyContextsOnExit;
#ifdef GITS_PLATFORM_LINUX
      vi_bool forceWaylandWindow;
#endif
    } player;

    struct Recorder {
      OpenGLRecorderModeOpt mode;
      struct All {
        vi_uint exitFrame;
        vi_uint exitDeleteContext;
      } all;
      struct Frames {
        vi_uint startFrame;
        vi_uint stopFrame;
        std::vector<std::string> startKeysStr;
        std::vector<uint32_t> startKeys;
        struct FrameSeparators {
          vi_bool glFinishSep;
          vi_bool glFlushSep;
        } frameSeparators;
      } frames;
      struct OglSingleDraw {
        vi_uint number;
      } oglSingleDraw;
      struct OglDrawsRange {
        vi_uint startDraw;
        vi_uint stopDraw;
        vi_uint frame;
      } oglDrawsRange;
      BitRange dumpScreenshots;
      BitRange dumpDrawsFromFrames;
      std::vector<std::string> suppressExtensions;
      vi_bool suppressProgramBinary;
      vi_uint endFrameSleep;
      vi_bool restoreDefaultFB;
      vi_bool doNotRemoveWin;
      vi_bool multiApiProtectBypass;
      vi_uint carrayMemCmpType;
      vi_uint stripIndicesValues;
      vi_bool optimizeBufferSize;
      vi_bool retryFunctionLoads;
      vi_bool detectRecursion;
      BuffersStateOpt buffersState;
      TexturesStateOpt texturesState;
      vi_bool coherentMapUpdatePerFrame;
      vi_uint bufferMapAccessMask;
      vi_uint bufferStorageFlagsMask;
      vi_bool coherentMapBehaviorWA;
#ifdef GITS_PLATFORM_WINDOWS
      vi_bool schedulefboEXTAsCoreWA;
      vi_bool useGlGetTexImageAndRestoreBuffersWhenPossibleES;
      vi_bool trackTextureBindingWA;
      vi_bool forceBuffersStateCaptureAlwaysWA;
      vi_bool restoreIndexedTexturesWA;
      vi_bool mtDriverWA;
#endif
      vi_bool ccodeRangesWA;
    } recorder;
  } opengl;

  struct Vulkan {
    struct Shared {
      std::vector<std::string> suppressPhysicalDeviceFeatures;
      std::vector<std::string> suppressExtensions;
      std::vector<std::string> suppressLayers;
    } shared;

    struct Player {
      vi_bool exitOnVkQueueSubmitFail;
      BitRange captureVulkanSubmits;
      BitRange captureVulkanSubmitsResources;
      VKCaptureGroupTypeOpt captureVulkanSubmitsGroupType;
      VulkanObjectRange captureVulkanRenderPasses;
      VulkanObjectRange captureVulkanRenderPassesResources;
      VulkanObjectRange captureVulkanDraws;
      VulkanObjectRange captureVulkanResources;
      vi_bool skipNonDeterministicImages;
      vi_bool ignoreVKCrossPlatformIncompatibilitiesWA;
      vi_bool waitAfterQueueSubmitWA;
      vi_bool traceVKShaderHashes;
      vi_uint maxAllowedVkSwapchainRewinds;
      std::filesystem::path overrideVKPipelineCache;
      vi_bool oneVulkanDrawPerCommandBuffer;
      vi_bool oneVulkanRenderPassPerCommandBuffer;
      vi_uint vulkanForcedPhysicalDeviceIndex;
      std::string vulkanForcedPhysicalDeviceName;
      DeviceTypeOpt vulkanForcedPhysicalDeviceType;
      vi_bool printStateRestoreLogsVk;
      vi_bool printMemUsageVk;
      vi_bool forceMultithreadedPipelineCompilation;
      vi_bool execCmdBuffsBeforeQueueSubmit;
#ifdef GITS_PLATFORM_WINDOWS
      struct RenderDoc {
        VkRenderDocCaptureOpt mode;
        BitRange captureRange;
        std::filesystem::path dllPath;
        vi_bool continuousCapture;
        vi_bool enableUI;
      } renderDoc;
#endif
    } player;

    struct Recorder {
      void SetRangeSpecial(const std::string& val,
                           const VulkanObjectMode& mode,
                           size_t expectedVecSize);

      VulkanRecorderModeOpt mode;
      struct All {
        vi_uint exitFrame;
      } all;
      struct Frames {
        vi_uint startFrame;
        vi_uint stopFrame;
        std::vector<std::string> startKeysStr;
        std::vector<uint32_t> startKeys;
      } frames;
      // needed to load config
      std::string queueSubmitStr;
      std::string commandBuffersRangeStr;
      std::string renderPassRangeStr;
      std::string drawsRangeStr;
      std::string dispatchRangeStr;
      std::string blitRangeStr;

      struct Objects {
        VulkanObjectRange rangeSpecial;
      } objRange;
      BitRange dumpScreenshots;
      BitRange dumpSubmits;
      vi_bool traceVkStructs;
      vi_uint memorySegmentSize;
      vi_bool shadowMemory;
      vi_bool memoryAccessDetection;
      vi_bool writeWatchDetection;
      MemoryTrackingModeOpt memoryTrackingMode;
      MemoryUpdateStateOpt memoryUpdateState;
      vi_bool forceUniversalRecording;
      vi_uint delayFenceChecksCount;
      vi_uint shortenFenceWaitTime;
      vi_uint addImageUsageFlags;
      vi_uint addBufferUsageFlags;
      vi_bool scheduleCommandBuffersBeforeQueueSubmit;
      vi_bool minimalStateRestore;
      vi_uint reusableStateRestoreResourcesCount;
      vi_uint reusableStateRestoreBufferSize;
      MemorySizeRequirementOverride increaseImageMemorySizeRequirement;
      struct IncreaseAccelerationStructureMemorySizeRequirement {
        MemorySizeRequirementOverride accelerationStructureSize;
        MemorySizeRequirementOverride buildScratchSize;
        MemorySizeRequirementOverride updateScratchSize;
      } increaseAccelerationStructureMemorySizeRequirement;
      struct MemoryOffsetAlignmentOverride {
        vi_uint images;
        vi_uint buffers;
        vi_uint descriptors;
      } memoryOffsetAlignmentOverride;
      struct CrossPlatformStateRestoration {
        vi_bool images;
        BufferStateRestorationOpt buffers;
      } crossPlatformStateRestoration;
      MemoryStateRestorationOpt memoryRestoration;
      vi_bool restoreMultisampleImagesWA;
      vi_uint maxArraySizeForCCode;
      vi_bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
      vi_bool useCaptureReplayFeaturesForRayTracingPipelines;
#ifdef GITS_PLATFORM_WINDOWS
      vi_bool useExternalMemoryExtension;
      vi_bool usePresentSrcLayoutTransitionAsAFrameBoundary;
      vi_bool renderDocCompatibility;
      std::vector<std::string> renderDocCompatibilitySuppressedExtensions = {
          "VK_EXT_graphics_pipeline_library",
          "VK_EXT_extended_dynamic_state3",
          "VK_EXT_external_memory_host",
          "VK_KHR_map_memory2",
          "VK_EXT_dynamic_rendering_unused_attachments",
          "VK_EXT_host_image_copy",
          "VK_KHR_maintenance5"};
#endif
    } recorder;
  } vulkan;

  struct OpenCL {
    struct Player {
      vi_bool captureImages;
      vi_bool removeSourceLengths;
      vi_bool captureReads;
      BitRange captureKernels;
      vi_bool omitReadOnlyObjects;
      vi_bool dumpLayoutOnly;
      vi_bool injectBufferResetAfterCreate;
      vi_bool disableNullIndirectPointersInBuffer;
      vi_bool noOpenCL;
      vi_bool aubSignaturesCL;
    } player;

    struct Recorder {
      OpenCLRecorderModeOpt mode;
      struct OclSingleKernel {
        vi_uint number;
      } oclSingleKernel;
      struct OclKernelsRange {
        vi_uint startKernel;
        vi_uint stopKernel;
      } oclKernelsRange;
      BitRange dumpKernels;
      vi_bool dumpImages;
      vi_bool omitReadOnlyObjects;
      vi_bool bufferResetAfterCreate;
      vi_bool nullIndirectPointersInBuffer;
    } recorder;
  } opencl;

  struct LevelZero {
    std::vector<std::string> ParseCaptureKernels(const std::string& value);

    struct Player {
      vi_bool captureImages;
      vi_bool dumpSpv;
      std::string captureKernelsStr;
      BitRange captureKernels;
      BitRange captureCommandLists;
      BitRange captureCommandQueues;
      vi_bool dumpLayoutOnly;
      vi_bool captureAfterSubmit;
      vi_bool captureInputKernels;
      vi_bool injectBufferResetAfterCreate;
      vi_bool disableNullIndirectPointersInBuffer;
      vi_uint disableAddressTranslation;
      vi_bool omitOriginalAddressCheck;
    } player;

    struct Recorder {
      LevelZeroRecorderModeOpt mode;
      std::string kernelRangeStr;
      struct Kernel {
        vi_uint startKernel;
        vi_uint stopKernel;
        vi_uint stopCommandList;
        vi_uint startCommandList;
        vi_uint stopCommandQueueSubmit;
        vi_uint startCommandQueueSubmit;
        vi_bool singleCapture;
      } kernel;
      std::string dumpKernelsStr;
      BitRange captureCommandQueues;
      BitRange captureKernels;
      BitRange captureCommandLists;
      vi_bool captureAfterSubmit;
      vi_bool captureImages;
      vi_bool bufferResetAfterCreate;
      vi_bool nullIndirectPointersInBuffer;
      vi_bool dumpInputKernels;
      struct BruteForceScanForIndirectPointers {
        vi_uint memoryType;
        vi_uint iterations;
      } bruteForceScanForIndirectPointers;
      struct AddressTranslation {
        vi_uint memoryType;
        vi_uint64 virtualDeviceMemorySize;
        vi_uint64 virtualHostMemorySize;
      } disableAddressTranslation;
      vi_bool dumpLayoutOnly;
    } recorder;
  } levelzero;

private:
  Config();
  static Config* config;

#ifndef BUILD_FOR_CCODE
  static YAML::Node LoadConfigFile(const std::filesystem::path& cfgDir);
  // Set funcs
  void SetCommon(const YAML::Node& commonYaml);
  void SetOpenGL(const YAML::Node& openglYaml);
  void SetVulkan(const YAML::Node& vulkanYaml);
  void SetOpenCL(const YAML::Node& openclYaml);
  void SetLevelZero(const YAML::Node& levelzeroYaml);
  void LoadLevelZeroSubcaptureSettings(const std::string& kernelInfo);
#endif
};

bool isTraceDataOptPresent(TraceData option);
} // namespace gits
