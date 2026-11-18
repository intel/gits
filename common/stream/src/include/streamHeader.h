// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "enumsAuto.h"
#include "commandId.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>
#include "nlohmann/json.hpp"

namespace gits {
namespace stream {

class StreamHeader {
public:
  static StreamHeader& Get() {
    return m_Instance;
  }
  void ReadHeader(std::istream& stream);
  bool IsLegacyStream() const {
    return m_SchedulerVersion == 0;
  }
  std::string GetApplicationName();
  std::array<unsigned, 4> GetVersion() const;
  std::string GetPropertiesDump() const;
  std::optional<nlohmann::ordered_json> FindProperty(const std::string& keyPath) const;
  void WriteCapturedFrames(std::ofstream& stream, uint64_t capturedFrames);
  CompressionType GetCompressionType() const;

  enum class Api : unsigned {
    API_NOT_SET,
    API_OPENGL_LEGACY,
    API_VULKAN_LEGACY,
    API_OPENCL,
    API_LEVELZERO,
    API_DIRECTX,
    API_VULKAN,
    API_OPENGL,
  };
  // The API of the first command of the stream.
  Api GetApi() const;
  // The APIs of the commands of the stream, GetApi() first. An application using several APIs,
  // e.g. OpenGL with Vulkan interop, records the commands of all of them into one stream.
  std::vector<Api> GetApis() const;
  bool HasApi(Api api) const;
  bool HasMultipleApis() const;
  ApiBool GetApiBool() const;

private:
  friend class StreamWriter;
  void WriteHeader(std::ofstream& stream, CompressionType compressionType);
  // `firstApi` is the API of the first command, `apis` holds the ApiBit of every API recorded.
  void WriteApis(std::ofstream& stream, ApiId firstApi, uint32_t apis);
  // 0 for commands of no API.
  static uint32_t ApiBit(ApiId id);
  static uint32_t ApiBit(Api api);

private:
  void WriteVersion(std::ostream& stream);
  void ReadVersion(std::istream& stream);
  void WriteProperties(std::ostream& stream);
  static Api TranslateApi(ApiId id);

private:
  StreamHeader() {}
  static StreamHeader m_Instance;
  static const unsigned VERSION[4];
  unsigned m_Version[4]{};
  static const unsigned VERSION_API_INFO[4];
  static const unsigned VERSION_APIS[4];
  unsigned m_SchedulerVersion{};
  static const unsigned SCHEDULER_VERSION;
  CompressionType m_CompressionType{};
  uint64_t m_ChunkSize{};
  nlohmann::ordered_json m_Properties;
  Api m_Api{};
  uint32_t m_Apis{};
  std::fstream::pos_type m_ApiPosition{};
  std::fstream::pos_type m_ApisPosition{};
  std::fstream::pos_type m_CapturedFramesPosition{};
};

} // namespace stream
} // namespace gits
