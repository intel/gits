// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "token.h"

#ifdef WITH_OPENCL
#include "openclHelperFunctions.h"
#endif

#if defined(GITS_PLATFORM_LINUX)
#include <sys/stat.h>
#include <csignal>
#endif

#include "openglRecorderWrapper.h"

#include <memory>
#include <filesystem>
#include <fstream>

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
  if (gits::CGits::Instance().apis.HasCompute() && Config::Get().common.recorder.enabled) {
    const auto& computeIface = gits::CGits::Instance().apis.IfaceCompute();
    computeIface.MemorySnifferUninstall();
    computeIface.PrintMaxLocalMemoryUsage();
  }
  if (Config::Get().common.recorder.enabled) {
    CGits::Instance().Dispose();
  }
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
      _runningStarted(false),
      _isMarkedForDeletion(false),
      _exitHotKeyId(0),
      _startHotKeyId(0) {
  CGits& inst = CGits::Instance();
  const Config& config = Config::Get();

  inst.SetSC(&this->_sc);

  Log(INFO) << "-----------------------------------------------------";
  Log(INFO) << " GITS Recorder (" << inst.Version() << ")";
  Log(INFO) << "-----------------------------------------------------";

  inst.GetMessageBus().subscribe({PUBLISHER_PLUGIN, TOPIC_LOG}, [](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LogMessage>(m);
    if (msg) {
      CLog(msg->getLevel(), NORMAL) << msg->getText();
    }
  });

#ifdef GITS_PLATFORM_WINDOWS
  // handling signals
  SignalsHandler();
#endif
  forceExit = false;

  // create file data and register it in GITS
  if (config.common.recorder.enabled) {
    auto& outputpath = config.common.recorder.dumpPath;
    std::filesystem::create_directories(outputpath);
#if defined(GITS_PLATFORM_X11)
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = InterruptHandler;
    sigaction(Config::Get().common.recorder.exitSignal, &action, nullptr);
#endif
    inst.CompressorInit(config.common.recorder.compression.type);
    inst.ResourceManagerInit(config.common.recorder.dumpPath);
  }

  // register behaviors
  const bool captureOnKeypress =
      inst.apis.Has3D() && !inst.apis.Iface3D().CfgRec_StartKeys().empty();
  Log(INFO) << "Recorder mode: ";
  std::ostringstream message;
  bool recEnabled = config.common.recorder.enabled;
  if (!recEnabled) {
    message << " - Off";
  } else {
    if (inst.apis.Has3D()) {
      const auto& api3dIface = inst.apis.Iface3D();
      if (api3dIface.CfgRec_IsAllMode()) {
        message << " - All";
      } else if (api3dIface.CfgRec_IsFramesMode()) {
        message << " - Frames ";
        message << " Start:" << api3dIface.CfgRec_StartFrame()
                << " Stop:" << api3dIface.CfgRec_StopFrame();
        if (captureOnKeypress) {
          message << "\n - Initiate capture on user request. ";
        }
      } else if (api3dIface.CfgRec_IsSingleDrawMode()) {
        message << " - SingleDraw ";
        message << " Number:" << api3dIface.CfgRec_SingleDraw();
      } else if (api3dIface.CfgRec_IsDrawsRangeMode()) {
        message << " - DrawsRange ";
        message << " Start:" << api3dIface.CfgRec_StartDraw()
                << " Stop:" << api3dIface.CfgRec_StopDraw()
                << " Frame:" << api3dIface.CfgRec_Frame();
      }
    }

    if (inst.apis.HasCompute()) {
      const auto& apiCompute = inst.apis.IfaceCompute();
      if (apiCompute.CfgRec_IsAllMode()) {
        message << " - All";
      } else if (apiCompute.CfgRec_IsSingleKernelMode()) {
        message << " - SingleKernel ";
        if (apiCompute.Api() != gits::ApisIface::TApi::LevelZero) {
          message << " Number:" << apiCompute.CfgRec_SingleKernel();
        } else {
          message << " Number: " << apiCompute.CfgRec_StartCommandQueueSubmit() << "/"
                  << apiCompute.CfgRec_StartCommandList() << "/" << apiCompute.CfgRec_StartKernel();
        }
      } else if (apiCompute.CfgRec_IsKernelsRangeMode()) {
        message << " - KernelsRange";
        if (apiCompute.Api() != gits::ApisIface::TApi::LevelZero) {
          message << " Start:" << apiCompute.CfgRec_StartKernel()
                  << " Stop:" << apiCompute.CfgRec_StopKernel();
        } else {
          message << " Start: " << apiCompute.CfgRec_StartCommandQueueSubmit() << "/"
                  << apiCompute.CfgRec_StartCommandList() << "/" << apiCompute.CfgRec_StartKernel()
                  << " Stop: " << apiCompute.CfgRec_StopCommandQueueSubmit() << "/"
                  << apiCompute.CfgRec_StopCommandList() << "/" << apiCompute.CfgRec_StopKernel();
        }
      }
    }
  }
  Log(INFO) << message.str();
  Register(std::unique_ptr<CBehavior>(new CBehavior(*this, captureOnKeypress)));

  if (config.common.recorder.forceDumpOnError) {
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
  if (recEnabled) {
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
  try {
    const Config& config = Config::Get();

    if (gits::CGits::Instance().apis.Has3D() && config.common.recorder.benchmark) {
      if (config.IsRecorder() && config.common.recorder.enabled) {
        std::ofstream out_file(config.common.recorder.dumpPath / "benchmark.csv");
        CGits::Instance().TimeSheet().OutputTimeData(out_file, true);
      }
    }

    if (config.common.recorder.forceDumpOnError) {
      Exception::Callback(nullptr, nullptr);
    }
#ifdef GITS_PLATFORM_WINDOWS
    if (config.common.recorder.closeAppOnStopRecording) {
      CloseApplicationOnStopRecording();
    }
#endif
    Close();
    Save();

    for (auto& e : _disposeEvents) {
      e();
    }

    _instance = nullptr;
  } catch (...) {
    topmost_exception_handler("CRecorder::~CRecorder");
  }
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
  CGits::Instance().Register(std::move(library));
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

  if (Config::Get().common.shared.useEvents) {
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

  auto& cfg = Config::Get();
  auto& cmm = cfg.common;
  auto& rec = cmm.recorder;
  bool recEnabled = rec.enabled;

  if (gits::CGits::Instance().apis.Has3D()) {
    const auto& api3dIface = gits::CGits::Instance().apis.Iface3D();
    _exitHotKeyId = _inputListener.AddHotKey(rec.exitKeys);
    _inputListener.AddHotKeyEvent(_exitHotKeyId, ExitHotKeyPressed);
    _startHotKeyId = _inputListener.AddHotKey(api3dIface.CfgRec_StartKeys());
    bool useMessageLoop = false;
#ifdef GITS_PLATFORM_WINDOWS
    useMessageLoop = rec.windowsKeyHandling == TWindowsKeyHandling::MESSAGE_LOOP;
#endif
    _inputListener.StartHotKeyListener(useMessageLoop);
  }

  if (recEnabled) {
    _sc.scheduler.reset(new CScheduler(rec.tokenBurst, rec.tokenBurstNum));
  }

  auto& outputpath = rec.dumpPath;
  auto filePath = (outputpath / "stream").string();
  if (cfg.dumpBinary() && rec.enabled) {
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

  if (Config::Get().dumpCCode()) {
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
  CGits& inst = CGits::Instance();

  // If you are looking for OpenCL state restoration, it is implemented in OpenCL wrapper, in clEnqueueNDRangeKernel.
  // If we started capture on frame other than the first one or recording mode is 'SingleDraw', state restoration is needed.
  bool shouldRestore3DState = false;
  if (inst.apis.Has3D()) {
    const auto& api3dIface = inst.apis.Iface3D();
    shouldRestore3DState =
        (api3dIface.CfgRec_IsFramesMode() && inst.CurrentFrame() != 1) ||
        (api3dIface.CfgRec_IsSingleDrawMode() && api3dIface.CfgRec_SingleDraw() != 1) ||
        api3dIface.CfgRec_IsDrawsRangeMode() || api3dIface.CfgRec_IsCmdBufferMode() ||
        api3dIface.CfgRec_IsRenderPassMode() || api3dIface.CfgRec_IsEncodersRangeMode() ||
        api3dIface.CfgRec_IsSubEncodersRangeMode() || api3dIface.CfgRec_IsQueueSubmitMode() ||
        api3dIface.CfgRec_IsBlitRangeMode() || api3dIface.CfgRec_IsDispatchRangeMode();
  }
  bool shouldRestoreComputeState = false;
  if (inst.apis.HasCompute()) {
    const auto& apiComputeIface = inst.apis.IfaceCompute();
    shouldRestoreComputeState =
        apiComputeIface.CfgRec_IsSingleKernelMode() || apiComputeIface.CfgRec_IsKernelsRangeMode();
  }
  const bool stateNeedsRestoring = shouldRestore3DState || shouldRestoreComputeState;

  if (stateNeedsRestoring && !Config::Get().IsPlayer()) {
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, inst.CurrentFrame()));
    for (auto it = inst.LibraryBegin(); it != inst.LibraryEnd(); ++it) {
      // schedule current library state only if not init frame number
      auto state((*it)->StateCreate());
      if (state) {
        inst.StateRestoreStarted();
        state->Prepare();
        state->Get();
        Scheduler().Register(new CTokenFrameNumber(CToken::ID_INIT_START, inst.CurrentFrame()));
        state->Schedule(Scheduler());
        Scheduler().Register(new CTokenFrameNumber(CToken::ID_INIT_END, inst.CurrentFrame()));
        inst.StateRestoreFinished();
        if (inst.apis.Has3D()) {
          inst.apis.Iface3D().Rec_StateRestoreFinished();
        }
        if (inst.apis.HasCompute()) {
          inst.apis.IfaceCompute().Rec_StateRestoreFinished();
        }
        state->PostSchedule(Scheduler());
        delete state;
      }
    }
  }

  bool isVulkan = inst.apis.Has3D() && inst.apis.Iface3D().Api() == inst.apis.Vulkan;
  // First frame is a special case.
  // TODO: Make it so that any frame number logic is not necessary in compute-only streams.
  if (inst.CurrentFrame() == 1 && (!Config::Get().dumpCCode() || !isVulkan)) {
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_PRE_RECORD_END, inst.CurrentFrame()));
    Scheduler().Register(new CTokenFrameNumber(CToken::ID_FRAME_START, 1));
    //first frame start time stamp
    if (inst.apis.Has3D() && Config::Get().common.recorder.benchmark) {
      inst.Timers().frame.Start();
    }
  }

  // update running flag
  //if (Config::Get().recorder.basic.enabled)

#ifdef GITS_PLATFORM_WINDOWS
  exitEventHandler.Start();
#endif // GITS_PLATFORM_WINDOWS

  _running = true;
  _runningStarted = true;
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
    bool modeCondition3D = false;
    bool modeConditionCompute = false;
    if (inst.apis.Has3D()) {
      const auto& api3dIface = inst.apis.Iface3D();
      modeCondition3D = api3dIface.CfgRec_IsFramesMode() || api3dIface.CfgRec_IsSingleDrawMode() ||
                        api3dIface.CfgRec_IsDrawsRangeMode();
    }
    if (inst.apis.HasCompute()) {
      const auto& apiComputeIface = inst.apis.IfaceCompute();
      modeConditionCompute = apiComputeIface.CfgRec_IsSingleKernelMode() ||
                             apiComputeIface.CfgRec_IsKernelsRangeMode();
    }

    if (modeCondition3D || modeConditionCompute) {
      if (inst.apis.HasCompute()) {
        inst.apis.IfaceCompute().MemorySnifferUninstall();
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
    Scheduler().Register(new gits::CTokenFrameNumber(CToken::ID_CCODE_FINISH, inst.CurrentFrame()));
  }

#ifdef GITS_PLATFORM_WINDOWS
  exitEventHandler.Stop();
#endif // GITS_PLATFORM_WINDOWS

  _running = false;
  _runningStarted = false;
}

std::recursive_mutex& gits::CRecorder::GetMutex() {
  return _mutex;
}

void gits::CRecorder::TrackThread(gits::ApisIface::TApi api) {
  static int generatedThreadId = 0;
  static int previousThreadId = 0;
  static thread_local int currentThreadId = -1;
  if (currentThreadId < 0) {
    currentThreadId = generatedThreadId;
    generatedThreadId++;
  }
  if (currentThreadId != previousThreadId) {
    if (api == gits::ApisIface::OpenGL) {
      Scheduler().Register(new CTokenMakeCurrentThread(currentThreadId));
    }
#ifdef WITH_OPENCL
    else if (api == gits::ApisIface::OpenCL) {
      Scheduler().Register(new gits::OpenCL::CGitsClTokenMakeCurrentThread(currentThreadId));
    }
#endif
    else {
      throw ENotImplemented("Thread tracking not implemented for this API.");
    }
    previousThreadId = currentThreadId;
  }
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
  auto& cfg = Config::Get();
  bool recEnabled = cfg.common.recorder.enabled;
  if (!recEnabled) {
    return;
  }

  CALL_ONCE[&] {
    Scheduler().WriteAll();
  };
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
  CGits& inst = CGits::Instance();
  if (inst.apis.Has3D()) {
    const auto& api3dIface = inst.apis.Iface3D();

    if (Running()) {
      Schedule(new CTokenFrameNumber(CToken::ID_FRAME_END, inst.CurrentFrame()));

      if (api3dIface.CfgRec_EndFrameSleep() > 0) {
        sleep_millisec(api3dIface.CfgRec_EndFrameSleep());
      }

      //frame end time stamp
      if (Config::Get().common.recorder.benchmark) {
        inst.TimeSheet().add_frame_time("stamp", inst.Timers().program.Get());
        inst.TimeSheet().add_frame_time("cpu", inst.Timers().frame.Get());
      }

      if (IsMarkedForDeletion()) {
        Close();
        return;
      }
    }

    Behavior().OnFrameEnd();

    // increment current frame number
    inst.FrameCountUp();
    if (api3dIface.CfgRec_IsFramesMode()) {
      if (_running && !Behavior().ShouldCapture()) {
        _running = false;
        _runningStarted = false;
      }
      if (!_running && Behavior().ShouldCapture()) {
        Start();
      }
    }

    // we are at the next frame
    if (Running()) {
      Schedule(new gits::CTokenFrameNumber(CToken::ID_FRAME_START, inst.CurrentFrame()));

      //frame start time stamp
      if (Config::Get().common.recorder.benchmark) {
        inst.Timers().frame.Restart();
      }
    }

    if (inst.CurrentFrame() - 1 == api3dIface.CfgRec_ExitFrame() ||
        _inputListener.WasPressed(_exitHotKeyId) || Behavior().CaptureFinished() || forceExit) {
      MarkForDeletion();
    }
  }
}

void gits::CRecorder::DrawBegin() {
  const CGits& inst = CGits::Instance();
  if (inst.apis.Has3D()) {
    const auto& api3dIface = inst.apis.Iface3D();
    const int drawCount = inst.CurrentDrawCount();
    const int drawInFrameCount = inst.CurrentDrawInFrameCount();
    const int frameCount = inst.CurrentFrame();
    if ((api3dIface.CfgRec_SingleDraw() == drawCount ||
         (api3dIface.CfgRec_StartDraw() == drawCount && api3dIface.CfgRec_Frame() == 0) ||
         (api3dIface.CfgRec_StartDraw() == drawInFrameCount &&
          api3dIface.CfgRec_Frame() == frameCount)) &&
        drawCount != 0) {
      Start();
      Schedule(new gits::CTokenFrameNumber(CToken::ID_FRAME_START, inst.CurrentFrame()));
    }
  }
}

void gits::CRecorder::DrawEnd() {
  const CGits& inst = CGits::Instance();
  if (inst.apis.Has3D()) {
    const auto& api3dIface = inst.apis.Iface3D();
    const int drawCount = inst.CurrentDrawCount();
    const int drawInFrameCount = inst.CurrentDrawInFrameCount();
    const int frameCount = inst.CurrentFrame();
    if ((api3dIface.CfgRec_SingleDraw() == drawCount ||
         (api3dIface.CfgRec_StopDraw() == drawCount && api3dIface.CfgRec_Frame() == 0) ||
         (api3dIface.CfgRec_StopDraw() == drawInFrameCount &&
          api3dIface.CfgRec_Frame() == frameCount)) &&
        drawCount != 0) {
      Stop();
      Close();
    }
  }
}

void gits::CRecorder::CmdBufferEnd(gits::CGits::CCounter counter) {}

void gits::CRecorder::QueueSubmitEnd() {
  const CGits& inst = CGits::Instance();
  if (inst.apis.Has3D()) {
    const auto& api3dIface = inst.apis.Iface3D();
    if (api3dIface.CfgRec_IsSubFrameMode() && api3dIface.CfgRec_IsObjectToRecord()) {
      Start();
      Stop();
      Close();
    }
  }
}

void gits::CRecorder::KernelBegin() {
  const CGits& inst = CGits::Instance();
  if (!inst.apis.HasCompute()) {
    return;
  }
  const auto& apiComputeIface = inst.apis.IfaceCompute();
  const int kernelCount = inst.CurrentKernelCount();
  if (((apiComputeIface.CfgRec_IsSingleKernelMode() &&
        apiComputeIface.CfgRec_SingleKernel() == kernelCount) ||
       (apiComputeIface.CfgRec_IsKernelsRangeMode() &&
        apiComputeIface.CfgRec_StartKernel() == kernelCount)) &&
      kernelCount != 0) {
    Start();
  }
}

void gits::CRecorder::KernelEnd() {
  const CGits& inst = CGits::Instance();
  if (!inst.apis.HasCompute()) {
    return;
  }
  const auto& apiComputeIface = inst.apis.IfaceCompute();
  const int kernelCount = inst.CurrentKernelCount();
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
  if (tokenCount == config.common.recorder.exitAfterAPICall || forceExit) {
    MarkForDeletion();
  }

  return scheduled;
}

void gits::CRecorder::Close() {
  CALL_ONCE[&] {
    Stop();
    Save();
    if (Config::Get().common.shared.useEvents) {
      CGits::Instance().PlaybackEvents().programExit();
    }
    CRecorder::Dispose();
  };
}

void gits::CRecorder::Pause() {
  _running = false;
}

void gits::CRecorder::Continue() {
  _running = _runningStarted;
}

#if defined(GITS_PLATFORM_LINUX)
void detach() {
  CALL_ONCE[] {
    if (gits::CRecorder::InstancePtr()) {
      gits::CRecorder::Instance().Close();
      Log(INFO) << "Recording done.";
    }
  };
}
#endif
