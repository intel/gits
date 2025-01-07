// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools.h
 *
 * @brief Common tools for GITS project.
 *
 */

#pragma once

#include "tools_lite.h"
#include "exception.h"
#include "pragmas.h"
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
#include "message_pump.h"
#endif
#include "dynamic_linker.h"

#include <set>
#include <map>
#include <deque>
#include <filesystem>
#include <climits>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#ifdef GITS_PLATFORM_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#ifndef BUILD_FOR_CCODE
#include "nlohmann/json.hpp"
#endif
#include <zstd.h>
#include <lz4.h>

#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG 1
#endif
#endif
#include <cassert>

#include "config.h"

namespace gits {
struct Config;
bool SavePng(const std::string& filename,
             size_t width,
             size_t height,
             bool hasAlpha,
             const void* bytes,
             bool flip = true,
             bool sRGB = false,
             bool bgr = false);

#ifndef BUILD_FOR_CCODE
void SaveJsonFile(const nlohmann::ordered_json& json, const std::filesystem::path& path);
#endif

void CheckMinimumAvailableDiskSize();

uint32_t HwCrc32ishHash(const void* data, size_t size, uint32_t hash);
uint64_t ComputeHash(const void* data,
                     size_t size,
                     THashType type,
                     bool hashPartially,
                     uint32_t partialHashCutoff,
                     uint32_t partialHashChunks,
                     uint32_t partialHashRatio);
uint64_t ComputeHash(const void* data, size_t size, THashType type);

std::string CommandOutput(const std::string& command, bool isRecorder);

void fast_exit(int);

template <class T>
size_t GetTermArraySize(const T* arr, const T terminator, const int term_pos = 1) {
  for (int i = 0; i >= 0; i = i + term_pos) {
    if (arr[i] == terminator) {
      return i;
    }
  }
  //Counter on index i turned - array malformated.
  throw std::runtime_error(EXCEPTION_MESSAGE);
}

template <class T>
size_t GetNullTermArraySize(const T* arr, const int term_pos = 1) {
  return GetTermArraySize(arr, (const T)0, term_pos);
}

//GetPropertyVal - returns property value from null terminated key/val array (key1, val1, key2, val2,...)
template <class T>
T GetPropertyVal(const T* properties, int prop) {
  auto ptr = properties;
  while (*ptr != 0) {
    if (*ptr == (T)prop) {
      return *(++ptr);
    }
    ptr = ptr + 2;
  }
  return 0;
}

//AddPropertyOption - Adds or updates bitfield property value in null terminated key/val array (key1, val1, key2, val2,...)
template <class T, class Y, class Z>
std::vector<T> AddPropertyOption(const T* properties, Y property, Z optionBitfield) {
  auto ptrBegin = properties;
  auto ptrEnd =
      properties + GetNullTermArraySize(properties, 2) + 1; //+ 1 - including null terminator
  std::vector<T> propertiesVec(ptrBegin, ptrEnd);

  //Update property
  auto iter = propertiesVec.begin();
  for (; iter != propertiesVec.end(); iter++) {
    auto index = iter - propertiesVec.begin();
    if (index % 2 == 0 && *iter == (T)property) {
      iter++;
      *iter = *iter | optionBitfield;
      break;
    }
  }

  //Add property
  if (iter == propertiesVec.end()) {
    //Inject property in front of null - the last element in the array
    propertiesVec.insert(iter - 1, property);
    iter = propertiesVec.end();
    propertiesVec.insert(iter - 1, optionBitfield);
  }

  return propertiesVec;
}

const std::filesystem::path GetDumpPath(const Config& cfg);

#ifdef GITS_PLATFORM_X11
pid_t GetPIDFromWindow(Display* display, Window w);
bool SearchForWindow(Display* display, pid_t pid, Window w, win_ptr_t& returnWindow);
#endif
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
win_ptr_t GetWindowHandle();
#endif

std::string GetWindowsProcessName(int processID);
#if defined GITS_PLATFORM_LINUX
std::string GetLinuxProcessName(pid_t processID);
#endif
std::string GetLinuxProcessNamePath();

template <class T>
int get_product_cost(const T&) {
  return 1;
}

// Check if a numeric value can be cast to a 32 bit signed/unsigned integer
// representation.
template <class SRC>
uint32_t ensure_unsigned32bit_representible(SRC value) {
  // Only interested in numeric values. So explicit casting to integral
  // types. Note that any negative values will also cause an exception.
  // If long long (introduced in C99) is not supported on a particular
  // compiler/platform then specific code using an alternate appropriate
  // suffix for the constant should be put here with the existing
  // implementation being the default case.
  if (!(static_cast<uint64_t>(value) & 0xFFFFFFFF00000000ull)) {
    return static_cast<uint32_t>(value);
  } else {
    Log(INFO) << "Value cannot be represented as a 32 bit unsigned integer!";
    return static_cast<uint32_t>(value);
    //throw EOperationFailed(EXCEPTION_MESSAGE); Workaround for SpecViewPerf Catia
  }
}
template <class SRC>
int32_t ensure_signed32bit_representible(SRC value) {
  // Only interested in numeric values. So explicit casting to integral types.
  if (static_cast<int64_t>(value) >= std::numeric_limits<int32_t>::min() &&
      static_cast<int64_t>(value) <= std::numeric_limits<int32_t>::max()) {
    return static_cast<int32_t>(value);
  } else {
    Log(INFO) << "Value cannot be represented as a 32 bit signed integer!";
    return static_cast<int32_t>(value);
    //throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

unsigned int stoui(const std::string& str);
std::vector<std::string> GetStringsWithRegex(std::string src,
                                             const char* regex,
                                             const char* rmRegex);

std::vector<std::string> GetIncludePaths(const char* buildOptions);

void CreateHeaderFiles(const std::vector<std::string>& sourceNamesToScan,
                       const std::vector<std::string>& searchPaths,
                       std::set<std::string>& alreadyCreatedHeaders,
                       const bool includeMainFiles = false);

} // namespace gits

template <class Product>
class ProducerConsumer : private gits::noncopyable {
public:
  ProducerConsumer(int max_cost_capacity = 5)
      : exhausted_(false), cost_capacity_(max_cost_capacity) {
    if (cost_capacity_ < 1) {
      cost_capacity_ = 1;
    }
  }
  ~ProducerConsumer() = default;
  ProducerConsumer(const ProducerConsumer&) = delete;
  ProducerConsumer& operator=(const ProducerConsumer&) = delete;
  ProducerConsumer(ProducerConsumer&&) = delete;
  ProducerConsumer& operator=(ProducerConsumer&&) = delete;
  // Blocks until product is available. Returns false when
  // producer is exhausted - no product is generated in such
  // event.
  bool consume(Product& product) {
    {
      std::unique_lock<std::mutex> lock(mutex_);

      // Wait for loader thread to load up a chunk.
      while (products_.empty()) {
        if (exhausted_) {
          return false;
        }
        cond_.wait(lock);
      }

      products_[0].swap(product);
      products_.pop_front();
      cost_capacity_ += gits::get_product_cost(product);

      // No need to notify producer thread if it will block
      // anyways. Only notify him if queue is reasonably empty.
      if (cost_capacity_ <= 0) {
        return true;
      }
    }

    cond_.notify_one();
    return true;
  }

  // If queue accepted the product, returns true.
  bool produce(Product& product) {
    std::unique_lock<std::mutex> lock(mutex_);

    // Can't accept any more products ever.
    if (exhausted_) {
      return false;
    }

    cost_capacity_ -= gits::get_product_cost(product);
    products_.resize(products_.size() + 1);
    products_.back().swap(product);

    // We have done our job in this thread. Notify
    // our client that the data is loaded.
    cond_.notify_one();

    // We have accepted the product, but now can't accept anymore.
    // Wait till queue has more room in it. This has to be checked
    // after product is added to queue, so that products exceeding
    // cost_capacity is ever added to queue.
    while (cost_capacity_ <= 0 && !exhausted_) {
      cond_.wait(lock);
    }

    return true;
  }

  void break_pipe() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      exhausted_ = true;
    }

    cond_.notify_one();
  }

private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::deque<Product> products_;
  bool exhausted_;
  int cost_capacity_;
};

template <class WorkUnit>
class TaskFunction {
public:
  typedef ProducerConsumer<WorkUnit> Queue;

  template <class T>
  TaskFunction(Queue& q, T func) : function_(std::move(func)), queue_(q) {}
  TaskFunction(const TaskFunction& other) : function_(other.function_), queue_(other.queue_) {}
  ~TaskFunction() = default;
  void operator()() {
    function_(queue_);
  }
  TaskFunction& operator=(const TaskFunction& other) = delete;

private:
  std::function<void(Queue&)> function_;
  Queue& queue_;
};

template <class WorkUnit>
class Task {
public:
  explicit Task(int max_products = 5) : queue_(max_products) {}
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  Task(Task&&) = delete;
  Task& operator=(Task&&) = delete;
  template <class T>
  void start(T&& func) {
    assert(thread_.size() == 0);
    thread_.emplace_back(TaskFunction<WorkUnit>(queue_, std::forward<T>(func)));
  }
  bool running() const {
    return thread_.size() != 0;
  }
  void finish() {
    queue_.break_pipe();
    for (auto& t : thread_) {
      if (t.joinable()) {
        t.join();
      }
    }
    thread_.clear();
  }

  ProducerConsumer<WorkUnit>& queue() {
    return queue_;
  }
  ~Task() {
    try {
      finish();
    } catch (...) {
      topmost_exception_handler("Task::~Task");
    }
  }

private:
  std::vector<std::thread> thread_;
  ProducerConsumer<WorkUnit> queue_;
};

namespace gits {
/**
  * The class represents the parameters / action of SavePng function.
  * The objects of the class are produced by main thread and consumed by i/o thread.
  */
class Image {
public:
  Image() : _width(0), _height(0), _hasAlpha(false), _flip(false), _isBGR(false), _isSRGB(false) {}

  Image(const std::string& fileName,
        size_t width,
        size_t height,
        bool hasAlpha,
        std::vector<uint8_t>& data,
        bool flip,
        bool isBGR,
        bool isSRGB)
      : _filename(fileName),
        _width(width),
        _height(height),
        _hasAlpha(hasAlpha),
        _flip(flip),
        _isBGR(isBGR),
        _isSRGB(isSRGB) {
    _data.swap(data);
  }

  void swap(Image& img) {
    std::swap(_width, img._width);
    std::swap(_height, img._height);
    std::swap(_hasAlpha, img._hasAlpha);
    _filename.swap(img._filename);
    _data.swap(img._data);
    std::swap(_flip, img._flip);
    std::swap(_isBGR, img._isBGR);
    std::swap(_isSRGB, img._isSRGB);
  }

  void Write() {
    if (_data.size() != 0) {
      SavePng(_filename, _width, _height, _hasAlpha, &_data[0], _flip, _isBGR, _isSRGB);
    }
  }

private:
  std::string _filename;
  size_t _width;
  size_t _height;
  bool _hasAlpha;
  std::vector<uint8_t> _data;
  bool _flip;
  bool _isBGR;
  bool _isSRGB;
};

/*
  * The class responsible for consuming png images from i/o thread.
  */
class ImageWriter {
public:
  void operator()(ProducerConsumer<Image>& queue) {
    try {
      Image image;
      while (queue.consume(image)) {
        image.Write();
      }
    } catch (std::exception& e) {
      Log(ERR) << "Image writer thread failed: " << e.what();
      fast_exit(1);
    } catch (...) {
      Log(ERR) << "Unknown error in image writer thread";
      fast_exit(1);
    }
  }
  // Default constructor
  ImageWriter() = default;

  // User-defined copy constructor
  ImageWriter(const ImageWriter& other) = delete;

  // User-defined copy assignment operator
  ImageWriter& operator=(const ImageWriter& other) = delete;

  // User-defined move constructor
  ImageWriter(ImageWriter&& other) = default;

  // User-defined move assignment operator
  ImageWriter& operator=(ImageWriter&& other) = default;

  // User-defined destructor
  ~ImageWriter() = default;
};

#ifndef BUILD_FOR_CCODE
class ShadowBuffer {
  void* _orig;
  size_t _size;
  std::shared_ptr<void*> _shadow; //Used for reference counting only
  bool _pagealigned;

public:
  ShadowBuffer() : _orig(0), _size(0), _pagealigned(false) {}
  ShadowBuffer(const ShadowBuffer& other) = delete;
  ShadowBuffer& operator=(const ShadowBuffer& other) = delete;
  ~ShadowBuffer();
  void* GetData(size_t offset = 0) {
    if (_shadow.get() != 0) {
      return (void*)((char*)*_shadow + offset);
    } else {
      return 0;
    }
  }
  void Init(bool pagealigned, size_t size, void* orig = 0, bool writeWatch = false);
  bool Initialized() const {
    return (_shadow.get() != 0);
  }
  void SetOriginalBuffer(void* orig) {
    _orig = orig;
  }
  void Flush(size_t offset, size_t size);
  void UpdateShadow(size_t offset, size_t size);
  void UpdateShadowFromSource(void* ptr, size_t offset, size_t size);
};

std::pair<const void*, size_t> GetSubrangeOverlappingMemoryPages(
    std::pair<const void*, size_t> range, const std::set<const void*>& pages);
std::vector<std::pair<uint64_t, uint64_t>> GetIntervalSetFromMemoryPages(
    std::pair<const void*, size_t> range, const std::set<const void*>& pages);
std::vector<std::pair<const uint8_t*, const uint8_t*>> GetChangedMemorySubranges(
    const void* oldData, const void* newRangeData, uint64_t length, size_t stepSize);
void GetMemoryDiffSubRange(const void* oldData,
                           const void* newRangeData,
                           uint64_t& length,
                           uint64_t& offset);
template <class T>
class CTokensBuffer {
public:
  std::vector<T*> _tokensList;
  ~CTokensBuffer() {
    Clear();
  }
  CTokensBuffer() {}
  CTokensBuffer(const CTokensBuffer& other) = delete;
  CTokensBuffer& operator=(const CTokensBuffer& other) = delete;

  void Exec() {
    for (auto elem : _tokensList) {
      elem->Exec();
    }
  }
  void Clear() {
    for (auto elem : _tokensList) {
      delete elem;
    }
    _tokensList.clear();
  }
  void Add(T* token) {
    _tokensList.push_back(token);
  }
  void AddVector(std::vector<T*>& tokensVector) {
    for (auto elem : tokensVector) {
      _tokensList.push_back(elem);
    }
  }
  void Flush(void (*schedulerFunc)(T*)) {
    for (auto elem : _tokensList) {
      schedulerFunc(elem);
    }
    _tokensList.clear();
  }
};
#endif

class StreamCompressor {
public:
  StreamCompressor() {}
  virtual ~StreamCompressor() {}
  virtual uint64_t Compress(const char* uncompressedData,
                            const uint64_t uncompressedDataSize,
                            std::vector<char>* compressedData) = 0;
  virtual uint64_t Decompress(const std::vector<char>& compressedData,
                              const uint64_t compressedDataSize,
                              const uint64_t expectedUncompressedSize,
                              char* uncompressedData) = 0;
  virtual uint64_t MaxCompressedSize(const uint64_t dataSize) = 0;
};

class LZ4StreamCompressor : public StreamCompressor {
public:
  LZ4StreamCompressor() {}
  virtual uint64_t Compress(const char* uncompressedData,
                            const uint64_t uncompressedDataSize,
                            std::vector<char>* compressedData) override;
  virtual uint64_t Decompress(const std::vector<char>& compressedData,
                              const uint64_t compressedDataSize,
                              const uint64_t expectedUncompressedSize,
                              char* uncompressedData) override;
  virtual uint64_t MaxCompressedSize(const uint64_t dataSize) override;

private:
  LZ4_stream_t ctx;
  std::mutex mutex_;
  const std::map<int, int> perfModes{
      {1, 50}, {2, 35}, {3, 15}, {4, 10}, {5, 6},
      {6, 5},  {7, 4},  {8, 3},  {9, 2},  {10, 1}}; // 1 - fastest, 10 - slowest
};

class ZSTDStreamCompressor : public StreamCompressor {
public:
  ZSTDStreamCompressor();
  ~ZSTDStreamCompressor();
  ZSTDStreamCompressor(const ZSTDStreamCompressor& other) = delete;
  ZSTDStreamCompressor& operator=(const ZSTDStreamCompressor& other) = delete;

  virtual uint64_t Compress(const char* uncompressedData,
                            const uint64_t uncompressedDataSize,
                            std::vector<char>* compressedData) override;
  virtual uint64_t Decompress(const std::vector<char>& compressedData,
                              const uint64_t compressedDataSize,
                              const uint64_t expectedUncompressedSize,
                              char* uncompressedData) override;
  virtual uint64_t MaxCompressedSize(const uint64_t dataSize) override;

private:
  ZSTD_CCtx* ZSTDContext;
  std::mutex mutex_;
  const std::map<int, int> perfModes{
      {1, -7}, {2, -5}, {3, -3}, {4, -1}, {5, 1},
      {6, 3},  {7, 5},  {8, 7},  {9, 9},  {10, 11}}; // 1 - fastest, 10 - slowest
};

#if defined(GITS_PLATFORM_WINDOWS)
std::string GetRenderDocDllPath();
#endif

class SharedLibrary {
  dl::SharedObject handle = nullptr;

public:
  SharedLibrary(const std::string& name) {
    handle = dl::open_library(name.c_str());
    if (handle == nullptr) {
      Log(ERR) << dl::last_error();
    }
  }
  ~SharedLibrary() {
    dl::close_library(handle);
  }
  SharedLibrary(const SharedLibrary&) = delete;
  SharedLibrary& operator=(const SharedLibrary&) = delete;
  SharedLibrary(SharedLibrary&&) = delete;
  SharedLibrary& operator=(SharedLibrary&&) = delete;
  dl::SharedObject getHandle() const {
    return handle;
  }
};

} // namespace gits
