// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"
#include "log.h"
#include "bit_range.h"

#include <string>
#include <cstring>
#include <map>
#include <optional>
#include <sstream>
#include <cassert>

namespace gits {
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
  std::optional<T> ParseEnum(const std::string& variantName) {
    std::optional<T> retVal = std::nullopt;

    auto iter = _map.find(ToLowerCopy(variantName));
    if (iter != _map.end()) {
      retVal = iter->second;
    }

    return retVal;
  }
};
template <>
inline CEnumParser<LogLevel>::CEnumParser() {
  if (_map.empty()) {
    _map["trace verbose"] = TRACEV;
    _map["traceverbose"] = TRACEV;
    _map["tracev"] = TRACEV;
    _map["trace"] = TRACE;
    _map["infov"] = INFOV;
    _map["info verbose"] = INFOV;
    _map["infoverbose"] = INFOV;
    _map["info"] = INFO;
    _map["warning"] = WARN;
    _map["warn"] = WARN;
    _map["error"] = ERR;
    _map["err"] = ERR;
    _map["off"] = OFF;
  }
}

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
    lhs << rhs.readable_name();
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

enum class TRecordingMode {
  NONE,
  BINARY,
  CCODE
};

struct RecordingModeOpt : NamedValuesBase<TRecordingMode, RecordingModeOpt> {
  static void describe_type() {
    readable_name() = "RecordingMode";
    default_value() = TRecordingMode::NONE;
    values()["None"] = TRecordingMode::NONE;
    values()["Binary"] = TRecordingMode::BINARY;
    values()["CCode"] = TRecordingMode::CCODE;
  }
};

enum class TOpenGLRecorderMode {
  ALL,
  FRAMES,
  SINGLE_DRAW,
  DRAWS_RANGE
};

struct OpenGLRecorderModeOpt : NamedValuesBase<TOpenGLRecorderMode, OpenGLRecorderModeOpt> {
  static void describe_type() {
    readable_name() = "OpenGLRecorderMode";
    default_value() = TOpenGLRecorderMode::ALL;
    values()["All"] = TOpenGLRecorderMode::ALL;
    values()["Frames"] = TOpenGLRecorderMode::FRAMES;
    values()["OglSingleDraw"] = TOpenGLRecorderMode::SINGLE_DRAW;
    values()["OglDrawsRange"] = TOpenGLRecorderMode::DRAWS_RANGE;
  }
};

enum class TVulkanRecorderMode {
  ALL,
  FRAMES,
  QUEUE_SUBMIT,
  COMMAND_BUFFERS_RANGE,
  RENDER_PASS_RANGE,
  DRAWS_RANGE,
  DISPATCH_RANGE,
  BLIT_RANGE
};

struct VulkanRecorderModeOpt : NamedValuesBase<TVulkanRecorderMode, VulkanRecorderModeOpt> {
  static void describe_type() {
    readable_name() = "VulkanRecorderMode";
    default_value() = TVulkanRecorderMode::ALL;
    values()["All"] = TVulkanRecorderMode::ALL;
    values()["Frames"] = TVulkanRecorderMode::FRAMES;
    values()["QueueSubmit"] = TVulkanRecorderMode::QUEUE_SUBMIT;
    values()["CommandBuffersRange"] = TVulkanRecorderMode::COMMAND_BUFFERS_RANGE;
    values()["RenderPassRange"] = TVulkanRecorderMode::RENDER_PASS_RANGE;
    values()["DrawsRange"] = TVulkanRecorderMode::DRAWS_RANGE;
    values()["DispatchRange"] = TVulkanRecorderMode::DISPATCH_RANGE;
    values()["BlitRange"] = TVulkanRecorderMode::BLIT_RANGE;
  }
};

enum class TOpenCLRecorderMode {
  ALL,
  SINGLE_KERNEL,
  KERNELS_RANGE
};

struct OpenCLRecorderModeOpt : NamedValuesBase<TOpenCLRecorderMode, OpenCLRecorderModeOpt> {
  static void describe_type() {
    readable_name() = "OpenCLRecorderMode";
    default_value() = TOpenCLRecorderMode::ALL;
    values()["All"] = TOpenCLRecorderMode::ALL;
    values()["OclSingleKernel"] = TOpenCLRecorderMode::SINGLE_KERNEL;
    values()["OclKernelsRange"] = TOpenCLRecorderMode::KERNELS_RANGE;
  }
};

enum class TLevelZeroRecorderMode {
  ALL,
  KERNEL
};

struct LevelZeroRecorderModeOpt
    : NamedValuesBase<TLevelZeroRecorderMode, LevelZeroRecorderModeOpt> {
  static void describe_type() {
    readable_name() = "LevelZeroRecorderMode";
    default_value() = TLevelZeroRecorderMode::ALL;
    values()["All"] = TLevelZeroRecorderMode::ALL;
    values()["Kernel"] = TLevelZeroRecorderMode::KERNEL;
  }
};

enum class TForcedGLProfile {
  NO_PROFILE_FORCED,
  COMPAT,
  CORE,
  ES
};

struct GLProfileOpt : NamedValuesBase<TForcedGLProfile, GLProfileOpt> {
  static void describe_type() {
    readable_name() = "Profile";
    default_value() = TForcedGLProfile::NO_PROFILE_FORCED;
    values()["NONE"] = TForcedGLProfile::NO_PROFILE_FORCED;
    values()["COMPAT"] = TForcedGLProfile::COMPAT;
    values()["CORE"] = TForcedGLProfile::CORE;
    values()["ES"] = TForcedGLProfile::ES;
  }
};

enum class TForcedGLNativeApi {
  NO_NTV_API_FORCED,
  EGL,
  GLX,
  WGL
};

struct GLNativeApiOpt : NamedValuesBase<TForcedGLNativeApi, GLNativeApiOpt> {
  static void describe_type() {
    readable_name() = "Native";
    default_value() = TForcedGLNativeApi::NO_NTV_API_FORCED;
    values()["NONE"] = TForcedGLNativeApi::NO_NTV_API_FORCED;
    values()["EGL"] = TForcedGLNativeApi::EGL;
    values()["WGL"] = TForcedGLNativeApi::WGL;
    values()["GLX"] = TForcedGLNativeApi::GLX;
  }
};

enum class TBuffersState {
  CAPTURE_ALWAYS,
  RESTORE,
  MIXED
};

struct BuffersStateOpt : NamedValuesBase<TBuffersState, BuffersStateOpt> {
  static void describe_type() {
    readable_name() = "BuffersState";
    default_value() = TBuffersState::CAPTURE_ALWAYS;
    values()["CaptureAlways"] = TBuffersState::CAPTURE_ALWAYS;
    values()["Restore"] = TBuffersState::RESTORE;
    values()["Mixed"] = TBuffersState::MIXED;
  }
};

enum class TTexturesState {
  CAPTURE_ALWAYS,
  RESTORE,
  MIXED
};

struct TexturesStateOpt : NamedValuesBase<TTexturesState, TexturesStateOpt> {
  static void describe_type() {
    readable_name() = "TexturesState";
    default_value() = TTexturesState::MIXED;
    values()["CaptureAlways"] = TTexturesState::CAPTURE_ALWAYS;
    values()["Restore"] = TTexturesState::RESTORE;
    values()["Mixed"] = TTexturesState::MIXED;
  }
};

enum class TMemoryUpdateStates {
  ALL_MAPPED,
  ONLY_USED,
  USING_TAGS
};

struct MemoryUpdateStateOpt : NamedValuesBase<TMemoryUpdateStates, MemoryUpdateStateOpt> {
  static void describe_type() {
    readable_name() = "MemoryUpdateState";
    default_value() = TMemoryUpdateStates::ONLY_USED;
    values()["AllMapped"] = TMemoryUpdateStates::ALL_MAPPED;
    values()["OnlyUsed"] = TMemoryUpdateStates::ONLY_USED;
    values()["UsingTags"] = TMemoryUpdateStates::USING_TAGS;
  }
};

enum class TMemoryTrackingMode {
#ifdef GITS_PLATFORM_WINDOWS
  EXTERNAL,
  WRITE_WATCH,
#endif
  SHADOW_AND_ACCESS_DETECTION,
  FULL_MEMORY_DUMP
};

struct MemoryTrackingModeOpt : NamedValuesBase<TMemoryTrackingMode, MemoryTrackingModeOpt> {
  static void describe_type() {
    readable_name() = "MemoryTrackingMode";
#ifdef GITS_PLATFORM_WINDOWS
    default_value() = TMemoryTrackingMode::EXTERNAL;
    values()["External"] = TMemoryTrackingMode::EXTERNAL;
    values()["WriteWatch"] = TMemoryTrackingMode::WRITE_WATCH;
#else
    default_value() = TMemoryTrackingMode::SHADOW_AND_ACCESS_DETECTION;
#endif
    values()["ShadowMemory"] = TMemoryTrackingMode::SHADOW_AND_ACCESS_DETECTION;
    values()["FullMemoryDump"] = TMemoryTrackingMode::FULL_MEMORY_DUMP;
  }
};

enum class TMemoryStateRestoration {
  NONE,
  HOST_VISIBLE
};

struct MemoryStateRestorationOpt
    : NamedValuesBase<TMemoryStateRestoration, MemoryStateRestorationOpt> {
  static void describe_type() {
    readable_name() = "MemoryRestoration";
    default_value() = TMemoryStateRestoration::HOST_VISIBLE;
    values()["None"] = TMemoryStateRestoration::NONE;
    values()["HostVisible"] = TMemoryStateRestoration::HOST_VISIBLE;
  }
};

enum class TBufferStateRestoration {
  NONE,
  WITH_NON_HOST_VISIBLE_MEMORY_ONLY,
  ALL
};

struct BufferStateRestorationOpt
    : NamedValuesBase<TBufferStateRestoration, BufferStateRestorationOpt> {
  static void describe_type() {
    readable_name() = "Buffers";
    default_value() = TBufferStateRestoration::WITH_NON_HOST_VISIBLE_MEMORY_ONLY;
    values()["None"] = TBufferStateRestoration::NONE;
    values()["WithNonHostVisibleMemoryOnly"] =
        TBufferStateRestoration::WITH_NON_HOST_VISIBLE_MEMORY_ONLY;
    values()["All"] = TBufferStateRestoration::ALL;
  }
};

enum class TWindowsKeyHandling {
  MESSAGE_LOOP,
  ASYNC_KEY_STATE
};

struct WindowsKeyHandlingOpt : NamedValuesBase<TWindowsKeyHandling, WindowsKeyHandlingOpt> {
  static void describe_type() {
    readable_name() = "WindowsKeyHandling";
    default_value() = TWindowsKeyHandling::MESSAGE_LOOP;
    values()["MessageLoop"] = TWindowsKeyHandling::MESSAGE_LOOP;
    values()["AsyncKeyState"] = TWindowsKeyHandling::ASYNC_KEY_STATE;
  }
};

enum class TCaptureGroupType {
  PER_COMMAND_BUFFER,
  PER_RENDER_PASS
};

struct VKCaptureGroupTypeOpt : NamedValuesBase<TCaptureGroupType, VKCaptureGroupTypeOpt> {
  static void describe_type() {
    readable_name() = "VKCaptureGroupType";
    default_value() = TCaptureGroupType::PER_COMMAND_BUFFER;
    values()["CmdBuffer"] = TCaptureGroupType::PER_COMMAND_BUFFER;
    values()["RenderPass"] = TCaptureGroupType::PER_RENDER_PASS;
  }
};

enum class TDeviceType {
  ANY,
  INTEGRATED,
  DISCRETE
};

struct DeviceTypeOpt : NamedValuesBase<TDeviceType, DeviceTypeOpt> {
  static void describe_type() {
    readable_name() = "DeviceType";
    default_value() = TDeviceType::ANY;
    values()["any"] = TDeviceType::ANY;
    values()["integrated"] = TDeviceType::INTEGRATED;
    values()["discrete"] = TDeviceType::DISCRETE;
  }
};

enum class TVkRenderDocCaptureMode {
  NONE,
  FRAMES,
  QUEUE_SUBMIT
};

struct VkRenderDocCaptureOpt : NamedValuesBase<TVkRenderDocCaptureMode, VkRenderDocCaptureOpt> {
  static void describe_type() {
    readable_name() = "VkRenderDocCapture";
    default_value() = TVkRenderDocCaptureMode::NONE;
    values()["None"] = TVkRenderDocCaptureMode::NONE;
    values()["Frames"] = TVkRenderDocCaptureMode::FRAMES;
    values()["QueueSubmit"] = TVkRenderDocCaptureMode::QUEUE_SUBMIT;
  }
};

enum class THashType {
  MURMUR,
  XX,
  INCREMENTAL_NUMBER,
  CRC32ISH,
  XXCRC32
};
enum CompressionType : uint8_t {
  NONE,
  LZ4,
  ZSTD
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
// A lot of values in config are left untouched at 'default' zero-like
// value. Following will allow to enumerate only defaults that are
// special.
template <class T>
class value_initialized {
public:
  value_initialized() : x() {}
  explicit value_initialized(const T& val) : x(val) {}
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

struct MemorySizeRequirementOverride {
  vi_uint fixedAmount;
  vi_uint percent;
};

struct ObjectRange {
  std::vector<uint32_t> objVector;
  BitRange range;
  bool empty() const {
    return range.empty();
  }
};

enum VulkanObjectMode {
  MODE_VK_NONE,
  MODE_VK_QUEUE_SUBMIT,
  MODE_VK_COMMAND_BUFFER,
  MODE_VK_RENDER_PASS,
  MODE_VK_DRAW,
  MODE_VK_DISPATCH,
  MODE_VK_BLIT
};
struct VulkanObjectRange : ObjectRange {
  VulkanObjectMode objMode;

  bool operator[](uint64_t queueSubmitNumber) const {
    if (objMode == MODE_VK_QUEUE_SUBMIT) {
      return range[(size_t)queueSubmitNumber];
    } else if (objMode == MODE_VK_COMMAND_BUFFER || objMode == MODE_VK_RENDER_PASS ||
               objMode == MODE_VK_DRAW || objMode == MODE_VK_BLIT || objMode == MODE_VK_DISPATCH) {
      return objVector[0] == queueSubmitNumber;
    } else {
      return false;
    }
  }

  void SetFromString(const std::string& str) {
    if (str.empty()) {
      return;
    }
    std::istringstream issVulkanObjects(str);
    std::vector<std::string> resourceTable;

    std::string strObj;
    while (std::getline(issVulkanObjects, strObj, '/')) {
      resourceTable.push_back(strObj);
    }
    range = BitRange(resourceTable.back());
    resourceTable.pop_back();
    for (const auto& obj : resourceTable) {
      objVector.push_back(std::stoul(obj, nullptr, 0));
    }
  }
};

enum class WindowMode {
  NORMAL,
  EXCLUSIVE_FULLSCREEN,
};

} // namespace gits
