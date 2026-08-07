// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamPlayer.h"
#include "configurationLib.h"
#include "log.h"
#include "argumentParser.h"
#include "messageBus.h"
#include "streamReader.h"
#include "streamLegacyReader.h"
#include "streamHeader.h"
#include "commandFactory.h"
#include "commandId.h"
#include "commandRunner.h"
#include "timer.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

#if defined GITS_PLATFORM_WINDOWS_X64
#if defined WITH_DIRECTX
#include "directXCommandFactory.h"
#endif
#endif
#if defined WITH_VULKAN
#include "vulkanCommandFactory.h"
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
  }

  void RunLoop(unsigned frame) {
    if (Configurator::Get().common.player.showWindowBorder &&
        Configurator::Get().common.player.showFrameNumberInTitle) {
      windowing::WindowManager::Get().SetTitle("Current frame: " + std::to_string(frame));
    }

    do {
      if (m_Interactive || m_StopAfterFrames[frame]) {
        m_Paused = true;
        m_PlaybackTimer->Pause();
      }

      auto events = windowing::WindowManager::Get().PollEvents();
      for (auto event : events) {
        switch (event) {
        case windowing::WindowEvent::Close:
        case windowing::WindowEvent::Stop:
          m_StreamReader->Close();
          m_Paused = false;
          m_Interactive = false;
          break;
        case windowing::WindowEvent::TogglePause:
          m_Paused = !m_Paused;
          if (m_Paused) {
            m_PlaybackTimer->Pause();
          } else {
            m_PlaybackTimer->Resume();
          }
          break;
        case windowing::WindowEvent::ToggleInteractive:
          m_Interactive = !m_Interactive;
          if (!m_Interactive) {
            m_Paused = false;
            m_PlaybackTimer->Resume();
          }
          break;
        }
      }
    } while (m_Paused);
  }

private:
  stream::BaseStreamReader* m_StreamReader{};
  Timer* m_PlaybackTimer{};
  bool m_Paused{};
  bool m_Interactive{};
  BitRange m_StopAfterFrames;
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
    static unsigned frameCount = 0;
    ++frameCount;

    m_MessageLoop->RunLoop(frameCount);

    if (frameCount == Configurator::Get().common.player.exitFrame) {
      m_StreamReader->Close();
    }
  }

private:
  MessageLoop* m_MessageLoop{};
  stream::BaseStreamReader* m_StreamReader{};
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

  stream::CommandRunner* CreateCommand(unsigned id) override {
    switch (static_cast<stream::CommonCommandId>(id)) {
    case stream::CommonCommandId::ID_INIT_START:
      return new StateRestoreBeginRunner(m_StateRestoreTimer);
    case stream::CommonCommandId::ID_INIT_END:
      return new StateRestoreEndRunner(m_MessageLoop.get(), m_StateRestoreTimer);
    case stream::CommonCommandId::ID_FRAME_END:
      return new FrameEndCommandRunner(m_MessageLoop.get(), m_StreamReader);
    case stream::CommonCommandId::ID_MARKER_UINT64:
      return new MarkerUInt64StatusRunner();
    }
    return nullptr;
  }

private:
  std::unique_ptr<MessageLoop> m_MessageLoop;
  stream::BaseStreamReader* m_StreamReader{};
  Timer* m_StateRestoreTimer{};
};

void PlayStream(const std::filesystem::path& streamPath) {
  std::ifstream stream(streamPath, std::ios::in | std::ios::binary);
  stream::StreamHeader& header = stream::StreamHeader::Get();
  header.ReadHeader(stream);

  Timer stateRestoreTimer(true);
  Timer playbackTimer(true);

  std::vector<stream::CommandFactory*> commandFactories;

#if defined GITS_PLATFORM_WINDOWS_X64 && defined WITH_DIRECTX
  DirectX::DirectXCommandFactory directXCommandFactory;
  if (header.GetApi() == stream::StreamHeader::Api::API_DIRECTX) {
    commandFactories.push_back(&directXCommandFactory);
  }
#endif
#if defined WITH_VULKAN
  vulkan::VulkanCommandFactory vulkanCommandFactory;
  if (header.GetApi() == stream::StreamHeader::Api::API_VULKAN) {
    commandFactories.push_back(&vulkanCommandFactory);
  }
#endif

  CommonCommandFactory commonCommandFactory;
  commandFactories.push_back(&commonCommandFactory);

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

  commonCommandFactory.Initialize(streamReader.get(), &stateRestoreTimer, &playbackTimer);
  playbackTimer.Start();
  streamReader->Run();
  playbackTimer.Pause();

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_END}, std::make_shared<ProgramMessage>());

  if (header.GetApi() == stream::StreamHeader::Api::API_VULKAN ||
      header.GetApi() == stream::StreamHeader::Api::API_DIRECTX) {
    windowing::WindowManager::Get().DestroyAllWindows();
  }

  LOG_INFO << "Playback completed";
  LOG_INFO << "  State restore duration: "
           << FormatDuration(std::chrono::nanoseconds(stateRestoreTimer.Get()));
  LOG_INFO << "  Playback duration: "
           << FormatDuration(
                  std::chrono::nanoseconds(playbackTimer.Get() - stateRestoreTimer.Get()));
  LOG_INFO << "  Total duration: " << FormatDuration(std::chrono::nanoseconds(playbackTimer.Get()));

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                            std::make_shared<ProgramMessage>());
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
    return header.GetApi() != stream::StreamHeader::Api::API_DIRECTX &&
           header.GetApi() != stream::StreamHeader::Api::API_VULKAN;
  }
}

} // namespace gits
