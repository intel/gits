// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "bit_range.h"
#include "pragmas.h"
#include "log.h"
#include "tools.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

DISABLE_WARNINGS
#ifdef check
#undef check
#endif
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>
ENABLE_WARNINGS

namespace gits {
namespace detail {
struct StringCompare {
  bool operator()(const char* lhs, const char* rhs) const {
    return std::strcmp(lhs, rhs) < 0;
  }
};
} // namespace detail
template <class Domain, class Derived>
struct NamedValuesBase {
  typedef NamedValuesBase<Domain, Derived> base_t;
  NamedValuesBase() {
    init();

    assert(!values().empty());
    value_ = Domain();
  }
  NamedValuesBase(const char* n) {
    init();
    value_ = get_value(n);
  }

  typedef std::map<const char*, Domain, detail::StringCompare> map_t;

  static const char*& readable_name() {
    static const char* n;
    return n;
  }

  static Domain& default_value() {
    static Domain dvalue;
    return dvalue;
  }

  static map_t& values() {
    static map_t m;
    return m;
  }

  static Domain get_value(const char* n) {
    if (values().find(n) == values().end()) {
      std::ostringstream output;

      output << "Invalid named value '" << std::string(n) << "' for type "
             << std::string(readable_name()) << ".\n";
      output << "Available values are:";
      for (auto& val : values()) {
        output << "\n - " << val.first;
      }
      throw std::runtime_error(output.str());
    }

    return values()[n];
  }

  operator Domain() const {
    return value_;
  }

  void setFromString(const std::string& str) {
    value_ = get_value(str.c_str());
  }

  friend std::ostream& operator<<(std::ostream& lhs, const base_t& rhs) {
    lhs << rhs.value_;
    return lhs;
  }

  friend std::istream& operator>>(std::istream& lhs, base_t& rhs) {
    std::string n;
    lhs >> n;
    rhs.setFromString(n);
    return lhs;
  }

private:
  void init() {
    if (readable_name() == 0) {
      Derived::describe_type();
    }
  }
  Domain value_;
};

enum class TForcedGLProfile { NO_PROFILE_FORCED, COMPAT, CORE, ES };

enum class TForcedGLNativeApi { NO_NTV_API_FORCED, EGL, GLX, WGL };

enum class TBuffersState {
  BUFFERS_STATE_CAPTURE_ALWAYS,
  BUFFERS_STATE_RESTORE,
  BUFFERS_STATE_MIXED
};

enum class TTexturesState {
  TEXTURES_STATE_CAPTURE_ALWAYS,
  TEXTURES_STATE_RESTORE,
  TEXTURES_STATE_MIXED
};

enum class TMemoryUpdateStates {
  MEMORY_STATE_UPDATE_ALL_MAPPED,
  MEMORY_STATE_UPDATE_ONLY_USED,
  MEMORY_STATE_UPDATE_USING_TAGS
};

enum class TMemoryStateRestoration {
  MEMORY_STATE_RESTORATION_NONE,
  MEMORY_STATE_RESTORATION_HOST_VISIBLE
};

enum class TBufferStateRestoration {
  BUFFER_STATE_RESTORATION_NONE,
  BUFFER_STATE_RESTORATION_WITH_NON_HOST_VISIBLE_MEMORY_ONLY,
  BUFFER_STATE_RESTORATION_ALL
};

enum class TWindowsKeyHandling { MESSAGE_LOOP, ASYNC_KEY_STATE };

enum class TCaptureGroupType { PER_COMMANDBUFFER, PER_RENDERPASS };

enum class TDeviceType { DEVICE_TYPE_ANY, DEVICE_TYPE_INTEGRATED, DEVICE_TYPE_DISCRETE };

struct GLProfileOpt : NamedValuesBase<TForcedGLProfile, GLProfileOpt> {
  static void describe_type() {
    readable_name() = "Profile";
    default_value() = TForcedGLProfile::NO_PROFILE_FORCED;
    values()["COMPAT"] = TForcedGLProfile::COMPAT;
    values()["CORE"] = TForcedGLProfile::CORE;
    values()["ES"] = TForcedGLProfile::ES;
  }
};

struct GLNativeApiOpt : NamedValuesBase<TForcedGLNativeApi, GLNativeApiOpt> {
  static void describe_type() {
    readable_name() = "Native";
    default_value() = TForcedGLNativeApi::NO_NTV_API_FORCED;
    values()["EGL"] = TForcedGLNativeApi::EGL;
    values()["WGL"] = TForcedGLNativeApi::WGL;
    values()["GLX"] = TForcedGLNativeApi::GLX;
  }
};

struct HashTypeOpt : NamedValuesBase<THashType, HashTypeOpt> {
  static void describe_type() {
    readable_name() = "HashType";
    default_value() = THashType::CRC32ISH;
    values()["Murmurhash"] = THashType::MURMUR;
    values()["Xxhash"] = THashType::XX;
    values()["IncrementalNumber"] = THashType::INCREMENTAL_NUMBER;
    values()["Crc32ish"] = THashType::CRC32ISH;
    values()["XxCrc32"] = THashType::XXCRC32;
  }
};

struct CompressionTypeOpt : NamedValuesBase<CompressionType, CompressionTypeOpt> {
  static void describe_type() {
    readable_name() = "Type";
    default_value() = CompressionType::NONE;
    values()["None"] = CompressionType::NONE;
    values()["LZ4"] = CompressionType::LZ4;
    values()["ZSTD"] = CompressionType::ZSTD;
  }
};

struct BuffersStateOpt : NamedValuesBase<TBuffersState, BuffersStateOpt> {
  static void describe_type() {
    readable_name() = "BuffersState";
    default_value() = TBuffersState::BUFFERS_STATE_CAPTURE_ALWAYS;
    values()["CaptureAlways"] = TBuffersState::BUFFERS_STATE_CAPTURE_ALWAYS;
    values()["Restore"] = TBuffersState::BUFFERS_STATE_RESTORE;
    values()["Mixed"] = TBuffersState::BUFFERS_STATE_MIXED;
  }
};

struct TexturesStateOpt : NamedValuesBase<TTexturesState, TexturesStateOpt> {
  static void describe_type() {
    readable_name() = "TexturesState";
    default_value() = TTexturesState::TEXTURES_STATE_MIXED;
    values()["CaptureAlways"] = TTexturesState::TEXTURES_STATE_CAPTURE_ALWAYS;
    values()["Restore"] = TTexturesState::TEXTURES_STATE_RESTORE;
    values()["Mixed"] = TTexturesState::TEXTURES_STATE_MIXED;
  }
};

struct MemoryUpdateStateOpt : NamedValuesBase<TMemoryUpdateStates, MemoryUpdateStateOpt> {
  static void describe_type() {
    readable_name() = "MemoryUpdateState";
    default_value() = TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED;
    values()["AllMapped"] = TMemoryUpdateStates::MEMORY_STATE_UPDATE_ALL_MAPPED;
    values()["OnlyUsed"] = TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED;
    values()["UsingTags"] = TMemoryUpdateStates::MEMORY_STATE_UPDATE_USING_TAGS;
  }
};

struct MemoryStateRestorationOpt
    : NamedValuesBase<TMemoryStateRestoration, MemoryStateRestorationOpt> {
  static void describe_type() {
    readable_name() = "MemoryRestoration";
    default_value() = TMemoryStateRestoration::MEMORY_STATE_RESTORATION_HOST_VISIBLE;
    values()["None"] = TMemoryStateRestoration::MEMORY_STATE_RESTORATION_NONE;
    values()["HostVisible"] = TMemoryStateRestoration::MEMORY_STATE_RESTORATION_HOST_VISIBLE;
  }
};

struct BufferStateRestorationOpt
    : NamedValuesBase<TBufferStateRestoration, BufferStateRestorationOpt> {
  static void describe_type() {
    readable_name() = "Buffers";
    default_value() =
        TBufferStateRestoration::BUFFER_STATE_RESTORATION_WITH_NON_HOST_VISIBLE_MEMORY_ONLY;
    values()["None"] = TBufferStateRestoration::BUFFER_STATE_RESTORATION_NONE;
    values()["WithNonHostVisibleMemoryOnly"] =
        TBufferStateRestoration::BUFFER_STATE_RESTORATION_WITH_NON_HOST_VISIBLE_MEMORY_ONLY;
    values()["All"] = TBufferStateRestoration::BUFFER_STATE_RESTORATION_ALL;
  }
};

struct WindowsKeyHandlingOpt : NamedValuesBase<TWindowsKeyHandling, WindowsKeyHandlingOpt> {
  static void describe_type() {
    readable_name() = "WindowsKeyHandling";
    default_value() = TWindowsKeyHandling::MESSAGE_LOOP;
    values()["MessageLoop"] = TWindowsKeyHandling::MESSAGE_LOOP;
    values()["AsyncKeyState"] = TWindowsKeyHandling::ASYNC_KEY_STATE;
  }
};

struct VKCaptureGroupTypeOpt : NamedValuesBase<TCaptureGroupType, VKCaptureGroupTypeOpt> {
  static void describe_type() {
    readable_name() = "VKCaptureGroupType";
    default_value() = TCaptureGroupType::PER_COMMANDBUFFER;
    values()["CmdBuffer"] = TCaptureGroupType::PER_COMMANDBUFFER;
    values()["RenderPass"] = TCaptureGroupType::PER_RENDERPASS;
  }
};

struct DeviceTypeOpt : NamedValuesBase<TDeviceType, DeviceTypeOpt> {
  static void describe_type() {
    readable_name() = "DeviceType";
    default_value() = TDeviceType::DEVICE_TYPE_ANY;
    values()["any"] = TDeviceType::DEVICE_TYPE_ANY;
    values()["integrated"] = TDeviceType::DEVICE_TYPE_INTEGRATED;
    values()["discrete"] = TDeviceType::DEVICE_TYPE_DISCRETE;
  }
};

// A lot of values in config are left untouched at 'default' zero-like
// value. Following will allow to enumerate only defaults that are
// special.
template <class T>
class value_initialized {
public:
  value_initialized() : x() {}
  operator T&() {
    return x;
  }
  operator const T&() const {
    return x;
  }
  value_initialized& operator=(const T& val) {
    x = val;
    return *this;
  }

private:
  T x;
};

typedef value_initialized<bool> vi_bool;
typedef value_initialized<int32_t> vi_int;
typedef value_initialized<uint32_t> vi_uint;
typedef value_initialized<uint64_t> vi_uint64;
typedef value_initialized<float> vi_float;

struct Config {
  enum TMode { MODE_UNKNOWN, MODE_RECORDER, MODE_PLAYER };
  struct ObjectRange {
    std::vector<uint32_t> objVector;
    BitRange range;
    bool empty() const {
      return range.empty();
    }
  };
  enum VulkanObjectMode {
    MODE_VKNONE,
    MODE_VKQUEUESUBMIT,
    MODE_VKCOMMANDBUFFER,
    MODE_VKRENDERPASS,
    MODE_VKDRAW,
    MODE_VKDISPATCH,
    MODE_VKBLIT
  };
  struct VulkanObjectRange : ObjectRange {
    VulkanObjectMode objMode;

    bool operator[](uint64_t queueSubmitNumber) const;
  };
  enum class WindowMode {
    NORMAL,
    EXCLUSIVE_FULLSCREEN,
  };

  static const Config& Get() {
    return *config;
  }
  static void Set(const Config& cfg);
  static bool Set(const std::filesystem::path& cfgDir);

  static bool IsRecorder();
  static bool IsPlayer();

  struct Common {
    TMode mode;
    std::filesystem::path installPath;
    std::filesystem::path streamDir;
    std::filesystem::path libClPath;
    std::filesystem::path libGL;
    std::filesystem::path libEGL;
    std::filesystem::path libGLESv1;
    std::filesystem::path libGLESv2;
    std::filesystem::path libVK;
    std::filesystem::path libL0;
    std::filesystem::path libL0Driver;
    std::filesystem::path libOcloc;
    LogLevel thresholdLogLevel;
    std::set<TraceData> traceDataOpts;

    vi_uint tokenBurst;
    vi_uint tokenBurstNum;
    vi_bool useZoneAllocator;

    //OpenGL common
    vi_bool traceGLError;
    vi_bool useEvents;
    std::string scriptArgsStr;

    //RS common
  } common;

  struct CCode {
    std::filesystem::path outputPath;
    vi_uint benchmarkStartFrame;
  } ccode;

  struct Player {
    std::string helpGroup;
    vi_bool version;
    vi_uint exitFrame;
    vi_uint exitCommandBuffer;
    vi_bool exitOnError;
    vi_bool exitOnVkQueueSubmitFail;
    std::filesystem::path streamPath;
    vi_bool interactive;
    vi_bool errors;
    std::string plugin;
    vi_bool stats;
    vi_bool statsVerb;
    vi_bool disableExceptionHandling;
    vi_bool escalatePriority;
    vi_bool swapAfterPrepare;
    vi_bool skipQueries;
    std::filesystem::path outputDir;
    std::filesystem::path outputTracePath;
    std::filesystem::path applicationPath;
    float scaleFactor;
    vi_bool logFncs;
    vi_bool logLoadedTokens;
    vi_bool loadWholeStreamBeforePlayback;

    vi_bool minimalConfig;
    vi_bool loadResourcesImmediately;
    BitRange stopAfterFrames;
    vi_uint tokenLoadLimit;
    BitRange captureFrames;
    BitRange traceSelectedFrames;
    vi_bool captureFramesHashes;
    vi_bool captureScreenshot;
    vi_bool captureWholeWindow;
    vi_bool dontForceBackBufferGL;
    vi_bool forceOrigScreenResolution;
    vi_bool showWindowBorder;
    WindowMode windowMode;
    vi_bool aubSignaturesCL;
    BitRange capture2DTexs;
    vi_bool clCaptureImages;
    vi_bool clRemoveSourceLengths;
    vi_bool l0CaptureImages;
    vi_bool l0CaptureInputKernels;
    vi_bool l0DumpSpv;
    vi_bool l0OmitOriginalAddressCheck;
    BitRange captureDraws2DTexs;
    BitRange captureDraws;
    vi_bool captureDrawsPre;
    BitRange captureFinishFrame;
    BitRange captureReadPixels;
    BitRange captureFlushFrame;
    BitRange captureBindFboFrame;
    BitRange captureVulkanSubmits;
    BitRange captureVulkanSubmitsResources;
    VKCaptureGroupTypeOpt captureVulkanSubmitsGroupType;
    VulkanObjectRange captureVulkanRenderPasses;
    VulkanObjectRange captureVulkanRenderPassesResources;
    VulkanObjectRange captureVulkanDraws;
    VulkanObjectRange captureVulkanResources;
    vi_bool skipNonDeterministicImages;
    vi_bool precacheResources;
    vi_bool forceWindowPos;
    std::pair<int, int> windowCoords;
    vi_bool forceWindowSize;
    vi_bool forceInvisibleWindows;
    std::pair<int, int> windowSize;
    vi_bool forceScissor;
    std::vector<int> scissorCoords;
    vi_bool syncWithRecorder;
    BitRange keepDraws;
    BitRange keepApis;
    BitRange keepFrames;

    vi_bool traceGitsInternal;

    vi_uint endFrameSleep;

    vi_bool benchmark;
    vi_bool faithfulThreading;
    vi_bool nullRun;
    vi_bool waitForEnter;
    vi_bool linkGetProgBinary;
    vi_bool linkUseProgBinary;
    vi_bool diags;
    vi_uint forcePortableWglDepthBits;
    vi_bool affectViewport;
    std::vector<int> affectedViewport;
    vi_bool clCaptureReads;
    BitRange clCaptureKernels;
    BitRange l0CaptureKernels;
    BitRange l0CaptureCommandLists;
    BitRange l0CaptureCommandQueues;
    vi_bool l0DumpLayoutOnly;
    vi_bool l0CaptureAfterSubmit;
    vi_bool clOmitReadOnlyObjects;
    vi_bool clDumpLayoutOnly;
    vi_bool clInjectBufferResetAfterCreate;
    vi_bool l0InjectBufferResetAfterCreate;
    vi_bool l0DisableNullIndirectPointersInBuffer;
    vi_uint l0DisableAddressTranslation;
    vi_bool clDisableNullIndirectPointersInBuffer;
    vi_bool noOpenCL;
    vi_bool showWindowsWA;
    BitRange traceGLBufferHashes;
    vi_bool showOriginalPixelFormat;
    vi_bool forceNoMSAA;
    vi_bool cleanResourcesOnExit;
    vi_bool destroyContextsOnExit;
    vi_bool signStream;
    vi_bool verifyStream;
    vi_bool dontVerifyStream;
    vi_bool renderOffscreen;
    vi_bool ignoreVKCrossPlatformIncompatibilitiesWA;
    GLProfileOpt forceGLProfile;
    GLNativeApiOpt forceGLNativeAPI;
    vi_bool forceGLVersion;
    int forceGLVersionMinor;
    int forceGLVersionMajor;
    vi_bool forceWaylandWindow;
    vi_bool waitAfterQueueSubmitWA;
    vi_bool traceVKShaderHashes;
    std::vector<std::string> suppressVKDeviceFeatures;
    std::vector<std::string> suppressVKExtensions;
    std::vector<std::string> suppressVKLayers;
    vi_bool fullscreen;
    vi_bool forceDesktopResolution;
    std::vector<int> forcedDesktopResolution;
    uint32_t maxAllowedVkSwapchainRewinds;
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
#if defined(GITS_PLATFORM_WINDOWS)
    struct RenderDoc {
      vi_bool frameRecEnabled;
      vi_bool queuesubmitRecEnabled;
      BitRange captureRange;
      std::filesystem::path dllPath;
      vi_bool enableUI;
      vi_bool continuousCapture;
    } renderDoc;
#endif
  } player;

  struct Recorder {
    struct Basic {
      vi_bool enabled;
      vi_bool dumpGITS;
      vi_bool dumpCCode;
      vi_uint exitSignal;
      vi_uint exitAfterAPICall;
      std::vector<unsigned> exitKeys;
      struct Paths {
        vi_bool uniqueDumpDirectory;
        //rest of potential PATHs content is in common group temporarily
      } paths;
    } basic;

    struct OpenGL {
      struct Capture {
        std::string mode;
        struct All {
          vi_uint exitFrame;
          vi_int exitDeleteContext;
        } all;
        struct Frames {
          vi_int startFrame;
          vi_int stopFrame;
          std::vector<unsigned> startKeys;
          struct FrameSeparators {
            vi_bool glFinishSep;
            vi_bool glFlushSep;
          } frameSeparators;
        } frames;
        struct OglSingleDraw {
          vi_int number;
        } oglSingleDraw;
        struct OglDrawsRange {
          vi_int startDraw;
          vi_int stopDraw;
          vi_int frame;
        } oglDrawsRange;
      } capture;

      struct Utilities {
        std::string forceGLVersion;
        vi_uint forceGLVersionMajor;
        vi_uint forceGLVersionMinor;
        std::vector<std::string> suppressExtensions;
        vi_bool suppressProgramBinary;
        vi_uint endFrameSleep;
        vi_bool detectRecursion;
        vi_bool restoreDefaulFB;
        vi_bool restoreFBFrontAndBackWA;
        vi_bool doNotRemoveWin;
        vi_bool multiApiProtectBypass;
        BuffersStateOpt buffersState;
        TexturesStateOpt texturesState;
        vi_uint carrayMemCmpType;
        vi_uint stripIndicesValues;
        vi_int updateMappedTexturesEveryNSwaps;
        vi_bool eventsBeforeCall;
        vi_bool schedulefboEXTAsCoreWA;
        vi_bool useGlGetTexImageAndRestoreBuffersWhenPossibleES;
        vi_bool ccodeRangesWA;
        vi_bool optimizeBufferSize;
        vi_bool coherentMapUpdatePerFrame;
        vi_bool trackTextureBindingWA;
        vi_bool forceBuffersStateCaptureAlwaysWA;
        vi_bool restoreIndexedTexturesWA;
        vi_bool forceSyncFlushCommands;
        vi_bool forceFBOSupportWA;
        vi_bool retryFunctionLoads;
      } utilities;

      struct Performance {
        vi_bool benchmark;
      } performance;

      struct Images {
        BitRange dumpScreenshots;
        BitRange dumpDrawsFromFrames;
      } images;
    } openGL;

    struct OpenCL {
      struct Capture {
        std::string mode;
        struct OclSingleKernel {
          vi_uint number;
        } oclSingleKernel;
        struct OclKernelsRange {
          vi_uint startKernel;
          vi_uint stopKernel;
        } oclKernelsRange;
      } capture;

      struct Utilities {
        BitRange dumpKernels;
        vi_bool dumpImages;
        vi_bool omitReadOnlyObjects;
        vi_bool bufferResetAfterCreate;
        vi_bool nullIndirectPointersInBuffer;
      } utilities;
    } openCL;

    struct Vulkan {
      struct MemorySizeRequirementOverride {
        vi_uint fixedAmount;
        vi_uint percent;
      };

      struct Capture {
        std::string mode;
        struct All {
          vi_uint exitFrame;
        } all;
        struct Frames {
          vi_int startFrame;
          vi_int stopFrame;
          std::vector<unsigned> startKeys;
        } frames;
        struct Objects {
          VulkanObjectRange rangeSpecial;
        } objRange;
      } capture;

      struct Performance {
        vi_bool benchmark;
      } performance;

      struct Images {
        BitRange dumpScreenshots;
        BitRange dumpSubmits;
      } images;

      struct Utilities {
        vi_uint memorySegmentSize;
        vi_bool shadowMemory;
        vi_bool memoryAccessDetection;
        vi_bool forceUniversalRecording;
        vi_bool useExternalMemoryExtension;
        MemoryUpdateStateOpt memoryUpdateState;
        vi_uint delayFenceChecksCount;
        vi_uint shortenFenceWaitTime;
        std::vector<std::string> suppressExtensions;
        std::vector<std::string> suppressLayers;
        vi_uint addImageUsageFlags;
        vi_uint addBufferUsageFlags;
        vi_bool scheduleCommandBuffersBeforeQueueSubmit;
        MemorySizeRequirementOverride increaseImageMemorySizeRequirement;
        struct IncreaseAccelerationStructureMemorySizeRequirement {
          MemorySizeRequirementOverride accelerationStructureSize;
          MemorySizeRequirementOverride buildScratchSize;
          MemorySizeRequirementOverride updateScratchSize;
        } increaseAccelerationStructureMemorySizeRequirement;
        std::vector<std::string> suppressPhysicalDeviceFeatures;
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
        vi_bool minimalStateRestore;
        vi_uint reusableStateRestoreResourcesCount;
        vi_uint reusableStateRestoreBufferSize;
        vi_uint maxArraySizeForCCode;
        vi_bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
        vi_bool useCaptureReplayFeaturesForRayTracingPipelines;
        vi_bool renderDocCompatibility;
        std::vector<std::string> renderDocCompatibilitySuppressedExtensions = {
            "VK_EXT_graphics_pipeline_library", "VK_EXT_extended_dynamic_state3",
            "VK_EXT_external_memory_host", "VK_KHR_map_memory2"};
        vi_bool usePresentSrcLayoutTransitionAsAFrameBoundary;
      } utilities;
    } vulkan;

    struct LevelZero {
      struct Capture {
        std::string mode;
        struct Kernel {
          vi_uint startKernel;
          vi_uint stopKernel;
          vi_uint stopCommandList;
          vi_uint startCommandList;
          vi_uint stopCommandQueueSubmit;
          vi_uint startCommandQueueSubmit;
          vi_bool singleCapture;
        } kernel;
      } capture;
      struct Utilities {
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
      } utilities;
    } levelZero;

    struct Extras {
      struct Optimizations {
        HashTypeOpt hashType;
        struct PartialHashInfo {
          vi_bool enabled;
          vi_uint cutoff;
          vi_uint chunks;
          vi_uint ratio;
        } partialHash;
        vi_uint asyncBufferWrites;
        vi_uint bufferMapAccessMask;
        vi_uint bufferStorageFlagsMask;
        vi_bool removeResourceHash;
        struct Compression {
          CompressionTypeOpt type;
          vi_uint level;
          vi_uint chunkSize;
        } compression;
      } optimizations;

      struct Utilities {
        vi_bool extendedDiagnosticInfo;
        vi_bool zipTextFiles;
        vi_bool highIntegrity;
        vi_bool nullIO;
        vi_bool forceDumpOnError;
        vi_bool mtDriverWA;
        vi_bool disableThreadTracker;
        vi_bool coherentMapBehaviorWA;
        vi_bool closeAppOnStopRecording;
        vi_bool removeDXSharing;
        vi_bool removeGLSharing;
        WindowsKeyHandlingOpt windowsKeyHandling;
      } utilities;

    } extras;
  } recorder;

private:
  Config();
  static Config* config;
};

void GetConfigPtree(const std::filesystem::path& cfgPath, boost::property_tree::ptree& pt);

bool isTraceDataOptPresent(TraceData option);

template <typename T>
class CEnumParser {
  // To use it for your own enum just specialize the constructor. See the
  // existing specializations for more info: declarations are below this
  // class and definitions in the cpp file.
  //
  // The reason we don't use NamedValuesBase is because it uses statics.
  // Statics use the new operator very early when it might not yet work (in
  // Interceptor) if recorded app overrides new.
  std::map<std::string, T> _map;

public:
  CEnumParser(){};
  boost::optional<T> ParseEnum(const std::string& variantName) {
    boost::optional<T> retVal = T();

    auto iter = _map.find(gits::ToLowerCopy(variantName));
    if (iter != _map.end()) {
      retVal = iter->second;
    }

    return retVal;
  }
};
template <>
CEnumParser<LogLevel>::CEnumParser();

template <class T>
void ReadRecorderOption(const boost::property_tree::ptree& pt,
                        const char* name,
                        T& value,
                        unsigned supportedPlatforms,
                        bool hide = false) {
  if (!(supportedPlatforms & GITS_PLATFORM_BIT_CURRENT)) {
    return;
  }
  boost::optional<T> optVal = pt.get_optional<T>(name);
  if (optVal) {
    value = *optVal;
  } else {
    if (!hide) {
      Log(WARN) << "Couldn't find option: " << name
                << " in config file. Option will get a default value.";
    }
  }
}
template <>
void ReadRecorderOption<std::string>(const boost::property_tree::ptree& pt,
                                     const char* name,
                                     std::string& value,
                                     unsigned supportedPlatforms,
                                     bool hide);
template <>
void ReadRecorderOption<std::filesystem::path>(const boost::property_tree::ptree& pt,
                                               const char* name,
                                               std::filesystem::path& value,
                                               unsigned supportedPlatforms,
                                               bool hide);
template <>
void ReadRecorderOption<vi_uint>(const boost::property_tree::ptree& pt,
                                 const char* name,
                                 vi_uint& value,
                                 unsigned supportedPlatforms,
                                 bool hide);
template <>
void ReadRecorderOption<vi_bool>(const boost::property_tree::ptree& pt,
                                 const char* name,
                                 vi_bool& value,
                                 unsigned supportedPlatforms,
                                 bool hide);
template <>
void ReadRecorderOption<BitRange>(const boost::property_tree::ptree& pt,
                                  const char* name,
                                  BitRange& value,
                                  unsigned supportedPlatforms,
                                  bool hide);
template <>
void ReadRecorderOption<std::vector<std::string>>(const boost::property_tree::ptree& pt,
                                                  const char* name,
                                                  std::vector<std::string>& value,
                                                  unsigned supportedPlatforms,
                                                  bool hide);
} // namespace gits
