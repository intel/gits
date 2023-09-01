// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

#include <set>
#include <deque>

#ifdef GITS_PLATFORM_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

DISABLE_WARNINGS
#include <boost/thread.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
ENABLE_WARNINGS

#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG 1
#endif
#endif
#include <cassert>

namespace gits {
enum class THashType { MURMUR, XX, INCREMENTAL_NUMBER, CRC32ISH, XXCRC32 };
}

#include "config.h"

namespace gits {

bool SavePng(const std::string& filename,
             size_t width,
             size_t height,
             bool hasAlpha,
             const void* bytes,
             bool flip = true,
             bool sRGB = false,
             bool bgr = false);

void SaveJsonFile(const boost::property_tree::ptree& pt, const boost::filesystem::path& path);

void CheckMinimumAvailableDiskSize();

uint32_t HwCrc32ishHash(const void* data, size_t size, uint32_t hash);
uint64_t ComputeHash(const void* data,
                     size_t size,
                     THashType type,
                     bool hashPartially,
                     uint32_t partialHashCutoff,
                     uint32_t partialHashChunks,
                     uint32_t partialHashRatio);
inline uint64_t ComputeHash(const void* data, size_t size, THashType type) {
  return ComputeHash(data, size, type, false, 0, 0, 0);
}

std::string CommandOutput(const std::string& command, bool isRecorder);

void fast_exit(int);

void CopyDirectoryRecursively(const boost::filesystem::path& from,
                              const boost::filesystem::path& to);

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
  if (static_cast<int64_t>(value) >= LONG_MIN && static_cast<int64_t>(value) <= LONG_MAX) {
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
  ProducerConsumer(const ProducerConsumer&) = delete;
  ProducerConsumer& operator=(const ProducerConsumer&) = delete;
  ProducerConsumer(ProducerConsumer&&) = delete;
  ProducerConsumer& operator=(ProducerConsumer&&) = delete;
  // Blocks until product is available. Returns false when
  // producer is exhausted - no product is generated in such
  // event.
  bool consume(Product& product) {
    boost::unique_lock<boost::mutex> lock(mutex_);

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
    // anyways. Only notify him if queue is resonably empty.
    if (cost_capacity_ > 0) {
      lock.unlock();
      cond_.notify_one();
    }

    return true;
  }

  // If queue accepted the product, returns true.
  bool produce(Product& product) {
    boost::unique_lock<boost::mutex> lock(mutex_);

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
    boost::unique_lock<boost::mutex> lock(mutex_);
    exhausted_ = true;

    lock.unlock();
    cond_.notify_one();
  }

private:
  boost::mutex mutex_;
  boost::condition_variable cond_;
  std::deque<Product> products_;
  bool exhausted_;
  int cost_capacity_;
};

template <class WorkUnit>
class TaskFunction {
public:
  typedef ProducerConsumer<WorkUnit> Queue;

  template <class T>
  TaskFunction(Queue& q, T func) : function_(func), queue_(q) {}
  void operator()() {
    function_(queue_);
  }

private:
  std::function<void(Queue&)> function_;
  Queue& queue_;
  void operator=(const TaskFunction&);
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
  void start(T func) {
    assert(thread_.size() == 0);
    thread_.create_thread(TaskFunction<WorkUnit>(queue_, func));
  }
  bool running() const {
    return thread_.size() != 0;
  }
  void finish() {
    queue_.break_pipe();
    if (thread_.size() != 0) {
      thread_.join_all();
    }
  }

  ProducerConsumer<WorkUnit>& queue() {
    return queue_;
  }
  ~Task() {
    finish();
  }

private:
  boost::thread_group thread_;
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

private:
  void operator=(ImageWriter&);
};

#ifndef BUILD_FOR_CCODE
class ShadowBuffer {
  void* _orig;
  size_t _size;
  std::shared_ptr<void*> _shadow; //Used for reference counting only
  bool _pagealigned;

public:
  ShadowBuffer() : _orig(0), _size(0), _pagealigned(false) {}
  ~ShadowBuffer();
  void* GetData(size_t offset = 0) {
    if (_shadow.get() != 0) {
      return (void*)((char*)*_shadow + offset);
    } else {
      return 0;
    }
  }
  void Init(bool pagealigned, size_t size, void* orig = 0);
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

template <class KEY,
          class VAL,
          class CMP = std::less<KEY>,
          class ALLOC = std::allocator<std::pair<const KEY, VAL>>>
class concurrentMap {
  std::map<KEY, VAL, CMP, ALLOC> _map;
  mutable boost::shared_mutex _mutex;

public:
  concurrentMap() = default;
  concurrentMap(const concurrentMap&) = delete;
  concurrentMap& operator=(const concurrentMap&) = delete;
  concurrentMap(concurrentMap&&) = delete;
  concurrentMap& operator=(concurrentMap&&) = delete;
  void insert(const std::pair<KEY, VAL> val) {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    _map.insert(val);
  }
  void set(KEY key, VAL val) {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    _map[key] = val;
  }
  size_t erase(const KEY& key) {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    return _map.erase(key);
  }
  void clear() {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    _map.clear();
  }
  std::map<KEY, VAL, CMP, ALLOC> copy() {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    return _map;
  }
  const VAL& at(const KEY& key) const {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    return _map.at(key);
  }
  bool has(const KEY& key) const {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    return _map.find(key) != _map.end();
  }
  bool size() const {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    return _map.size();
  }
};

template <class KEY, class CMP = std::less<KEY>, class ALLOC = std::allocator<KEY>>
class concurrentSet {
  std::set<KEY, CMP, ALLOC> _set;
  mutable boost::shared_mutex _mutex;

public:
  concurrentSet() = default;
  concurrentSet(const concurrentSet&) = delete;
  concurrentSet& operator=(const concurrentSet&) = delete;
  concurrentSet(concurrentSet&&) = delete;
  concurrentSet& operator=(concurrentSet&&) = delete;
  void insert(const KEY key) {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    _set.insert(key);
  }
  size_t erase(const KEY& key) {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    return _set.erase(key);
  }
  void clear() {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    _set.clear();
  }
  std::set<KEY, CMP, ALLOC> copy() {
    boost::unique_lock<boost::shared_mutex> lock(_mutex);
    return _set;
  }
  bool has(const KEY& key) const {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    return _set.find(key) != _set.end();
  }
  bool size() const {
    boost::shared_lock<boost::shared_mutex> lock(_mutex);
    return _set.size();
  }
};

#if defined(GITS_PLATFORM_WINDOWS)
std::string GetRenderDocDllPath();
#endif

} // namespace gits
