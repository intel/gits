// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "enumsAuto.h"

#include <iostream>
#include <memory>
#include "nlohmann/json.hpp"

namespace gits {
namespace stream {

class StreamHeader {
public:
  static StreamHeader& Get() {
    return m_Instance;
  }
  void WriteHeader(std::ostream& stream);
  void ReadHeader(std::istream& stream);
  bool IsLegacyStream() const {
    return m_SchedulerVersion == 0;
  }
  std::string GetApplicationName();
  std::string GetPropertiesDump() const;
  std::optional<nlohmann::ordered_json> FindProperty(const std::string& keyPath) const;
  CompressionType GetCompressionType() const;

  enum class Api : unsigned {
    API_NOT_SET,
    API_OPENGL,
    API_VULKAN,
    API_OPENCL,
    API_LEVELZERO,
    API_DIRECTX
  };
  Api GetApi() const;

private:
  void WriteVersion(std::ostream& stream);
  void ReadVersion(std::istream& stream);
  void WriteProperties(std::ostream& stream);

private:
  StreamHeader() {}
  static StreamHeader m_Instance;
  static const unsigned VERSION[4];
  unsigned m_Version[4]{};
  static const unsigned VERSION_API_INFO[4];
  unsigned m_SchedulerVersion{};
  static const unsigned SCHEDULER_VERSION;
  CompressionType m_CompressionType{};
  uint64_t m_ChunkSize{};
  nlohmann::ordered_json m_Properties;
  Api m_Api{};
};

} // namespace stream
} // namespace gits
