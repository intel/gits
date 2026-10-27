// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"

#include "systemApi.h"
#include "unsupportedCommandTypes.h"

#if !defined(_WIN32)
#include "messageBus.h"

#include <cstdlib>
#endif

namespace gits::opengl2 {

#if !defined(_WIN32)
namespace {

// Windows publishes the program exit from the ExitProcess hook in recorder/recorderDLL.cpp.
void PublishProgramExit() {
  MessageBus::get().publish({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                            std::make_shared<ProgramMessage>());
}

} // namespace
#endif

// See SetPendingPreHookKey()/TakePendingPreHookKey()'s own header comment (captureManager.h).
thread_local uint64_t CaptureManager::s_PendingPreHookKey = 0;
thread_local uint64_t CaptureManager::s_PendingPostHookKey = 0;
thread_local uint64_t CaptureManager::s_PendingMappedTextureKey = 0;
thread_local uint64_t CaptureManager::s_PendingPixelFormatKey = 0;
CaptureManager& CaptureManager::Get() {
  // Thread-safe function-local static initialization (C++11 "magic statics") replaces a
  // hand-rolled, unsynchronized check-then-write of a raw pointer, which could race two
  // concurrent first-callers (GL wrapper threads) into constructing two different
  // CaptureManager instances - splitting command keys/recording across them. The instance is
  // still heap-allocated and intentionally never destroyed (see this method's declaration
  // comment in captureManager.h) - only the *construction* was unsynchronized before; the
  // "never destroyed" property is unaffected by using a static pointer instead of a static
  // value here.
  static CaptureManager* instance = new CaptureManager();
  return *instance;
}

CaptureManager::CaptureManager() {
  // Runs inside the application's first OpenGL call.
  const LastSystemErrorGuard lastErrorGuard;
  m_LayerManager.LoadLayers();
  // After the layers, so that plugins handle a close request before the recorder closes the
  // stream. Layer hooks run only after this, so a plugin may request closing from its first hook.
  m_Recorder = std::make_unique<stream::OrderingRecorder>();
#if !defined(_WIN32)
  // Exit handlers run in reverse order of registration, together with the destructors of static
  // objects, so everything constructed before this outlives the handler: the plugins, and the
  // services the recorder constructs at startup to subscribe to the program exit.
  std::atexit(PublishProgramExit);
#endif
}

stream::OrderingRecorder& CaptureManager::GetOrderingRecorder() {
  return *m_Recorder;
}

void CaptureManager::RecordCommand(Command& command, stream::CommandSerializer* serializer) {
  // See the header comment: command.m_Key must already be set by the caller - `serializer`
  // was built from `command` before this call, so setting it here would not affect the
  // already-encoded bytes.
  GetOrderingRecorder().Record(command.m_Key, serializer);
}

void CaptureManager::RecordUnsupportedCommand(const char* name, uint64_t key, uint32_t threadId) {
  UnsupportedCommand command(threadId, name);
  command.m_Key = key;
  RecordCommand(command, new UnsupportedCommandSerializer(command));
}

void CaptureManager::SkipCommandKey(uint64_t key) {
  GetOrderingRecorder().Skip(key);
}

} // namespace gits::opengl2
