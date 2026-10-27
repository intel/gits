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
#include "commandFactory.h"
#include "commandId.h"
#include "commandRunner.h"
#include "timer.h"
#include "remotePlaybackController.h"

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
#if defined WITH_OPENGL
  opengl::OpenGLCommandFactory openGLCommandFactory;
  if (header.GetApi() == stream::StreamHeader::Api::API_OPENGL) {
    commandFactories.push_back(&openGLCommandFactory);
  }
#endif

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

  if (header.GetApi() == stream::StreamHeader::Api::API_VULKAN ||
      header.GetApi() == stream::StreamHeader::Api::API_DIRECTX) {
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
    // Streams from APIs with a new-architecture (per-command CommandFactory) player path must
    // be excluded here, or they get misrouted into the legacy CFile/CBinIStream player.Load()
    // path, which reinterprets the Api field using the unrelated ApisIface::TApi enum and
    // throws ENotImplemented from ApisIface::ToApiBool() on the mismatched value.
    //
    // The API_OPENGL term is only correct to exclude when this binary was actually built with
    // an OpenGLCommandFactory to route to - see the identical "WITH_OPENGL" guard around
    // PlayStream()'s own OpenGLCommandFactory registration above. WITH_OPENGL is a normal,
    // independently togglable CMake option (see the root CMakeLists.txt) - a build with
    // WITH_OPENGL=OFF compiles out that factory entirely. Without this guard, such a binary
    // would still take THIS function's "not legacy" branch for an OpenGL stream, route it into
    // PlayStream()'s modern StreamReader path anyway, and have StreamReader::DecodeBlock()
    // silently skip every unrecognized OpenGL command ID (no factory claims them) - completing
    // "successfully" having replayed zero GL calls, rather than the loud ENotImplemented failure
    // this same exclusion already produces today for a DIRECTX/VULKAN stream handed to a player
    // binary missing that backend (see this method's own review fix - reported as a real,
    // reachable gap specific to OpenGL's own opt-out-capable build option, not fixed here for
    // DIRECTX/VULKAN's pattern-identical WITH_DIRECTX/WITH_VULKAN options, which are outside
    // this fix's OpenGL-backend scope).
    //
    // Deliberately just `defined WITH_OPENGL`, with no additional platform/architecture term:
    // the root CMakeLists.txt owns platform/architecture gating (WITH_OPENGL is 64-bit-only and
    // defaults ON on Windows x64 and x86-64 Linux). Keep this C++-level guard checking only the
    // one thing it actually cares about - whether the OpenGLCommandFactory this branch routes to
    // was compiled in.
    return header.GetApi() != stream::StreamHeader::Api::API_DIRECTX &&
           header.GetApi() != stream::StreamHeader::Api::API_VULKAN
#if defined WITH_OPENGL
           && header.GetApi() != stream::StreamHeader::Api::API_OPENGL
#endif
        ;
  }
}

} // namespace gits
