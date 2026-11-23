// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamHeader.h"
#include "diagnostic.h"
#include "configurator.h"
#include "exception.h"
#include "log.h"
#include "version.h"
#include "yaml-cpp/yaml.h"

#include <bit>
#include <string>
#include <limits>

namespace gits {
namespace stream {

StreamHeader StreamHeader::m_Instance;
const unsigned StreamHeader::VERSION_API_INFO[4] = {2, 0, 11, 0};
const unsigned StreamHeader::VERSION_APIS[4] = {2, 0, 14, 0};
const unsigned StreamHeader::SCHEDULER_VERSION = 1;

void StreamHeader::WriteHeader(std::ofstream& stream, CompressionType compressionType) {
  WriteVersion(stream);
  unsigned skippedCallCount{};
  stream.write(reinterpret_cast<const char*>(&skippedCallCount), sizeof(skippedCallCount));

  WriteProperties(stream);

  Api api = Api::API_NOT_SET;
  m_ApiPosition = stream.tellp();
  stream.write(reinterpret_cast<const char*>(&api), sizeof(api));
  unsigned apiCompute{};
  stream.write(reinterpret_cast<const char*>(&apiCompute), sizeof(apiCompute));
  uint32_t apis{};
  m_ApisPosition = stream.tellp();
  stream.write(reinterpret_cast<const char*>(&apis), sizeof(apis));
  stream.write(reinterpret_cast<const char*>(&SCHEDULER_VERSION), sizeof(SCHEDULER_VERSION));

  if (compressionType != CompressionType::LZ4 && compressionType != CompressionType::ZSTD) {
    LOG_ERROR << "Cannot use compression type from configuration, only LZ4 or ZSTD compression is "
                 "supported.";
    std::quick_exit(EXIT_FAILURE);
  }
  stream.write(reinterpret_cast<const char*>(&compressionType), sizeof(compressionType));

  uint64_t chunkSize{};
  stream.write(reinterpret_cast<const char*>(&chunkSize), sizeof(chunkSize));
}

void StreamHeader::WriteApis(std::ofstream& stream, ApiId firstApi, uint32_t apis) {
  std::fstream::pos_type currentPosition = stream.tellp();
  stream.seekp(m_ApiPosition);
  Api api = TranslateApi(firstApi);
  stream.write(reinterpret_cast<const char*>(&api), sizeof(api));
  stream.seekp(m_ApisPosition);
  stream.write(reinterpret_cast<const char*>(&apis), sizeof(apis));
  stream.seekp(currentPosition);
}

uint32_t StreamHeader::ApiBit(ApiId id) {
  return ApiBit(TranslateApi(id));
}

uint32_t StreamHeader::ApiBit(Api api) {
  return api == Api::API_NOT_SET ? 0 : 1u << static_cast<unsigned>(api);
}

void StreamHeader::ReadHeader(std::istream& stream) {
  m_Version[0] = m_Version[1] = m_Version[2] = m_Version[3] = 0;
  m_SchedulerVersion = 0;
  m_CompressionType = {};
  m_ChunkSize = 0;
  m_Properties = nlohmann::ordered_json();
  m_Api = Api::API_NOT_SET;

  ReadVersion(stream);

  unsigned skippedCallCount{};
  stream.read(reinterpret_cast<char*>(&skippedCallCount), sizeof(skippedCallCount));
  for (unsigned i = 0; i < skippedCallCount; ++i) {
    unsigned id{};
    stream.read(reinterpret_cast<char*>(&id), sizeof(id));
    unsigned num{};
    stream.read(reinterpret_cast<char*>(&num), sizeof(num));
  }

  unsigned propertiesSize{};
  stream.read(reinterpret_cast<char*>(&propertiesSize), sizeof(propertiesSize));
  if (propertiesSize) {
    std::string properties(propertiesSize, '\0');
    stream.read(properties.data(), propertiesSize);
    if (properties.find("<?xml version") == std::string::npos) {
      nlohmann::ordered_json json = nlohmann::ordered_json::parse(properties, nullptr, false);
      if (!json.is_discarded()) {
        m_Properties = std::move(json);
      }
    }
  }

  auto versionToUint64 = [](const unsigned v[4]) {
    return (static_cast<uint64_t>(v[0]) << 48 | static_cast<uint64_t>(v[1]) << 32 |
            static_cast<uint64_t>(v[2]) << 16 | static_cast<uint64_t>(v[3]));
  };

  m_Apis = 0;
  if (versionToUint64(m_Version) >= versionToUint64(VERSION_API_INFO)) {
    stream.read(reinterpret_cast<char*>(&m_Api), sizeof(m_Api));
    unsigned apiCompute{};
    stream.read(reinterpret_cast<char*>(&apiCompute), sizeof(apiCompute));
    if (versionToUint64(m_Version) >= versionToUint64(VERSION_APIS)) {
      stream.read(reinterpret_cast<char*>(&m_Apis), sizeof(m_Apis));
    }
    stream.read(reinterpret_cast<char*>(&m_SchedulerVersion), sizeof(m_SchedulerVersion));
    m_Apis |= ApiBit(m_Api);
  }
  stream.read(reinterpret_cast<char*>(&m_CompressionType), sizeof(m_CompressionType));
  stream.read(reinterpret_cast<char*>(&m_ChunkSize), sizeof(m_ChunkSize));
}

void StreamHeader::WriteVersion(std::ostream& stream) {
  std::stringstream s;
  s << CurrentVersion();
  std::string str = s.str();
  stream.write(str.data(), str.size());
}

void StreamHeader::ReadVersion(std::istream& stream) {
  const char MAGIC[] = "GITS_";
  char magic[sizeof(MAGIC) - 1];
  stream.read(magic, sizeof(magic));
  if (!std::equal(magic, magic + sizeof(magic), MAGIC)) {
    throw std::runtime_error("Unrecognized version format");
  }

  for (size_t i = 0; i < 4; i++) {
    unsigned strSize = i == 3 ? 3 : 2;
    std::string str;
    str.resize(strSize);
    stream.read(str.data(), strSize);
    m_Version[i] = std::stoi(str);
    if (i < 3) {
      char separator{};
      stream.read(&separator, 1);
      if (separator != '.') {
        throw std::runtime_error("Unrecognized version format");
      }
    }
  }
}

void StreamHeader::WriteProperties(std::ostream& stream) {

  gits::gather_diagnostic_info(m_Properties);
  m_Properties["diag"]["gits"]["config_path"] = Configurator::Get().configFilePath;
  YAML::Emitter configYaml;
  if (!Configurator::Emit(configYaml, Configurator::Get(), false)) {
    LOG_ERROR << "Failed to serialize the configuration for the stream metadata";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  m_Properties["diag"]["gits"]["config"] = std::string(configYaml.c_str());
  m_Properties["diag"]["gits"]["captured_frames"] = std::numeric_limits<uint64_t>::max();

  std::string properties = m_Properties.dump();
  unsigned propertiesSize = properties.size();
  stream.write(reinterpret_cast<const char*>(&propertiesSize), sizeof(propertiesSize));
  const std::string capturedFramesProperty = "\"captured_frames\":";
  const auto capturedFramesOffset = properties.find(capturedFramesProperty);
  if (capturedFramesOffset == std::string::npos) {
    throw EOperationFailed("Captured frames metadata property was not serialized");
  }
  m_CapturedFramesPosition =
      stream.tellp() +
      static_cast<std::streamoff>(capturedFramesOffset + capturedFramesProperty.size());
  stream.write(properties.c_str(), propertiesSize);
}

void StreamHeader::WriteCapturedFrames(std::ofstream& stream, uint64_t capturedFrames) {
  const auto currentPosition = stream.tellp();
  const std::string value = std::to_string(capturedFrames);
  stream.seekp(m_CapturedFramesPosition);
  stream.write(value.data(), value.size());
  const std::string padding(20 - value.size(), ' ');
  stream.write(padding.data(), padding.size());
  stream.seekp(currentPosition);
}

std::string StreamHeader::GetApplicationName() {
  std::string appName;

  auto diag = m_Properties.find("diag");
  if (diag != m_Properties.end()) {
    auto app = diag->find("original_app");
    if (app == diag->end()) {
      app = diag->find("app");
    }
    if (app != diag->end()) {
      auto name = app->find("name");
      if (name != app->end() && name->is_string()) {
        appName = name->get<std::string>();
      }
    }
  }
  return appName;
}

std::array<unsigned, 4> StreamHeader::GetVersion() const {
  return {m_Version[0], m_Version[1], m_Version[2], m_Version[3]};
}

StreamHeader::Api StreamHeader::TranslateApi(ApiId id) {
  switch (id) {
  case ApiId::ID_COMMON:
    return Api::API_NOT_SET;
  case ApiId::ID_OPENGL_LEGACY:
  case ApiId::ID_GL_HELPER_TOKENS:
  case ApiId::ID_WGL:
  case ApiId::ID_GLX:
  case ApiId::ID_EGL:
    return Api::API_OPENGL_LEGACY;
  case ApiId::ID_OPENGL:
    return Api::API_OPENGL;
  case ApiId::ID_OPENCL:
    return Api::API_OPENCL;
  case ApiId::ID_VULKAN_LEGACY:
    return Api::API_VULKAN_LEGACY;
  case ApiId::ID_LEVELZERO:
  case ApiId::ID_OCLOC:
    return Api::API_LEVELZERO;
  case ApiId::ID_DIRECTX:
    return Api::API_DIRECTX;
  case ApiId::ID_VULKAN:
    return Api::API_VULKAN;
  default:
    return Api::API_NOT_SET;
  }
}

StreamHeader::Api StreamHeader::GetApi() const {
  return m_Api;
}

std::vector<StreamHeader::Api> StreamHeader::GetApis() const {
  std::vector<Api> apis;
  if (m_Api != Api::API_NOT_SET) {
    apis.push_back(m_Api);
  }
  for (unsigned value = 1; value < 32; ++value) {
    const Api api = static_cast<Api>(value);
    if (api != m_Api && (m_Apis & ApiBit(api))) {
      apis.push_back(api);
    }
  }
  return apis;
}

bool StreamHeader::HasApi(Api api) const {
  return (m_Apis & ApiBit(api)) != 0;
}

bool StreamHeader::HasMultipleApis() const {
  return std::popcount(m_Apis) > 1;
}

ApiBool StreamHeader::GetApiBool() const {
  switch (m_Api) {
  case Api::API_OPENGL:
    return ApiBool::OGL;
  case Api::API_VULKAN:
    return ApiBool::VK;
  case Api::API_DIRECTX:
    return ApiBool::DX;
  default:
    return ApiBool::NOT_SET;
  }
}

CompressionType StreamHeader::GetCompressionType() const {
  return m_CompressionType;
}

std::string StreamHeader::GetPropertiesDump() const {
  return m_Properties.dump(2);
}

std::optional<nlohmann::ordered_json> StreamHeader::FindProperty(const std::string& keyPath) const {
  std::istringstream keyStream(keyPath);
  std::string key;
  const nlohmann::ordered_json* current = &m_Properties;

  while (std::getline(keyStream, key, '.')) {
    if (current->contains(key)) {
      current = &(*current)[key];
    } else {
      return std::nullopt;
    }
  }

  return *current;
}

} // namespace stream
} // namespace gits
