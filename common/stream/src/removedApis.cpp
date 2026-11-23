// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "removedApis.h"
#include "streamHeader.h"
#include "streamLegacyReader.h"
#include "streamCompressor.h"
#include "commandFactory.h"
#include "commandId.h"

#include <cstring>
#include <fstream>
#include <memory>
#include <vector>

namespace gits {
namespace stream {

namespace {

class ApiExtractor : public CommandFactory {
public:
  void Initialize(StreamLegacyReader* streamReader) {
    m_StreamReader = streamReader;
  }
  CommandRunner* CreateCommand(unsigned id) override {
    m_ApiId = ExtractApiIdentifier(id);
    if (m_ApiId != ApiId::ID_COMMON) {
      m_StreamReader->Close();
    }
    return nullptr;
  }
  ApiId GetApi() const {
    return m_ApiId;
  }

private:
  StreamLegacyReader* m_StreamReader{};
  ApiId m_ApiId{};
};

bool IsRemovedApiStream(const std::filesystem::path& streamPath) {
  std::ifstream stream(streamPath, std::ios::in | std::ios::binary);
  StreamHeader& header = StreamHeader::Get();
  header.ReadHeader(stream);

  if (header.GetApi() == StreamHeader::Api::API_NOT_SET) {
    if (header.GetCompressionType() != CompressionType::LZ4) {
      return true;
    }
    ApiExtractor apiExtractor;
    std::vector<CommandFactory*> commandFactories;
    commandFactories.push_back(&apiExtractor);
    StreamLegacyReader reader(commandFactories, stream);
    apiExtractor.Initialize(&reader);
    reader.Run();
    return apiExtractor.GetApi() != ApiId::ID_DIRECTX;
  }
  return header.GetApi() != StreamHeader::Api::API_DIRECTX &&
         header.GetApi() != StreamHeader::Api::API_VULKAN &&
         header.GetApi() != StreamHeader::Api::API_OPENGL;
}

const char* RemovedApiName(StreamHeader::Api api) {
  switch (api) {
  case StreamHeader::Api::API_OPENGL_LEGACY:
    return "OpenGLLegacy";
  case StreamHeader::Api::API_VULKAN_LEGACY:
    return "VulkanLegacy";
  case StreamHeader::Api::API_OPENCL:
    return "OpenCL";
  case StreamHeader::Api::API_LEVELZERO:
    return "Level Zero";
  default:
    return nullptr;
  }
}

const char* RemovedApiName(ApiId apiId) {
  switch (apiId) {
  case ApiId::ID_OPENGL_LEGACY:
  case ApiId::ID_GL_HELPER_TOKENS:
  case ApiId::ID_WGL:
  case ApiId::ID_GLX:
  case ApiId::ID_EGL:
    return "OpenGLLegacy";
  case ApiId::ID_OPENCL:
    return "OpenCL";
  case ApiId::ID_VULKAN_LEGACY:
    return "VulkanLegacy";
  case ApiId::ID_LEVELZERO:
    return "Level Zero";
  case ApiId::ID_OCLOC:
    return "ocloc";
  default:
    return nullptr;
  }
}

constexpr uint64_t MakeVersion(uint64_t major, uint64_t minor, uint64_t patch) {
  return major << 48 | minor << 32 | patch << 16;
}

// Versions of the legacy stream format.
constexpr uint64_t VERSION_TOKEN_COMPRESSION = MakeVersion(2, 0, 10);
constexpr uint64_t VERSION_API_INFO = MakeVersion(2, 0, 11);
// Reads the token data at the beginning of a legacy stream, decompressing its first chunk.
std::vector<char> ReadLegacyTokenData(std::istream& stream, uint64_t version) {
  constexpr uint64_t MAX_DATA_SIZE = 1024 * 1024 * 1024;
  std::vector<char> data;
  if (version >= VERSION_TOKEN_COMPRESSION) {
    CompressionType compressionType{};
    uint64_t chunkSize{};
    stream.read(reinterpret_cast<char*>(&compressionType), sizeof(compressionType));
    stream.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
    if (compressionType != CompressionType::NONE) {
      uint64_t size{};
      uint8_t writeType{};
      uint64_t compressedSize{};
      stream.read(reinterpret_cast<char*>(&size), sizeof(size));
      stream.read(reinterpret_cast<char*>(&writeType), sizeof(writeType));
      stream.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
      constexpr uint8_t WRITE_TYPE_PACKAGE = 1;
      if (!stream || writeType != WRITE_TYPE_PACKAGE || size > MAX_DATA_SIZE ||
          compressedSize > MAX_DATA_SIZE) {
        return {};
      }
      std::vector<char> compressedData(compressedSize);
      stream.read(compressedData.data(), compressedSize);
      std::unique_ptr<StreamDecompressor> decompressor;
      if (compressionType == CompressionType::LZ4) {
        decompressor = std::make_unique<LZ4StreamDecompressor>();
      } else {
        decompressor = std::make_unique<ZSTDStreamDecompressor>();
      }
      data.resize(size);
      const uint64_t decompressedSize = decompressor->Decompress(compressedData.data(), data.data(),
                                                                 stream.gcount(), data.size());
      data.resize(decompressedSize <= size ? decompressedSize : 0);
      return data;
    }
  }
  data.resize(4096);
  stream.read(data.data(), data.size());
  data.resize(stream.gcount());
  return data;
}

// Returns the API of the first token in the data that doesn't belong to the GITS common tokens,
// or ID_COMMON if there is none.
ApiId FindFirstApiToken(const std::vector<char>& data) {
  size_t offset = 0;
  while (offset + sizeof(uint8_t) + sizeof(uint16_t) <= data.size()) {
    const auto apiId = static_cast<uint8_t>(data[offset]);
    uint16_t tokenId{};
    std::memcpy(&tokenId, data.data() + offset + sizeof(apiId), sizeof(tokenId));
    offset += sizeof(apiId) + sizeof(tokenId);
    if (apiId != static_cast<uint8_t>(ApiId::ID_COMMON)) {
      return static_cast<ApiId>(apiId);
    }
    // Common tokens are markers without data, except for these, which have 8 bytes of data.
    constexpr uint16_t ID_PLAYER_RECORDER_SYNC = 7;
    constexpr uint16_t ID_SCREEN_RESOLUTION = 9;
    if (tokenId == ID_PLAYER_RECORDER_SYNC || tokenId == ID_SCREEN_RESOLUTION ||
        tokenId == static_cast<uint16_t>(CommonCommandId::ID_MARKER_UINT64)) {
      offset += sizeof(uint64_t);
    }
  }
  return ApiId::ID_COMMON;
}

// Returns the name of the removed API a legacy stream was recorded with. Expects the header of
// the stream to be read into StreamHeader::Get().
const char* FindRemovedApi(const std::filesystem::path& streamPath) {
  const StreamHeader& header = StreamHeader::Get();
  if (const char* name = RemovedApiName(header.GetApi())) {
    return name;
  }

  const auto versionArray = header.GetVersion();
  const uint64_t version = MakeVersion(versionArray[0], versionArray[1], versionArray[2]);
  std::ifstream stream(streamPath, std::ios::in | std::ios::binary);
  constexpr unsigned VERSION_SIZE = 17;
  stream.ignore(VERSION_SIZE);
  unsigned skippedCallCount{};
  stream.read(reinterpret_cast<char*>(&skippedCallCount), sizeof(skippedCallCount));
  stream.ignore(static_cast<std::streamsize>(skippedCallCount) * 2 * sizeof(unsigned));
  unsigned propertiesSize{};
  stream.read(reinterpret_cast<char*>(&propertiesSize), sizeof(propertiesSize));
  stream.ignore(propertiesSize);
  if (version >= VERSION_API_INFO) {
    unsigned api3D{};
    unsigned apiCompute{};
    unsigned schedulerVersion{};
    stream.read(reinterpret_cast<char*>(&api3D), sizeof(api3D));
    stream.read(reinterpret_cast<char*>(&apiCompute), sizeof(apiCompute));
    stream.read(reinterpret_cast<char*>(&schedulerVersion), sizeof(schedulerVersion));
    if (const char* name = RemovedApiName(static_cast<StreamHeader::Api>(apiCompute))) {
      return name;
    }
  }
  if (!stream) {
    return nullptr;
  }
  return RemovedApiName(FindFirstApiToken(ReadLegacyTokenData(stream, version)));
}

} // namespace

std::optional<std::string> FindRemovedApiError(const std::filesystem::path& streamPath) {
  if (!IsRemovedApiStream(streamPath)) {
    return std::nullopt;
  }
  const char* removedApi = FindRemovedApi(streamPath);
  if (!removedApi) {
    return "The stream format is no longer supported by GITS. Streams recorded with "
           "OpenGLLegacy, VulkanLegacy, OpenCL, Level Zero or ocloc can only be replayed with an "
           "older GITS release.";
  }
  return std::string("The stream was recorded with ") + removedApi +
         ", which is no longer supported by GITS. Replay it with an older GITS release that "
         "supports " +
         removedApi + ".";
}

} // namespace stream
} // namespace gits
