// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorder.h
 *
 * @brief Declaration of function calls recorder.
 *
 */

#pragma once

#include "gits.h"
#include "tools.h"
#include "function.h"
#include "performance.h"
#include "pragmas.h"
#include "InputListener.h"

#include <list>
#include <vector>
#include <string>
#include <apis_iface.h>

#ifdef GITS_PLATFORM_WINDOWS
#include "eventHandlers.h"
#endif

namespace gits {
class CScheduler;
class CToken;
class CBinOStream;
class CCodeOStream;
class CRecorderWrapper;
class CRecorder;
class CBehavior;
class CAction;

/**
   * @brief Main class of a recorder
   *
   * gits::CRecorder class is responsible for a process of recording
   * library calls. Its behavior is specified by one or more
   * gits::CBehavior classes that run FrameNumber(), Start(),
   * Stop(), Save() and Clear() methods of a recorder. To know
   * when to call that methods gits::CBehavior class has
   * gits::CBehavior::OnInit() and gits::CBehavior::OnFrameEnd()
   * handlers. First is called when recorder is initializing
   * and the second one is triggered by the proxy library that tracks
   * incoming library calls. Each gits::CBehavior class has
   * assigned its own library calls scheduler (gits::CScheduler). @n
   * Class also provides FrameEnd(), Running() and Schedule() methods
   * for proxy library. Schedule() is used to dispatch created library
   * function call wrapper to all schedulers. That wrappers should be
   * created and provided to the recorder only if the recorder is running
   * (Running()). FrameEnd() is used to signal that rendering of a current
   * frame has ended.
   *
   * @note gits::CRecorder class needs at least one
   * gits::CBehavior class registered to run correctly.
   */
class CRecorder : private gits::noncopyable {
  static CRecorder* _instance;
  bool _recordingOverride;
  bool _running;
  bool _runningStarted;
  bool _isMarkedForDeletion;
  InputListener::CInputListener _inputListener;
  uint _exitHotKeyId;
  uint _startHotKeyId;
  std::recursive_mutex _mutex;

#ifdef GITS_PLATFORM_WINDOWS
  ExitEventHandler exitEventHandler;
#endif

  std::vector<std::function<void()>> _disposeEvents;
  StreamingContext _sc;

  CRecorder();
  CRecorder(const CRecorder& other) = delete;
  CRecorder& operator=(const CRecorder& other) = delete;
  ~CRecorder();

public:
  static CRecorder& Instance();
  static CRecorder* InstancePtr();

  static void Dispose();
  void Close();

  void RegisterDisposeEvent(std::function<void()> e);

  CLibrary* Library(CLibrary::TId id);
  void Register(std::shared_ptr<CLibrary> library);
  void EndFramePost();

  CScheduler& Scheduler() const;
  CBehavior& Behavior() const;

  void Register(std::unique_ptr<CBehavior> behavior);
  void Init();
  const CRecorderWrapper& Wrapper() const;

  void Start();
  void Stop();
  void Save();
  std::recursive_mutex& GetMutex();
  void TrackThread(gits::ApisIface::TApi api);

  void RecordingOverride(bool enable) {
    _recordingOverride = enable;
  }
  bool RecordingOverride() {
    return _recordingOverride;
  }
  void Pause();
  void Continue();

  // interface for proxy library
  void DrawBegin();
  void DrawEnd();
  void CmdBufferEnd(gits::CGits::CCounter counter);
  void QueueSubmitEnd();
  void KernelBegin();
  void KernelEnd();
  void FrameEnd();
  bool Running() const;
  bool Schedule(CToken* function, bool force = false);
  void MarkForDeletion() {
    _isMarkedForDeletion = true;
  }
  bool IsMarkedForDeletion() {
    return _isMarkedForDeletion;
  }
};
} // namespace gits

/**
 * @brief Running state getter
 *
 * Getter that returns current running state of the recorder.
 *
 * @return Current running state of the recorder.
 * @retval true If any registered behavior is running
 * @retval false When all registered behaviors are stopped
 */
inline bool gits::CRecorder::Running() const {
  return _running || _recordingOverride;
}
