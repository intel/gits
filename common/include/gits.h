// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gits.h
 *
 * @brief Declaration of GITS project information class.
 *
 */

#pragma once

#include "resource_manager.h"
#include "library.h"
#include "version.h"
#include "runner.h"
#include "performance.h"
#include "timer.h"
#include "tools_lite.h"
#include "pragmas.h"
#include "apis_iface.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include <unzip.h>
#include <zip.h>
#include "nlohmann/json.hpp"

#define GITS_FILE_COMPAT_64BIT_HASHES                                                              \
  GITS_MAKE_VERSION3(1, 2, 23) //64-bit hashes in resource manager
#define GITS_EGL_IMAGE_MAPPING        GITS_MAKE_VERSION3(2, 0, 2)
#define GITS_REC_PLAY_PTR_VECTOR_MAPS GITS_MAKE_VERSION3(2, 0, 4)
#define GITS_METAL_GPU_DONE_TOKEN_MOD GITS_MAKE_VERSION3(2, 0, 5)
#define GITS_VULKAN_RETURN_VALUE_FIX  GITS_MAKE_VERSION3(2, 0, 6)
#define GITS_OPENCL_SET_USM_ARG       GITS_MAKE_VERSION4(2, 0, 8, 23)
#define GITS_TOKEN_COMPRESSION        GITS_MAKE_VERSION3(2, 0, 10)

struct lua_State;

namespace gits {
class CScheduler;
class CCodeOStream;
class CBinOStream;
class CBinIStream;
class CBehavior;
class CAction;
class CToken;

class CFile : private gits::noncopyable {
public:
  typedef std::map<unsigned, unsigned> CSkippedCalls;

private:
  CVersion _version;
  std::string _dumpDirectory;
  std::string _dataFileName;
  CSkippedCalls _skippedCalls;
  std::string _formerProperties;
  std::shared_ptr<nlohmann::ordered_json> _properties;

public:
  explicit CFile(const CVersion& version);

  const CVersion& Version() const;
  void SkippedCallAdd(unsigned id);
  const CSkippedCalls& SkippedCalls() const;

  nlohmann::ordered_json& GetProperties() const;
  std::string ReadProperties() const;

  friend CBinOStream& operator<<(CBinOStream& stream, const CFile& file);
  friend CBinIStream& operator>>(CBinIStream& stream, CFile& file);
};

//
// Events that will be fired at specified time during stream replay.
//
struct Events {
  std::function<void(int)> frameBegin;
  std::function<void(int)> frameEnd;
  std::function<void(int)> loopBegin;
  std::function<void(int)> loopEnd;
  std::function<void()> stateRestoreBegin;
  std::function<void()> stateRestoreEnd;
  std::function<void()> programExit;
  std::function<void()> programStart;
  std::function<void(const char*)> logging;
};

//
// Be wary when changing order of following members, as they depend on existence
// of each other in non-obvious ways.
//
struct StreamingContext {
  std::unique_ptr<CBinOStream> oBinStream;
  std::unique_ptr<CCodeOStream> oCodeStream;
  std::shared_ptr<CBehavior> behavior;

  std::unique_ptr<CBinIStream> iBinStream;
  std::shared_ptr<CAction> action;

  std::unique_ptr<CScheduler> scheduler;
  ~StreamingContext();
};

struct TimerSet {
  TimerSet();
  Timer program;
  Timer frame;
  Timer loading;
  Timer init;
  Timer playback;
  Timer restoration;
  std::vector<std::unique_ptr<Timer>> stateRestoreTimers;
};

/**
   * @brief Main GITS project class
   *
   * gits::CGits is a main class of GITS project. It provides information
   * about current version of a project and contains an array of registered
   * libraries.
   *
   * @note Singleton design pattern
   */
class CGits : private gits::noncopyable {
public:
  typedef std::vector<std::shared_ptr<CLibrary>> CLibraryList;

private:
  static CGits* _instance; /**< @brief singleton class instance */

  const CVersion _version;                        /**< @brief version of GITS project */
  std::unique_ptr<CResourceManager> _resources;   // has to be defined before libraries
  std::unique_ptr<CResourceManager2> _resources2; // has to be defined before libraries
  CLibraryList _libraryList;                      /**< @brief array of registered libraries */
  std::unique_ptr<CFile> _file;                   /**< @brief GITS file connected data */
  std::unique_ptr<StreamCompressor> _compressor;
  CRunner _runner;
  FrameTimeSheet _timeSheet;
  int _currentThreadId;
  bool _multithreadedApp;
  int _kernelCounter;
  uint32_t _cmdListCounter;
  uint32_t _cmdQueueExecCounter;
  uint32_t _drawCounter;
  uint32_t _drawInFrameCounter;
  std::atomic<bool> _finished;
  zipFile _glProgramsZipFile;
  unzFile _glProgramsUnZipFile;
  struct ZippedFileInfo {
    unz_file_pos location;
    unsigned size;
  };
  std::unordered_map<std::string, ZippedFileInfo> _glProgramsLocationsInZipFile;

  uint32_t _frameNo;
  bool _restoringState;
  bool _ccodePreRecord;
  bool _ccodeStateRestore;
  std::vector<std::function<void()>> _endPlaybackEvents;
  std::vector<std::function<void()>> _luaFunctionsRegistrators;
  Events _playbackEvents;
  std::shared_ptr<lua_State> _lua;
  Task<Image> _imageWriter;
  StreamingContext* _sc;
  TimerSet _timers;

  uint64_t _currentLocalMemoryUsage;
  uint64_t _maxLocalMemoryUsage;

  std::unordered_map<void*, uint64_t> _ptrToOrderedId;

  CGits();
  CGits(uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3);
  ~CGits();

public:
  CGits(const CGits& other) = delete;
  CGits& operator=(const CGits& other) = delete;
  static CGits& Instance() {
    if (!_instance) {
      _instance = new CGits();
    }
    return *_instance;
  }
  static CGits* InstancePtr() {
    return _instance;
  }
  void Dispose();
  void RegisterEndPlaybackEvent(std::function<void()> f) {
    _endPlaybackEvents.push_back(f);
  }
  void ProcessEndPlaybackEvents() {
    for (auto& f : _endPlaybackEvents) {
      f();
    }
  }

  void RegisterLuaFunctionsRegistrator(std::function<void()> f) {
    _luaFunctionsRegistrators.push_back(f);
  }
  void ProcessLuaFunctionsRegistrators() {
    for (auto& f : _luaFunctionsRegistrators) {
      f();
    }
  }

  void RegisterPlaybackEvents(std::shared_ptr<lua_State>& L, const Events& e) {
    _lua = L;
    _playbackEvents = e;
  }
  const Events& PlaybackEvents() const {
    return _playbackEvents;
  }
  std::shared_ptr<lua_State> GetLua() const {
    return _lua;
  }

  void Register(std::shared_ptr<CLibrary> library);
  void Register(std::unique_ptr<CFile> file);

  void ResourceManagerInit(const std::filesystem::path& dump_dir);
  void ResourceManagerDispose() {
    _resources.reset();
    _resources2.reset();
  }

  void CompressorInit(CompressionType compressionType);

  void CurrentThreadId(int threadId);
  int CurrentThreadId() const {
    return _currentThreadId;
  }
  bool MultithreadedApp() const {
    return _multithreadedApp;
  }
  uint32_t CurrentDrawCount() const {
    return _drawCounter;
  }
  uint32_t CurrentDrawInFrameCount() const {
    return _drawInFrameCounter;
  }
  bool Finished() {
    return _finished.load();
  }
  void DrawCountUp() {
    ++_drawCounter;
    ++_drawInFrameCounter;
  }

  int CurrentKernelCount() const {
    return _kernelCounter;
  }
  void KernelCountUp() {
    ++_kernelCounter;
  }

  uint32_t CurrentCommandListCount() const {
    return _cmdListCounter;
  }
  void CommandListCountUp() {
    ++_cmdListCounter;
  }

  uint32_t CurrentCommandQueueExecCount() const {
    return _cmdQueueExecCounter;
  }
  void CommandQueueExecCountUp() {
    ++_cmdQueueExecCounter;
  }

  uint32_t CurrentFrame() const {
    return _frameNo;
  }
  void FrameCountUp() {
    ++_frameNo;
    _drawInFrameCounter = 0;
  }

  void StateRestoreStarted() {
    _restoringState = true;
  }
  void StateRestoreFinished() {
    _restoringState = false;
  }
  void SetPlayerFinish() {
    _finished.store(true);
  }
  bool IsStateRestoration() {
    return _restoringState;
  }

  void CCodePreRecordStart() {
    _ccodePreRecord = true;
  }
  void CCodePreRecordEnd() {
    _ccodePreRecord = false;
  }
  bool IsCCodePreRecord() {
    return _ccodePreRecord;
  }

  void CCodeStateRestoreStart() {
    _ccodeStateRestore = true;
  }
  void CCodeStateRestoreEnd() {
    _ccodeStateRestore = false;
  }
  bool IsCCodeStateRestore() {
    return _ccodeStateRestore;
  }

  void AddLocalMemoryUsage(const size_t& size);
  void SubtractLocalMemoryUsage(const size_t& size);
  size_t GetMaxLocalMemoryUsage() const;

  zipFile OpenZipFileGLPrograms();
  void CloseZipFileGLPrograms();

  void OpenUnZipFileGLPrograms();
  void ReadGlProgramFromUnZipFile(std::string progname, std::string& text);
  void CloseUnZipFileGLPrograms();

  uint64_t GetOrderedIdFromPtr(void* ptr);

  void SetSC(StreamingContext* sc);
  const StreamingContext* GetSC() const;

  TimerSet& Timers() {
    return _timers;
  }
  class CCounter {
  public:
    std::vector<uint64_t> countersTable;
    void operator++(int);
    CCounter();
    CCounter(std::initializer_list<uint64_t> init);
    std::vector<uint64_t> GetCountersTable() {
      return countersTable;
    }
  };
  struct VkCounters {
  private:
    uint64_t queueSubmitCounter;
    uint64_t imageCounter;
    uint64_t bufferCounter;

  public:
    VkCounters() : queueSubmitCounter(0), imageCounter(0), bufferCounter(0) {}
    void QueueSubmitCountUp() {
      queueSubmitCounter++;
    }
    uint64_t CurrentQueueSubmitCount() {
      return queueSubmitCounter;
    }
    void ImageCountUp() {
      imageCounter++;
    }
    uint64_t CurrentImageCount() {
      return imageCounter;
    }
    void BufferCountUp() {
      bufferCounter++;
    }
    uint64_t CurrentBufferCount() {
      return bufferCounter;
    }
  } vkCounters;
  CResourceManager& ResourceManager() {
    assert(_resources);
    return *_resources;
  }
  CResourceManager2& ResourceManager2() {
    assert(_resources2);
    return *_resources2;
  }
  StreamCompressor& GitsStreamCompressor() {
    return *_compressor;
  }
  FrameTimeSheet& TimeSheet() {
    return _timeSheet;
  }
  //destroys the data argument
  void WriteImage(const std::string& filename,
                  size_t width,
                  size_t height,
                  bool hasAlpha,
                  std::vector<uint8_t>& data,
                  bool flip = true,
                  bool isBGR = false,
                  bool isSRGB = false);

  CLibraryList::const_iterator LibraryBegin() const {
    return begin(_libraryList);
  }
  CLibraryList::const_iterator LibraryEnd() const {
    return end(_libraryList);
  }

  CLibrary& Library(CLibrary::TId id);
  CToken* TokenCreate(CId id);

  CFile& File() const;

  const CVersion& Version() const {
    return _version;
  }

  CRunner& Runner() {
    return _runner;
  }
  bool traceGLAPIBypass;
  ApisIface apis;

  friend std::ostream& operator<<(std::ostream& stream, const CGits& g);
  friend CBinOStream& operator<<(CBinOStream& stream, const CGits& g);
  friend CBinIStream& operator>>(CBinIStream& stream, CGits& g);
};

bool stream_older_than(uint64_t version);
} // namespace gits
