// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorder.cpp
 *
 * @brief Definition of function calls recorder.
 *
 */

#include "openglDrivers.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "recorder.h"
#include "gits.h"
#include "state.h"
#include "library.h"
#include "scheduler.h"
#include "streams.h"
#include "tools.h"
#include "exception.h"
#include "config.h"
#include "log.h"
#include "config.h"
#include "controlHandler.h"
#include "recorderBehaviors.h"
#include "pragmas.h"

#if defined(GITS_PLATFORM_LINUX)
#include <sys/stat.h>
#include <csignal>
#endif

#include "openglRecorderWrapper.h"

#include <memory>
#include <filesystem>
#include <fstream>

DISABLE_WARNINGS
#include <boost/thread.hpp>
ENABLE_WARNINGS

#ifndef GITS_PLATFORM_WINDOWS
static void detach() __attribute__((destructor(101)));
#endif

/* ******************************* R E C O R D E R ******************************** */

bool forceExit;

void ExitHotKeyPressed() {
  //If only OCL was loaded then close gits now because frame will be less then 2
  //If not then close method will be called at OGL EndFramePost method.
  if (gits::CGits::Instance().CurrentFrame() < 2) {
    gits::CRecorder::Instance().MarkForDeletion();
  }
}

gits::CRecorder* gits::CRecorder::_instance = nullptr;

/**
 * @brief Getter of recorder instance
 *
 * It is assumed that only one recorder instance is created
 * in test system. That method returns a reference to it.
 *
 * @exception ENotFound Instance not created.
 *
 * @return Recorder instance
 */
gits::CRecorder& gits::CRecorder::Instance() {
  if (!_instance) {
    gits::OpenGL::drv.add_terminate_event([] { CRecorder::Dispose(); });
    _instance = new CRecorder;
#ifndef GITS_PLATFORM_WINDOWS
    std::atexit(detach);
#endif
  }
  return *_instance;
}

gits::CRecorder* gits::CRecorder::InstancePtr() {
  return _instance;
}

void gits::CRecorder::RegisterDisposeEvent(std::function<void()> e) {
  _disposeEvents.push_back(e);
}

void gits::CRecorder::Dispose() {
  delete _instance;
  _instance = nullptr;

#ifdef GITS_PLATFORM_WINDOWS
  RemoveSignalsHandler();
#endif
  if (gits::CGits::Instance().apis.HasCompute()) {
    const auto& computeIface = gits::CGits::Instance().apis.IfaceCompute();
    computeIface.MemorySnifferUninstall();
    computeIface.PrintMaxLocalMemoryUsage();
  }
  CGits::Instance().Dispose();
}

#ifdef GITS_PLATFORM_WINDOWS
namespace {
LONG WINAPI MyUnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo) {
  using namespace gits;
  if (CRecorder::InstancePtr()) {
    Log(ERR) << "Running user GITS unhandled exception callback";
    CRecorder::Instance().Stop();
    CRecorder::Instance().Save();
  }
  return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace
#endif

#if defined(GITS_PLATFORM_LINUX)
void InterruptHandler(int sig) {
  detach();
}
#endif

/**
 * @brief Constructor
 *
 * Constructor of gits::CRecorder class. Method sets current frame
 * number to @c 0.
 */

gits::CRecorder::CRecorder()
    : _recordingOverride(false),
      _running(false),
      _isMarkedForDeletion(false),
      _exitHotKeyId(0),
      _startHotKeyId(0) {
  CGits::Instance().SetSC(&this->_sc);

  Log(INFO) << "-----------------------------------------------------";
  Log(INFO) << " GITS Recorder (" << CGits::Instance().Version() << ")";
  Log(INFO) << "-----------------------------------------------------";

#ifdef GITS_PLATFORM_WINDOWS
  // handling signals
  SignalsHandler();
#endif
  forceExit = false;
  CGits& inst = CGits::Instance();
  const Config& config = Config::Get();

  // create file data and regisapi3dIfaceter it in GITS
  if (config.recorder.basic.enabled) {
    std::filesystem::create_directories(config.common.streamDir);
#if defined(GITS_PLATFORM_X11)
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = InterruptHandler;
    sigaction(Config::Get().recorder.basic.exitSignal, &action, nullptr);
#endif
    inst.ResourceManagerInit(config.common.streamDir);
  }
  // register behaviors
  Log(INFO) << "Recorder mode: ";

  // register behavior that dumps ranges of frames
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  std::ostringstream message;
  bool captureOnKeypress = !api3dIface.CfgRec_StartKeys().empty();
  if (!config.recorder.basic.enabled) {
    Log(INFO) << " - Off";
  } else if (api3dIface.CfgRec_IsFramesMode()) {
    message << " Start:" << api3dIface.CfgRec_StartFrame()
            << " Stop:" << api3dIface.CfgRec_StopFrame();
    Log(INFO) << " - Frames " << message.str();
    if (captureOnKeypress) {
      Log(INFO) << " - Initiate capture on user request. ";
    }
  } else if (api3dIface.CfgRec_IsAllMode()) {
    Log(INFO) << " - All";
  } else if (api3dIface.CfgRec_IsSingleDrawMode()) {
    message << " Number:" << api3dIface.CfgRec_SingleDraw();

    Log(INFO) << " - SingleDraw " << message.str();
  } else if (api3dIface.CfgRec_IsDrawsRangeMode()) {
    message << " Start:" << api3dIface.CfgRec_StartDraw()
            << " Stop:" << api3dIface.CfgRec_StopDraw() << " Frame:" << api3dIface.CfgRec_Frame();

    Log(INFO) << " - DrawsRange " << message.str();
  }
  Register(std::unique_ptr<CBehavior>(new CBehavior(*this, captureOnKeypress)));

  if (gits::CGits::Instance().apis.HasCompute()) {
    auto& apiCompute = gits::CGits::Instance().apis.IfaceCompute();
    std::ostringstream message2;
    if (apiCompute.CfgRec_IsAllMode()) {
      Log(INFO) << " - All";
    } else if (apiCompute.CfgRec_IsSingleKernelMode()) {
      message2 << " Number:" << apiCompute.CfgRec_SingleKernel();

      Log(INFO) << " - SingleKernel " << message2.str();
    } else if (apiCompute.CfgRec_IsKernelsRangeMode()) {
      message2 << " Start:" << apiCompute.CfgRec_StartKernel()
               << " Stop:" << apiCompute.CfgRec_StopKernel();

      Log(INFO) << " - KernelsRange"
                << (apiCompute.Api() != gits::ApisIface::TApi::LevelZero ? message2.str() : "");
    }
  }

  if (config.recorder.extras.utilities.forceDumpOnError) {
#ifdef _WIN32
    Exception::Callback(
        [](void*) {
          if (CRecorder::InstancePtr()) {
            CRecorder::Instance().Close();
          }
        },
        this);
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
#endif
  }

  // init recorder
  if (config.recorder.basic.enabled) {
    Init();
  }
}

#ifdef GITS_PLATFORM_WINDOWS

BOOL CALLBACK EnumWindowsProcCallback(HWND hwnd, LPARAM lParam) {
  DWORD windowProcessId;
  GetWindowThreadProcessId(hwnd, &windowProcessId);
  if (windowProcessId == static_cast<DWORD>(lParam)) {
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return FALSE;
  }
  return TRUE;
}

void CloseApplicationOnStopRecording() {
  EnumWindows(EnumWindowsProcCallback, GetCurrentProcessId());
}

#else

void CloseApplicationOnStopRecording() {
  TODO("Implementation required for platforms other than Windows!")
}

#endif

/**
 * @brief Destructor
 *
 * Destructor of gits::CRecorder class.
 */
gits::CRecorder::~CRecorder() {
  const Config& config = Config::Get();
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  if (api3dIface.CfgRec_IsBenchmark() && config.recorder.basic.enabled) {
    std::ofstream out_file(config.common.streamDir / "benchmark.csv");
    CGits::Instance().TimeSheet().OutputTimeData(out_file, true);
  }

  if (config.recorder.extras.utilities.forceDumpOnError) {
    Exception::Callback(nullptr, nullptr);
  }

  if (config.recorder.extras.utilities.closeAppOnStopRecording) {
    CloseApplicationOnStopRecording();
  }

  Close();
  Save();

  for (auto& e : _disposeEvents) {
    e();
  }

  _instance = nullptr;
}

gits::CLibrary* gits::CRecorder::Library(CLibrary::TId id) {
  CGits& inst = CGits::Instance();
  for (auto it = inst.LibraryBegin(); it != inst.LibraryEnd(); ++it) {
    if ((*it)->Id() == id) {
      return it->get();
    }
  }
  return nullptr;
}

void gits::CRecorder::Register(std::shared_ptr<CLibrary> library) {
  CGits::Instance().Register(library);
}

gits::CScheduler& gits::CRecorder::Scheduler() const {
  if (_sc.scheduler == nullptr) {
    throw ENotInitialized(EXCEPTION_MESSAGE);
  }

  return *_sc.scheduler;
}

gits::CBehavior& gits::CRecorder::Behavior() const {
  if (_sc.behavior == nullptr) {
    throw ENotInitialized(EXCEPTION_MESSAGE);
  }

  return *_sc.behavior;
}

/**
 * @brief Inits recorder class
 *
 * Method inits recorder class. All registered behaviors are notified via
 * gits::CBehavior::OnInit() handler.
 *
 * @note Should be run after registering initial behaviors and before
 * the start of capturing library calls.
 *
 * @note Ownership of a wrapper is passed to CRecorder class.
 *
 * @param wrapper Recorder wrapper to be used during capture
 */
void gits::CRecorder::Init() {
  Scheduler().Register(
      new CTokenFrameNumber(CToken::ID_PRE_RECORD_START, CGits::Instance().CurrentFrame()));

  if (Config::Get().common.useEvents) {
    CGits::Instance().PlaybackEvents().programStart();
  }

  if (Behavior().ShouldCapture()) {
    Start();
  }
}

/**
 * @brief Registers new behavior class
 *
 * Method registers new behavior class.
 *
 * @note Ownership is passed to gits::CRecorder::CBehaviorData class.
 *
 * @param behavior Behavior class to register
 */
void gits::CRecorder::Register(std::unique_ptr<CBehavior> behavior) {
  if (_sc.behavior) {
    throw std::runtime_error("GITS no longer supports running multiple behaviors. "
                             "Please double check configuration file to make sure "
                             "only single capturing behavior is enabled");
  }
  _sc.behavior.reset(behavior.release());

  auto& cmm = Config::Get().common;
  auto& rec = Config::Get().recorder;

  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  _exitHotKeyId = _inputListener.AddHotKey(rec.basic.exitKeys);
  _inputListener.AddHotKeyEvent(_exitHotKeyId, ExitHotKeyPressed);
  _startHotKeyId = _inputListener.AddHotKey(api3dIface.CfgRec_StartKeys());
  _inputListener.StartHotKeyListener(rec.extras.utilities.windowsKeyHandling ==
                                     TWindowsKeyHandling::MESSAGE_LOOP);

  if (rec.basic.enabled) {
    _sc.scheduler.reset(new CScheduler(cmm.tokenBurst, cmm.tokenBurstNum));
  }

  auto filePath = (cmm.streamDir / "stream").string();
  if (rec.basic.dumpGITS && rec.basic.enabled) {
    // create file
    std::string filename = filePath + ".gits2";

    _sc.oBinStream.reset(new CBinOStream(filename));

    // check if file was created
    if (!*_sc.oBinStream) {
      Log(ERR) << "Cannot create file '" << filename << "'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    Scheduler().Stream(_sc.oBinStream.get());
    (*_sc.oBinStream) << CGits::Instance();
  }

  if (rec.basic.dumpCCode && rec.basic.enabled) {
    // create file
    _sc.oCodeStream.reset(new CCodeOStream(filePath));

    // check if file was created
    if (!*_sc.oCodeStream) {
      Log(ERR) << "Cannot create file '" << filePath << ".c'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    Scheduler().Stream(_sc.oCodeStream.get());
  }
}

/**
 * @brief Starts recording
 *
 * Method starts recording of incoming library function calls for specified
 * behavior.
 *
 * @exception ENotFound Specified behavior class was not found.
 */
void gits::CRecorder::Start() {
  const CGits& inst = CGits::Instance();

  // If you are looking for OpenCL state restoration, it is implemented in OpenCL wrapper, in clEnqueueNDRangeKernel.
  // If we started capture on frame other than the first one or recording mode is 'SingleDraw', state restoration is needed.
  auto& api3dIface = inst.apis.Iface3D();
  auto apiComputeIface = inst.apis.HasCompute() ? &inst.apis.IfaceCompute() : nullptr;
  if ((api3dIface.CfgRec_IsFramesMode() && inst.CurrentFrame() != 1) ||
      (api3dIface.CfgRec_IsSingleDrawMode() && api3dIface.CfgRec_SingleDraw() != 1) ||
      (api3dIface.CfgRec_IsDrawsRangeMode()) || (api3dIface.CfgRec_IsCmdBufferMode()) ||
      (api3dIface.CfgRec_IsRenderPassMode()) || (api3dIface.CfgRec_IsEncodersRangeMode()) ||
      (api3dIface.CfgRec_IsSubEncodersRangeMode()) || (api3dIface.CfgRec_IsQueueSubmitMode()) ||
      (apiComputeIface != nullptr && (apiComputeIface->CfgRec_IsSingleKernelMode() ||
                                      apiComputeIface->CfgRec_IsKernelsRangeMode()))) {
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, inst.CurrentFrame()));
    for (auto it = inst.LibraryBegin(); it != inst.LibraryEnd(); ++it) {
      // schedule current library state only if not init frame number
      auto state((*it)->StateCreate());
      if (state) {
        CGits::Instance().StateRestoreStarted();
        state->Prepare();
        state->Get();
        Scheduler().Register(new CTokenFrameNumber(CToken::ID_INIT_START, inst.CurrentFrame()));
        state->Schedule(Scheduler());
        Scheduler().Register(new CTokenFrameNumber(CToken::ID_INIT_END, inst.CurrentFrame()));
        CGits::Instance().StateRestoreFinished();
        if (gits::CGits::Instance().apis.Has3D()) {
          api3dIface.Rec_StateRestoreFinished();
        }
        state->PostSchedule(Scheduler());
        delete state;
      }
    }
  }

  auto api = inst.apis.Iface3D().Api();
  if (CGits::Instance().CurrentFrame() == 1 &&
      (!Config::Get().recorder.basic.dumpCCode ||
       api != inst.apis.Vulkan)) //first frame is special case
  {
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, inst.CurrentFrame()));
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_FRAME_START, 1));
    auto& api3dIface_ = gits::CGits::Instance().apis.Iface3D();
    //first frame start time stamp
    if (api3dIface_.CfgRec_IsBenchmark()) {
      CGits::Instance().Timers().frame.Start();
    }
  }

  // update running flag
  //if (Config::Get().recorder.basic.enabled)
  _running = true;
}

/**
 * @brief Stops recording
 *
 * Method stops recording of incoming library function calls for specified
 * behavior. Current library state is stored.
 *
 * @exception ENotFound Specified behavior class was not found.
 */
void gits::CRecorder::Stop() {
  if (_running) {
    const CGits& inst = CGits::Instance();
    auto& api3dIface = inst.apis.Iface3D();
    auto apiComputeIface = inst.apis.HasCompute() ? &inst.apis.IfaceCompute() : nullptr;
    if (api3dIface.CfgRec_IsFramesMode() || api3dIface.CfgRec_IsSingleDrawMode() ||
        api3dIface.CfgRec_IsDrawsRangeMode() ||
        (apiComputeIface != nullptr && (apiComputeIface->CfgRec_IsSingleKernelMode() ||
                                        apiComputeIface->CfgRec_IsKernelsRangeMode()))) {
      if (apiComputeIface != nullptr) {
        apiComputeIface->MemorySnifferUninstall();
      }
      for (auto it = inst.LibraryBegin(); it != inst.LibraryEnd(); ++it) {
        // FIXME OpenGL crashes here if enabled
        if ((*it)->Id() == CLibrary::TId::ID_OPENCL || (*it)->Id() == CLibrary::TId::ID_LEVELZERO) {
          auto state((*it)->StateCreate());
          if (state) {
            state->Finish(Scheduler());
            delete state;
          }
        }
      }
    }
    Scheduler().Register(
        new gits::CTokenFrameNumber(CToken::ID_CCODE_FINISH, CGits::Instance().CurrentFrame()));
  }
  _running = false;
}

/**
 * @brief Saves recorded data to binary
 *
 * Method saves recorded library function calls to a binary file with
 * specified filename.
 *
 * @exception ENotFound Specified behavior class was not found.
 */
void gits::CRecorder::Save() {
  if (!Config::Get().recorder.basic.enabled) {
    return;
  }

  CALL_ONCE[&] {
    Scheduler().WriteAll();
  };
  CGits::Instance().CloseZipFileGLPrograms();
}

/**
 * @brief Frame end handler
 *
 * Method notifies all registered behaviors via
 * gits::CBehavior::OnFrameEnd() handler and increments
 * current frame number counter.
 *
 * @note Should be run by the proxy library when the process of rendering current
 * frame is finished.
 */
void gits::CRecorder::FrameEnd() {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();

  if (Running()) {
    Schedule(new CTokenFrameNumber(CToken::ID_FRAME_END, CGits::Instance().CurrentFrame()));

    if (api3dIface.CfgRec_EndFrameSleep() > 0) {
      sleep_millisec(api3dIface.CfgRec_EndFrameSleep());
    }

    Schedule(new CTokenPlayerRecorderSync);

    //frame end time stamp
    if (api3dIface.CfgRec_IsBenchmark()) {
      CGits::Instance().TimeSheet().add_frame_time("stamp",
                                                   CGits::Instance().Timers().program.Get());
      CGits::Instance().TimeSheet().add_frame_time("cpu", CGits::Instance().Timers().frame.Get());
    }
  }

  Behavior().OnFrameEnd();

  // increment current frame number
  CGits::Instance().FrameCountUp();
  if (api3dIface.CfgRec_IsFramesMode()) {
    if (_running && !Behavior().ShouldCapture()) {
      _running = false;
    }
    if (!_running && Behavior().ShouldCapture()) {
      Start();
    }
  }

  // we are at the next frame
  if (Running()) {
    Schedule(new gits::CTokenFrameNumber(CToken::ID_FRAME_START, CGits::Instance().CurrentFrame()));

    //frame start time stamp
    if (api3dIface.CfgRec_IsBenchmark()) {
      CGits::Instance().Timers().frame.Start();
    }
  }

  if (CGits::Instance().CurrentFrame() - 1 == api3dIface.CfgRec_ExitFrame() ||
      _inputListener.WasPressed(_exitHotKeyId) || Behavior().CaptureFinished() || forceExit) {
    MarkForDeletion();
  }
}

void gits::CRecorder::DrawBegin() {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  const int drawCount = gits::CGits::Instance().CurrentDrawCount();
  const int drawInFrameCount = gits::CGits::Instance().CurrentDrawInFrameCount();
  const int frameCount = gits::CGits::Instance().CurrentFrame();
  if ((api3dIface.CfgRec_SingleDraw() == drawCount ||
       (api3dIface.CfgRec_StartDraw() == drawCount && api3dIface.CfgRec_Frame() == 0) ||
       (api3dIface.CfgRec_StartDraw() == drawInFrameCount &&
        api3dIface.CfgRec_Frame() == frameCount)) &&
      drawCount != 0) {
    Start();
    Schedule(new gits::CTokenFrameNumber(CToken::ID_FRAME_START, CGits::Instance().CurrentFrame()));
  }
}

void gits::CRecorder::DrawEnd() {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  const int drawCount = gits::CGits::Instance().CurrentDrawCount();
  const int drawInFrameCount = gits::CGits::Instance().CurrentDrawInFrameCount();
  const int frameCount = gits::CGits::Instance().CurrentFrame();
  if ((api3dIface.CfgRec_SingleDraw() == drawCount ||
       (api3dIface.CfgRec_StopDraw() == drawCount && api3dIface.CfgRec_Frame() == 0) ||
       (api3dIface.CfgRec_StopDraw() == drawInFrameCount &&
        api3dIface.CfgRec_Frame() == frameCount)) &&
      drawCount != 0) {
    Stop();
    Close();
  }
}

void gits::CRecorder::CmdBufferEnd(gits::CGits::CCounter counter) {}

void gits::CRecorder::QueueSubmitEnd() {
  auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
  if (api3dIface.CfgRec_IsSubFrameMode() && api3dIface.CfgRec_IsObjectToRecord()) {
    Start();
    Stop();
    Close();
  }
}

void gits::CRecorder::KernelBegin() {
  bool hasCompute = gits::CGits::Instance().apis.HasCompute();
  if (!hasCompute) {
    return;
  }
  auto& apiComputeIface = gits::CGits::Instance().apis.IfaceCompute();
  const int kernelCount = gits::CGits::Instance().CurrentKernelCount();
  if (((apiComputeIface.CfgRec_IsSingleKernelMode() &&
        apiComputeIface.CfgRec_SingleKernel() == kernelCount) ||
       (apiComputeIface.CfgRec_IsKernelsRangeMode() &&
        apiComputeIface.CfgRec_StartKernel() == kernelCount)) &&
      kernelCount != 0) {
    Start();
  }
}

void gits::CRecorder::KernelEnd() {
  bool hasCompute = gits::CGits::Instance().apis.HasCompute();
  if (!hasCompute) {
    return;
  }
  auto& apiComputeIface = gits::CGits::Instance().apis.IfaceCompute();
  const int kernelCount = gits::CGits::Instance().CurrentKernelCount();
  if (((apiComputeIface.CfgRec_IsSingleKernelMode() &&
        apiComputeIface.CfgRec_SingleKernel() == kernelCount) ||
       (apiComputeIface.CfgRec_IsKernelsRangeMode() &&
        apiComputeIface.CfgRec_StopKernel() == kernelCount)) &&
      kernelCount != 0) {
    Stop();
    Close();
  }
}

void gits::CRecorder::EndFramePost() {
  static bool startKeysHeld = false;
  if (_isMarkedForDeletion) { // No need to proceed as it is anyway to be deleted
    return;
  }
  if (_inputListener.WasPressed(_startHotKeyId)) {
    TODO("need to code this path") // OGL
    if (!startKeysHeld) {
      Behavior().OnStartAction();
      startKeysHeld = true;
    }
  } else {
    startKeysHeld = false;
  }
}

/**
 * @brief Schedules new library token call wrapper
 *
 * Method schedules new library token call wrapper for every
 * running behavior.
 *
 * @param token New library token call wrapper.
 *
 * @return @true if a token was scheduled inside the recorder
 */
bool gits::CRecorder::Schedule(CToken* token, bool force /* = false */) {
  static unsigned tokenCount = 0;
  bool scheduled = false;

  if (!Behavior().CaptureFinished() || force) {
    Scheduler().Register(token);
    scheduled = true;
  }

  const Config& config = Config::Get();
  tokenCount++;
  if (tokenCount == config.recorder.basic.exitAfterAPICall || forceExit) {
    MarkForDeletion();
  }

  return scheduled;
}

void gits::CRecorder::Close() {
  CALL_ONCE[&] {
    Stop();
    Save();
    if (Config::Get().common.useEvents) {
      CGits::Instance().PlaybackEvents().programExit();
    }
    CRecorder::Dispose();
  };
}

void gits::CRecorder::Pause() {
  _running = false;
}

void gits::CRecorder::Continue() {
  _running = true;
}

#ifdef GITS_PLATFORM_WINDOWS

// Following is a hack to allow for graceful termination of GITS recorder.
// GITS recorder spawns threads to offload IO from application thread.
// This causes problems for cases where stream is recorded until the end
// of the application, because ExitProcess in first kills all threads
// other then current thread and then calls DLL main functions of loaded
// modules. GITS can't handle such situation correctly because threads are
// terminated without a chance to leave consistent state - in general
// cleanup after them will be impossible from DLL main function.
// To work around, we overwrite ExitProcess function with a to jump
// to our custom procedure which deals with GITS shutdown when its still
// possible cleanly.
char exitProcessHead[12];
FARPROC exitProcessAddr;

void STDCALL GitsExitProcess(UINT uExitCode) {
  if (gits::CRecorder::InstancePtr()) {
    gits::CRecorder::Instance().Close();
  }
  Log(INFO) << "Recording done";

  // Restore code of ExitProcess, now that we intercepted it.
  DWORD oldProtect;
  VirtualProtect(exitProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  std::copy_n(exitProcessHead, sizeof(exitProcessHead), (char*)exitProcessAddr);
  VirtualProtect(exitProcessAddr, 32, oldProtect, &oldProtect);

  ExitProcess(uExitCode);
}

char terminateProcessHead[12];
FARPROC terminateProcessAddr;

namespace {

void RestoreTerminateProcess();
void InstrumentTerminateProcess();

} // namespace

void STDCALL GitsTerminateProcess(_In_ HANDLE hProcess, _In_ UINT uExitCode) {
  if (GetProcessId(hProcess) == GetCurrentProcessId()) {
    if (gits::CRecorder::InstancePtr()) {
      gits::CRecorder::Instance().Close();
    }
    Log(INFO) << "Recording done by TerminateProcess() ";
  }
  RestoreTerminateProcess();
  TerminateProcess(hProcess, uExitCode);
  InstrumentTerminateProcess();
}

namespace {

void routeEntryPoint(PROC src, PROC dst) {
#if defined GITS_ARCH_X86
  const int ptrOffset = 1;
  unsigned char routingCode[] = {0xB8, 0, 0, 0, 0, 0xFF, 0xE0};
#elif defined GITS_ARCH_X64
  const int ptrOffset = 2;
  unsigned char routingCode[] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
#endif
  void* addr_value = (void*)dst;
  unsigned char* addr = (unsigned char*)&addr_value;
  std::copy(addr, addr + sizeof(void*), &routingCode[ptrOffset]);
  std::copy(routingCode, routingCode + sizeof(routingCode), (unsigned char*)src);
}

void InstrumentExitProcess() {
  HMODULE kern = GetModuleHandle("Kernel32.dll");
  exitProcessAddr = GetProcAddress(kern, "ExitProcess");

  DWORD oldProtect;
  VirtualProtect(exitProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  // Save what we will overwrite.
  std::copy_n((char*)exitProcessAddr, sizeof(exitProcessHead), exitProcessHead);
  // Route to gits cleanup function.
  routeEntryPoint(exitProcessAddr, (PROC)&GitsExitProcess);
  VirtualProtect(exitProcessAddr, 32, oldProtect, &oldProtect);
}

void InstrumentTerminateProcess() {
  HMODULE kern = GetModuleHandle("Kernel32.dll");
  terminateProcessAddr = GetProcAddress(kern, "TerminateProcess");

  DWORD oldProtect;
  VirtualProtect(terminateProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  // Save what we will overwrite.
  std::copy_n((char*)terminateProcessAddr, sizeof(terminateProcessHead), terminateProcessHead);
  // Route to gits cleanup function.
  routeEntryPoint(terminateProcessAddr, (PROC)&GitsTerminateProcess);
  VirtualProtect(terminateProcessAddr, 32, oldProtect, &oldProtect);
}

void RestoreTerminateProcess() {
  DWORD oldProtect;
  VirtualProtect(terminateProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  std::copy_n(terminateProcessHead, sizeof(terminateProcessHead), (char*)terminateProcessAddr);
  VirtualProtect(terminateProcessAddr, 32, oldProtect, &oldProtect);
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    InstrumentExitProcess();
    InstrumentTerminateProcess();
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

#else // GITS_PLATFORM_WINDOWS

void detach() {
  CALL_ONCE[] {
    if (gits::CRecorder::InstancePtr()) {
      gits::CRecorder::Instance().Close();
      Log(INFO) << "Recording done.";
    }
  };
}

#endif // GITS_PLATFORM_WINDOWS
