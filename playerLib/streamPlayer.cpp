// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamPlayer.h"
#include "configurationLib.h"
#include "gits.h"
#include "log.h"
#include "messageBus.h"
#include "streamReader.h"
#include "streamLegacyReader.h"
#include "streamHeader.h"
#include "streamCompressor.h"
#include "commandFactory.h"
#include "commandId.h"
#include "commandRunner.h"
#include "timer.h"
#include "remotePlaybackController.h"

#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#if defined GITS_PLATFORM_WINDOWS_X64
#if defined WITH_DIRECTX
#include "directXCommandFactory.h"
#endif
#endif
#if defined WITH_VULKAN
#include "vulkanCommandFactory.h"
#endif
#if defined WITH_OPENGL
#include "openGLCommandFactory.h"
#endif
#include "windowManager.h"

namespace gits {

namespace {

template <typename Duration>
std::string FormatDuration(Duration duration) {
  using namespace std::chrono;

  auto hrs = duration_cast<hours>(duration).count();
  auto mins = duration_cast<minutes>(duration).count() % 60;
  auto secs = duration_cast<seconds>(duration).count() % 60;
  auto msecs = duration_cast<milliseconds>(duration).count() % 1000;

  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << hrs << ":" << std::setfill('0') << std::setw(2)
      << mins << ":" << std::setfill('0') << std::setw(2) << secs << "." << std::setfill('0')
      << std::setw(3) << msecs;
  return oss.str();
}

} // namespace

class MessageLoop {
public:
  MessageLoop(stream::BaseStreamReader* streamReader, Timer* playbackTimer)
      : m_StreamReader(streamReader), m_PlaybackTimer(playbackTimer) {
    m_Interactive = Configurator::Get().common.player.interactive;
    m_StopAfterFrames = Configurator::Get().common.player.stopAfterFrames;
    if (m_Interactive) {
      m_RemoteControl = std::make_unique<RemotePlaybackController>(
          false, true,
          [this](RemotePlaybackController::PlayerState state) {
            if (state == RemotePlaybackController::PlayerState::Finished) {
              m_StopRequested = true;
            } else {
              m_Paused = state == RemotePlaybackController::PlayerState::Paused;
            }
          },
          [this] { return m_StopRequested; });
    }
  }

  void SetStreamReader(stream::BaseStreamReader* streamReader) {
    m_StreamReader = streamReader;
  }

  void NotifyPlaybackDone() {
    if (m_RemoteControl) {
      m_RemoteControl->NotifyPlaybackDone();
    }
  }

  void RunLoop(unsigned frame) {
    if (Configurator::Get().common.player.showWindowBorder &&
        Configurator::Get().common.player.showFrameNumberInTitle) {
      windowing::WindowManager::Get().SetTitle("Current frame: " + std::to_string(frame));
    }

    if (m_RemoteControl) {
      m_RemoteControl->SendFramePlayed(frame);
    }
    if (m_RemoteControl && m_RemoteControl->HasRemoteControl()) {
      m_Paused = m_RemoteControl->ShouldPauseAfterFrame(frame);
    } else if (m_Interactive) {
      m_Paused = true;
    }
    if (m_StopAfterFrames[frame]) {
      m_Paused = true;
      if (m_RemoteControl && m_RemoteControl->HasRemoteControl()) {
        m_RemoteControl->Pause();
      }
    }

    do {
      if (m_RemoteControl) {
        m_RemoteControl->ProcessPendingCommands();
      }
      if (m_StopRequested) {
        m_StreamReader->Close();
        m_Paused = false;
        break;
      }
      auto events = windowing::WindowManager::Get().PollEvents();
      for (auto event : events) {
        switch (event) {
        case windowing::WindowEvent::Close:
        case windowing::WindowEvent::Stop:
          m_StreamReader->Close();
          m_Paused = false;
          m_Interactive = false;
          m_StopRequested = true;
          break;
        case windowing::WindowEvent::TogglePause:
          if (m_RemoteControl) {
            m_RemoteControl->UseLocalControl();
          }
          m_Paused = !m_Paused;
          if (m_Paused) {
            m_PlaybackTimer->Pause();
          } else {
            m_PlaybackTimer->Resume();
          }
          break;
        case windowing::WindowEvent::ToggleInteractive:
          if (m_RemoteControl) {
            m_RemoteControl->UseLocalControl();
          }
          m_Interactive = !m_Interactive;
          if (!m_Interactive) {
            m_Paused = false;
            m_PlaybackTimer->Resume();
          }
          break;
        }
      }
      if (m_StopRequested) {
        m_Paused = false;
      }
      if (m_Paused) {
        m_PlaybackTimer->Pause();
      } else if (!m_StopRequested) {
        m_PlaybackTimer->Resume();
      }
      if (m_RemoteControl) {
        using State = RemotePlaybackController::PlayerState;
        m_RemoteControl->ReportState(m_StopRequested ? State::Finished
                                     : m_Paused      ? State::Paused
                                                     : State::Running);
      }
      if (m_Paused) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    } while (m_Paused);
  }

private:
  stream::BaseStreamReader* m_StreamReader{};
  Timer* m_PlaybackTimer{};
  bool m_Paused{};
  bool m_Interactive{};
  BitRange m_StopAfterFrames;
  bool m_StopRequested{};
  std::unique_ptr<RemotePlaybackController> m_RemoteControl;
};

class StateRestoreBeginRunner : public stream::CommandRunner {
public:
  StateRestoreBeginRunner(Timer* stateRestoreTimer) : m_StateRestoreTimer(stateRestoreTimer) {}
  void Run() override {
    LOG_INFO << "State restore started";
    m_StateRestoreTimer->Start();
  }

private:
  Timer* m_StateRestoreTimer{};
};

class StateRestoreEndRunner : public stream::CommandRunner {
public:
  StateRestoreEndRunner(MessageLoop* messageLoop, Timer* stateRestoreTimer)
      : m_MessageLoop(messageLoop), m_StateRestoreTimer(stateRestoreTimer) {}
  void Run() override {
    m_StateRestoreTimer->Pause();
    LOG_INFO << "State restore completed in "
             << FormatDuration(std::chrono::nanoseconds(m_StateRestoreTimer->Get()));
    m_MessageLoop->RunLoop(0);
    LOG_INFO << "Playback started";
  }

private:
  MessageLoop* m_MessageLoop{};
  Timer* m_StateRestoreTimer{};
};

class FrameEndCommandRunner : public stream::CommandRunner {
public:
  FrameEndCommandRunner(MessageLoop* messageLoop, stream::BaseStreamReader* streamReader)
      : m_MessageLoop(messageLoop), m_StreamReader(streamReader) {}

  void Run() override {
    ++(*m_FrameCount);
    MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_FRAME_END},
                              std::make_shared<FrameEndMessage>(*m_FrameCount));

    m_MessageLoop->RunLoop(*m_FrameCount);

    if (*m_FrameCount == Configurator::Get().common.player.exitFrame) {
      m_StreamReader->Close();
    }
  }

  void SetFrameCount(unsigned* frameCount) {
    m_FrameCount = frameCount;
  }

private:
  MessageLoop* m_MessageLoop{};
  stream::BaseStreamReader* m_StreamReader{};
  unsigned* m_FrameCount{};
};

class MarkerUInt64StatusRunner : public stream::CommandRunner {
public:
  void Run() override {
    using Value = stream::MarkerUInt64Value;
    static auto segmentBegin = std::chrono::steady_clock::now();
    switch (static_cast<Value>(m_Value)) {
    case Value::STATE_RESTORE_OBJECTS_BEGIN:
    case Value::STATE_RESTORE_RTAS_BEGIN:
    case Value::STATE_RESTORE_RESOURCES_BEGIN:
      segmentBegin = std::chrono::steady_clock::now();
      break;
    case Value::STATE_RESTORE_OBJECTS_END:
      LOG_INFO << "Objects restored in "
               << FormatDuration(std::chrono::steady_clock::now() - segmentBegin);
      break;
    case Value::STATE_RESTORE_RTAS_END:
      LOG_INFO << "RTAS restored in "
               << FormatDuration(std::chrono::steady_clock::now() - segmentBegin);
      break;
    case Value::STATE_RESTORE_RESOURCES_END:
      LOG_INFO << "Resources restored in "
               << FormatDuration(std::chrono::steady_clock::now() - segmentBegin);
      break;
    default:
      break;
    }
  }

protected:
  void DecodeCommand() override {
    std::memcpy(&m_Value, m_Data, sizeof(m_Value));
  }

private:
  uint64_t m_Value{};
};

class CommonCommandFactory : public stream::CommandFactory {
public:
  void Initialize(stream::BaseStreamReader* streamReader,
                  Timer* stateRestoreTimer,
                  Timer* playbackTimer) {
    m_MessageLoop.reset(new MessageLoop(streamReader, playbackTimer));
    m_StreamReader = streamReader;
    m_StateRestoreTimer = stateRestoreTimer;
  }

  void SetStreamReader(stream::BaseStreamReader* streamReader) {
    m_StreamReader = streamReader;
    m_MessageLoop->SetStreamReader(streamReader);
  }

  stream::CommandRunner* CreateCommand(unsigned id) override {
    switch (static_cast<stream::CommonCommandId>(id)) {
    case stream::CommonCommandId::ID_INIT_START:
      return new StateRestoreBeginRunner(m_StateRestoreTimer);
    case stream::CommonCommandId::ID_INIT_END:
      return new StateRestoreEndRunner(m_MessageLoop.get(), m_StateRestoreTimer);
    case stream::CommonCommandId::ID_FRAME_END: {
      auto runner = new FrameEndCommandRunner(m_MessageLoop.get(), m_StreamReader);
      runner->SetFrameCount(&m_FrameCount);
      return runner;
    }
    case stream::CommonCommandId::ID_MARKER_UINT64:
      return new MarkerUInt64StatusRunner();
    }
    return nullptr;
  }

  void NotifyPlaybackDone() {
    m_MessageLoop->NotifyPlaybackDone();
  }

private:
  std::unique_ptr<MessageLoop> m_MessageLoop;
  stream::BaseStreamReader* m_StreamReader{};
  Timer* m_StateRestoreTimer{};
  unsigned m_FrameCount = 0;
};

void PlayStream(const std::filesystem::path& streamPath) {
  std::ifstream stream(streamPath, std::ios::in | std::ios::binary);
  stream::StreamHeader& header = stream::StreamHeader::Get();
  header.ReadHeader(stream);

  Timer stateRestoreTimer(true);
  // Published on CGits rather than kept local, so a plugin can measure frame
  // time on a clock that stops while playback is paused or being stepped. The
  // TimerSet constructs it paused, exactly as a local Timer(true) would be.
  Timer& playbackTimer = CGits::Instance().Timers().playback;

  std::vector<stream::CommandFactory*> commandFactories;

#if defined GITS_PLATFORM_WINDOWS_X64 && defined WITH_DIRECTX
  DirectX::DirectXCommandFactory directXCommandFactory;
  if (header.HasApi(stream::StreamHeader::Api::API_DIRECTX)) {
    commandFactories.push_back(&directXCommandFactory);
  }
#endif
#if defined WITH_VULKAN
  vulkan::VulkanCommandFactory vulkanCommandFactory;
  if (header.HasApi(stream::StreamHeader::Api::API_VULKAN)) {
    commandFactories.push_back(&vulkanCommandFactory);
  }
#endif
#if defined WITH_OPENGL
  opengl::OpenGLCommandFactory openGLCommandFactory;
  if (header.HasApi(stream::StreamHeader::Api::API_OPENGL)) {
    commandFactories.push_back(&openGLCommandFactory);
  }
#endif
  if (header.HasMultipleApis() && commandFactories.size() < header.GetApis().size()) {
    throw std::runtime_error("The stream was recorded with several APIs, but the player was "
                             "built without some of them.");
  }
  // The subcapture of a stream with several APIs counts the frames of all of them from the first
  // one, also for an API whose first command follows the subcapture range or is never replayed.
  if (header.HasMultipleApis()) {
    for (stream::CommandFactory* commandFactory : commandFactories) {
      commandFactory->CreatePlayer();
    }
  }

  Configurator::Instance().LogUnspportedConfigUsage(header.GetApiBool());

  CommonCommandFactory commonCommandFactory;
  commandFactories.push_back(&commonCommandFactory);

  if (Configurator::Get().common.player.interactive) {
    LOG_INFO << "Starting interactive playback IPC";
  }
  commonCommandFactory.Initialize(nullptr, &stateRestoreTimer, &playbackTimer);

  std::unique_ptr<stream::BaseStreamReader> streamReader;
#if defined GITS_PLATFORM_WINDOWS_X64
  if (stream::StreamHeader::Get().IsLegacyStream()) {
    streamReader.reset(new stream::StreamLegacyReader(commandFactories, stream));
  } else {
    streamReader.reset(new stream::StreamReader(commandFactories, stream));
  }
#else
  streamReader.reset(new stream::StreamReader(commandFactories, stream));
#endif

  commonCommandFactory.SetStreamReader(streamReader.get());
  if (Configurator::Get().common.player.interactive) {
    LOG_INFO << "Interactive playback IPC is ready for stream processing";
  }

  auto closeSubscription = MessageBus::get().subscribe(
      {PUBLISHER_PLAYER, TOPIC_CLOSE_PLAYER},
      [&streamReader](Topic, const MessagePtr&) { streamReader->Close(); });

  playbackTimer.Start();
  streamReader->Run();
  playbackTimer.Pause();
  commonCommandFactory.NotifyPlaybackDone();

  MessageBus::get().unsubscribe(closeSubscription);

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_END}, std::make_shared<ProgramMessage>());

  LOG_INFO << "Playback completed";
  LOG_INFO << "  State restore duration: "
           << FormatDuration(std::chrono::nanoseconds(stateRestoreTimer.Get()));
  LOG_INFO << "  Playback duration: "
           << FormatDuration(
                  std::chrono::nanoseconds(playbackTimer.Get() - stateRestoreTimer.Get()));
  LOG_INFO << "  Total duration: " << FormatDuration(std::chrono::nanoseconds(playbackTimer.Get()));

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                            std::make_shared<ProgramMessage>());

  if (header.HasApi(stream::StreamHeader::Api::API_VULKAN) ||
      header.HasApi(stream::StreamHeader::Api::API_DIRECTX)) {
    windowing::WindowManager::Get().DestroyAllWindows();
  }
}

class ApiExtractor : public stream::CommandFactory {
public:
  void Initialize(stream::StreamLegacyReader* streamReader) {
    m_StreamReader = streamReader;
  }
  stream::CommandRunner* CreateCommand(unsigned id) override {
    m_ApiId = stream::ExtractApiIdentifier(id);
    if (m_ApiId != stream::ApiId::ID_COMMON) {
      m_StreamReader->Close();
    }
    return nullptr;
  }
  stream::ApiId GetApi() const {
    return m_ApiId;
  }

private:
  stream::StreamLegacyReader* m_StreamReader{};
  stream::ApiId m_ApiId{};
};

namespace {

bool IsLegacyStream(const std::filesystem::path& streamPath) {
  std::ifstream stream(streamPath, std::ios::in | std::ios::binary);
  stream::StreamHeader& header = stream::StreamHeader::Get();
  header.ReadHeader(stream);

  if (header.GetApi() == stream::StreamHeader::Api::API_NOT_SET) {
    if (header.GetCompressionType() != CompressionType::LZ4) {
      return true;
    }
    ApiExtractor apiExtractor;
    std::vector<stream::CommandFactory*> commandFactories;
    commandFactories.push_back(&apiExtractor);
    stream::StreamLegacyReader reader(commandFactories, stream);
    apiExtractor.Initialize(&reader);
    reader.Run();
    stream::ApiId apiId = apiExtractor.GetApi();
    return apiId != stream::ApiId::ID_DIRECTX;
  } else {
    // A build with WITH_OPENGL=OFF has no OpenGLCommandFactory, so OpenGL streams have to be
    // rejected here instead of being replayed without any OpenGL commands.
    return header.GetApi() != stream::StreamHeader::Api::API_DIRECTX &&
           header.GetApi() != stream::StreamHeader::Api::API_VULKAN
#if defined WITH_OPENGL
           && header.GetApi() != stream::StreamHeader::Api::API_OPENGL
#endif
        ;
  }
}

const char* RemovedApiName(stream::StreamHeader::Api api) {
  switch (api) {
  case stream::StreamHeader::Api::API_OPENGL_LEGACY:
    return "OpenGLLegacy";
  case stream::StreamHeader::Api::API_VULKAN_LEGACY:
    return "VulkanLegacy";
  case stream::StreamHeader::Api::API_OPENCL:
    return "OpenCL";
  case stream::StreamHeader::Api::API_LEVELZERO:
    return "Level Zero";
  default:
    return nullptr;
  }
}

const char* RemovedApiName(stream::ApiId apiId) {
  switch (apiId) {
  case stream::ApiId::ID_OPENGL_LEGACY:
  case stream::ApiId::ID_GL_HELPER_TOKENS:
  case stream::ApiId::ID_WGL:
  case stream::ApiId::ID_GLX:
  case stream::ApiId::ID_EGL:
    return "OpenGLLegacy";
  case stream::ApiId::ID_OPENCL:
    return "OpenCL";
  case stream::ApiId::ID_VULKAN_LEGACY:
    return "VulkanLegacy";
  case stream::ApiId::ID_LEVELZERO:
    return "Level Zero";
  case stream::ApiId::ID_OCLOC:
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
      std::unique_ptr<stream::StreamDecompressor> decompressor;
      if (compressionType == CompressionType::LZ4) {
        decompressor = std::make_unique<stream::LZ4StreamDecompressor>();
      } else {
        decompressor = std::make_unique<stream::ZSTDStreamDecompressor>();
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
stream::ApiId FindFirstApiToken(const std::vector<char>& data) {
  size_t offset = 0;
  while (offset + sizeof(uint8_t) + sizeof(uint16_t) <= data.size()) {
    const auto apiId = static_cast<uint8_t>(data[offset]);
    uint16_t tokenId{};
    std::memcpy(&tokenId, data.data() + offset + sizeof(apiId), sizeof(tokenId));
    offset += sizeof(apiId) + sizeof(tokenId);
    if (apiId != static_cast<uint8_t>(stream::ApiId::ID_COMMON)) {
      return static_cast<stream::ApiId>(apiId);
    }
    // Common tokens are markers without data, except for these, which have 8 bytes of data.
    constexpr uint16_t ID_PLAYER_RECORDER_SYNC = 7;
    constexpr uint16_t ID_SCREEN_RESOLUTION = 9;
    if (tokenId == ID_PLAYER_RECORDER_SYNC || tokenId == ID_SCREEN_RESOLUTION ||
        tokenId == static_cast<uint16_t>(stream::CommonCommandId::ID_MARKER_UINT64)) {
      offset += sizeof(uint64_t);
    }
  }
  return stream::ApiId::ID_COMMON;
}

// Returns the name of the removed backend a legacy stream was recorded with.
const char* FindRemovedApi(const std::filesystem::path& streamPath) {
  const stream::StreamHeader& header = stream::StreamHeader::Get();
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
    if (const char* name = RemovedApiName(static_cast<stream::StreamHeader::Api>(apiCompute))) {
      return name;
    }
  }
  if (!stream) {
    return nullptr;
  }
  return RemovedApiName(FindFirstApiToken(ReadLegacyTokenData(stream, version)));
}

} // namespace

bool IsSupportedStream(const std::filesystem::path& streamPath) {
  if (!IsLegacyStream(streamPath)) {
    return true;
  }

  if (stream::StreamHeader::Get().GetApi() == stream::StreamHeader::Api::API_OPENGL) {
    LOG_ERROR << "The stream was recorded with OpenGL, which isn't supported by this GITS build.";
  } else if (const char* removedApi = FindRemovedApi(streamPath)) {
    LOG_ERROR << "The stream was recorded with " << removedApi
              << ", which is no longer supported by GITS. Replay it with an older GITS release "
                 "that supports "
              << removedApi << ".";
  } else {
    LOG_ERROR << "The stream format is no longer supported by GITS. Streams recorded with "
                 "OpenGLLegacy, VulkanLegacy, OpenCL, Level Zero or ocloc can only be replayed "
                 "with an older GITS release.";
  }
  return false;
}

void LogStreamRecordingVersion() {
  const stream::StreamHeader& header = stream::StreamHeader::Get();
  const std::array<unsigned, 4> version = header.GetVersion();
  LogRecordingVersion(CVersion(GITS_MAKE_VERSION4(version[0], version[1], version[2], version[3])),
                      header.FindProperty("diag.gits.version"));
}

} // namespace gits
