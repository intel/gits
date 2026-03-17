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
#include "log.h"
#include "yaml-cpp/yaml.h"

#include <string>
#include <fstream>

namespace gits {
namespace stream {

#ifndef VERSION_4
#define VERSION_4 999
#endif

StreamHeader StreamHeader::m_Instance;
const unsigned StreamHeader::VERSION[4] = {2, 0, 12, VERSION_4};
const unsigned StreamHeader::VERSION_API_INFO[4] = {2, 0, 11, 0};
const unsigned StreamHeader::SCHEDULER_VERSION = 1;

void StreamHeader::WriteHeader(std::ostream& stream) {
  WriteVersion(stream);
  unsigned skippedCallCount{};
  stream.write(reinterpret_cast<const char*>(&skippedCallCount), sizeof(skippedCallCount));

  WriteProperties(stream);

  Api api = Api::API_DIRECTX;
  stream.write(reinterpret_cast<const char*>(&api), sizeof(api));
  unsigned apiCompute{};
  stream.write(reinterpret_cast<const char*>(&apiCompute), sizeof(apiCompute));
  stream.write(reinterpret_cast<const char*>(&SCHEDULER_VERSION), sizeof(SCHEDULER_VERSION));

  CompressionType compressionType = Configurator::Get().common.recorder.compression.type;
  if (compressionType != CompressionType::LZ4 && compressionType != CompressionType::ZSTD) {
    LOG_ERROR << "Cannot use compression type from configuration, only LZ4 or ZSTD compression is "
                 "supported.";
    std::quick_exit(EXIT_FAILURE);
  }
  stream.write(reinterpret_cast<const char*>(&compressionType), sizeof(compressionType));

  uint64_t chunkSize{};
  stream.write(reinterpret_cast<const char*>(&chunkSize), sizeof(chunkSize));
}

void StreamHeader::ReadHeader(std::istream& stream) {
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

  if (versionToUint64(m_Version) >= versionToUint64(VERSION_API_INFO)) {
    stream.read(reinterpret_cast<char*>(&m_Api), sizeof(m_Api));
    unsigned apiCompute{};
    stream.read(reinterpret_cast<char*>(&apiCompute), sizeof(apiCompute));
    stream.read(reinterpret_cast<char*>(&m_SchedulerVersion), sizeof(m_SchedulerVersion));
  }
  stream.read(reinterpret_cast<char*>(&m_CompressionType), sizeof(m_CompressionType));
  stream.read(reinterpret_cast<char*>(&m_ChunkSize), sizeof(m_ChunkSize));
}

void StreamHeader::WriteVersion(std::ostream& stream) {
  const char MAGIC[] = "GITS_";
  stream.write(MAGIC, strlen(MAGIC));
  std::stringstream s;
  for (unsigned i = 0; i < 4; ++i) {
    if (i > 0) {
      s << '.';
    }
    s << std::setfill('0') << std::setw(i == 3 ? 3 : 2) << VERSION[i];
  }
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
  std::ifstream configFile(Configurator::Get().configFilePath);
  GITS_ASSERT(configFile.is_open());
  YAML::Node configYaml = YAML::Load(configFile);
  std::stringstream configStrStream;
  configStrStream << configYaml;
  m_Properties["diag"]["gits"]["config"] = configStrStream.str();

  std::string properties = m_Properties.dump();
  unsigned propertiesSize = properties.size();
  stream.write(reinterpret_cast<const char*>(&propertiesSize), sizeof(propertiesSize));
  stream.write(properties.c_str(), propertiesSize);
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

StreamHeader::Api StreamHeader::GetApi() const {
  return m_Api;
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
