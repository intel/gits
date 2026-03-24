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
#include "timer.h"

#include <fstream>
#include <vector>
#include <memory>

#if defined GITS_PLATFORM_WINDOWS_X64
#if defined WITH_DIRECTX
#include "directXCommandFactory.h"
#endif
#include <windows.h>
#endif

namespace gits {

#if defined GITS_PLATFORM_WINDOWS_X64

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_CLOSE:
    PostMessage(hWnd, uMsg, wParam, lParam);
    break;
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}

class MessageLoop {
public:
  MessageLoop(stream::BaseStreamReader* streamReader, Timer* playbackTimer)
      : m_StreamReader(streamReader), m_PlaybackTimer(playbackTimer) {
    m_Interactive = Configurator::Get().common.player.interactive;
    m_StopAfterFrames = Configurator::Get().common.player.stopAfterFrames;
  }

  void RunLoop(unsigned frame) {
    MSG msg{};
    do {
      if (m_Interactive || m_StopAfterFrames[frame]) {
        m_Paused = true;
        m_PlaybackTimer->Pause();
      }
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        if (msg.message == WM_CLOSE) {
          m_StreamReader->Close();
          m_Paused = false;
          m_Interactive = false;
        } else {
          if (msg.message == WM_KEYDOWN) {
            const unsigned ESC_CODE = 27;
            switch (msg.wParam) {
            case ESC_CODE:
              m_StreamReader->Close();
              m_Paused = false;
              m_Interactive = false;
              break;
            case ' ':
              m_Paused = !m_Paused;
              if (m_Paused) {
                m_PlaybackTimer->Pause();
              } else {
                m_PlaybackTimer->Resume();
              }
              break;
            case 'i':
            case 'I':
              m_Interactive = !m_Interactive;
              if (!m_Interactive) {
                m_Paused = false;
                m_PlaybackTimer->Resume();
              }
              break;
            }
          }
          DispatchMessage(&msg);
        }
      }
    } while (m_Paused);
  }

private:
  bool m_Paused{};
  bool m_Interactive{};
  BitRange m_StopAfterFrames;
  Timer* m_PlaybackTimer{};
  stream::BaseStreamReader* m_StreamReader{};
};

class StateRestoreBeginRunner : public stream::CommandRunner {
public:
  StateRestoreBeginRunner(Timer* stateRestoreTimer) : m_StateRestoreTimer(stateRestoreTimer) {}
  void Run() override {
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
    m_MessageLoop->RunLoop(0);
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

    HWND window = GetWindowHandle();
    if (window) {
      static bool procChanged = false;
      if (!procChanged) {
        SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));
        procChanged = true;
      }
      if (Configurator::Get().common.player.showWindowBorder) {
        std::string title = "Current frame: " + std::to_string(frameCount);
        SetWindowTextA(window, title.c_str());
      }
    }
    m_MessageLoop->RunLoop(frameCount);

    if (frameCount == Configurator::Get().common.player.exitFrame) {
      m_StreamReader->Close();
    }
  }

private:
  MessageLoop* m_MessageLoop{};
  stream::BaseStreamReader* m_StreamReader{};
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

  CommonCommandFactory commonCommandFactory;
  commandFactories.push_back(&commonCommandFactory);

#if defined WITH_DIRECTX
  DirectX::DirectXCommandFactory directXCommandFactory;
  commandFactories.push_back(&directXCommandFactory);
#endif

  std::unique_ptr<stream::BaseStreamReader> streamReader;
  if (stream::StreamHeader::Get().IsLegacyStream()) {
    streamReader.reset(new stream::StreamLegacyReader(commandFactories, stream));
  } else {
    streamReader.reset(new stream::StreamReader(commandFactories, stream));
  }

  commonCommandFactory.Initialize(streamReader.get(), &stateRestoreTimer, &playbackTimer);
  playbackTimer.Start();
  streamReader->Run();
  playbackTimer.Pause();

  LOG_INFO << "";
  LOG_INFO << "State restored in: " << stateRestoreTimer.Get() / 1e6 << "ms";
  LOG_INFO << "Played back in: " << (playbackTimer.Get() - stateRestoreTimer.Get()) / 1e6 << "ms";
  LOG_INFO << "Total runtime: " << playbackTimer.Get() / 1e6 << "ms";

  MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                            std::make_shared<ProgramMessage>());
}

#else
void PlayStream(const std::filesystem::path& streamPath) {}
#endif // GITS_PLATFORM_WINDOWS_X64

class ApiExtractor : public stream::CommandFactory {
public:
  void Initialize(stream::StreamLegacyReader* streamReader) {
    m_StreamReader = streamReader;
  }
  stream::CommandRunner* CreateCommand(unsigned id) override {
    m_ApiId = static_cast<stream::ApiId>(id / 0x10000);
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
    return header.GetApi() != stream::StreamHeader::Api::API_DIRECTX;
  }
}

} // namespace gits
