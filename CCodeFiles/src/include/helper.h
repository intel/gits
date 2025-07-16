// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// When defined, _PERF_MODE_ assumes the existence of file gitsData.raw and preloads it into memory before use.
//#define _PERF_MODE_

// When defined, _TIMING_MASK_AND_FRAME_LOOP_ allows passing parameters to time GLblocks and loop the rendered image.
//#define _TIMING_MASK_AND_FRAME_LOOP_

#pragma once
#include "gitsApi.h"
#include "config.h"
#include "resource_manager.h"

#include <iostream>
#include <fstream>

namespace gits {}
using namespace gits;

class BinaryResource : gits::noncopyable {
public:
  BinaryResource(size_t size, size_t offset, const char* filename);
  ~BinaryResource();

  template <class T>
  operator const T*() {
    return _ptr;
  }

private:
  const void* _ptr;
};

inline CResourceManager2& GetResourceManager() {
  static CResourceManager2 resourceManager(
      resource_filenames(Configurator::Get().common.player.streamDir));
  return resourceManager;
}

class LoadResourcesImmediately {
private:
  typedef std::map<std::string, char*> ResourcesMap;
  ResourcesMap fileMap;

  char* LoadFile(const std::string& fileName);
  LoadResourcesImmediately();
  LoadResourcesImmediately(const LoadResourcesImmediately&);
  LoadResourcesImmediately& operator=(const LoadResourcesImmediately&);
  void ReleaseResources(void);

public:
  ~LoadResourcesImmediately();
  void LoadResources();

  static LoadResourcesImmediately& GetInstance();
  char* GetData(const std::string& fileName, unsigned offset, unsigned size);

  bool initialized;
};

template <typename T>
void read_from_stream2(std::istream& i, T& value) {
  i.read(reinterpret_cast<char*>(&value), sizeof(value));
}

class StreamType {
public:
  static bool IsStreamNative() {
    return getType();
  }
  static void SetStreamNative() {
    getType() = true;
  }
  static void SetStreamStandard() {
    getType() = false;
  }

private:
  static bool& getType() {
    static bool nativeStream = false;
    return nativeStream;
  }
};

struct PointerDescr {
  PointerDescr() : pointer(0), size(0), stride(0), copy(0) {}
  PointerDescr(char* p, int s, int t, char* c)
      : pointer(p), size(s), stride(t), copy(c), identifier(-1) {}

  char* pointer;
  int size;
  int stride;
  char* copy;
  int identifier;
};

extern PointerDescr vertexPointer;
extern PointerDescr colorPointer;
extern PointerDescr secondaryColorPointer;
extern PointerDescr normalPointer;
extern PointerDescr texcoordPointer[32];
extern PointerDescr vertexAttribPointer[32];
extern PointerDescr interleavedArray;
extern unsigned clientActiveTexture;
extern unsigned currentProgram;

void UpdatePointer(
    PointerDescr& pointer, const int* indexArray, int indicesCount, void* updateData, int elemSize);
template <size_t S>
void UpdatePointer(PointerDescr& pointer,
                   const int (&indexArray)[S],
                   void* updateData,
                   int elemSize) {
  UpdatePointer(pointer, indexArray, S, updateData, elemSize);
}

void SetPointer(PointerDescr& pointer, char* dataArray, int dataArraySize, int stride);
void LoadPointer(PointerDescr& pointer, int identifier, int stride);

uint64_t FileSize(const std::string& fileName);

char* LoadFile(const std::string& fileName);

class TextFile {
  std::string _filename;
  std::vector<char> _source;
  const char* _sourcePtr;
  const char** _sourcePtrPtr;

public:
  TextFile(const char* filename) : _filename(filename) {
    std::filesystem::path filePath = Configurator::Get().common.player.streamDir / _filename;
    uint64_t fileSize = FileSize(_filename);

    if (fileSize > 0) {
      _source.resize((unsigned)fileSize + 1, 0);
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filePath << "\n";
    }
    _sourcePtr = &_source[0];
    _sourcePtrPtr = &_sourcePtr;

    file.read(&_source[0], fileSize);
  }

  operator const char*() const {
    return _sourcePtr;
  }
  operator const char**() const {
    return _sourcePtrPtr;
  }
  operator const unsigned char*() const {
    return reinterpret_cast<const unsigned char*>(_sourcePtr);
  }
  operator const void*() const {
    return _sourcePtr;
  }
};

const std::string getDumpFrameFileName(int frameNumber);

#ifdef _PERF_MODE_
inline char* LoadFileDense(const std::string& fileName, unsigned offset, unsigned size);
#else
char* LoadFileDense(const std::string& fileName, unsigned offset, unsigned size);
#endif
char* AllocMem(int size);
void FreeMem(const char* memBlock);
char* CopyArray(const char* src, int size);
void ProcessMessages();
void OnFrameEnd();

// Read an std::map (or compatible) from file.
template <typename T>
T read_map(const std::string& filename) {
  T retval;
  std::ifstream file(filename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("corrupted key-val store (couldn't open file): " + filename);
  }

  file.putback(static_cast<char>(file.get()));
  while (!file.fail()) {
    typename T::key_type key;
    typename T::mapped_type value;

    read_from_stream2(file, key);
    if (!file) {
      throw std::runtime_error("corrupted key-val store (bad key): " + filename);
    }

    read_from_stream2(file, value);
    if (!file) {
      throw std::runtime_error("corrupted key-val store (bad value): " + filename);
    }

    retval[key] = value;
    //lookahead for eof
    file.putback(static_cast<char>(file.get()));
  }
  return retval;
}

#define GITS_MEM_AREA_SIZE 4096;
template <class T>
uint64_t GetAreaPtr(T ptr) {
  return (uint64_t)ptr - (uint64_t)ptr % GITS_MEM_AREA_SIZE;
}
template <class T>
uint64_t GetAreaOffset(T ptr) {
  return (uint64_t)ptr % GITS_MEM_AREA_SIZE;
}

struct MemTracker {
  typedef std::vector<char> TMemory;
  typedef std::map<uint64_t, TMemory> TMemoryAreas;
  static TMemoryAreas& Instance();
};
void* GetDataPtr(uint64_t recptr);
void DataUpdate(uint64_t recarea, uint64_t offset, uint64_t hash);

class Resource {
  std::vector<char> _data;

public:
  explicit Resource(uint64_t hash);
  size_t Size() const {
    return _data.size();
  }
  operator const void*() const {
    return _data.data();
  }
  operator const uint32_t*() const {
    return (const uint32_t*)_data.data();
  }
  // For compatibility with GLubyte without knowing it exists.
  operator const unsigned char*() const {
    return (const unsigned char*)_data.data();
  }
  operator const char*() const {
    return _data.data();
  }
};

class ToAnyPtr {
  void* ptr_;

public:
  explicit ToAnyPtr(void* ptr) : ptr_(ptr) {}
  template <class T>
  operator T*() {
    return (T*)ptr_;
  }
};

inline ToAnyPtr outArg() {
  static std::vector<char> space(64 * 1024);
  return ToAnyPtr(&space[0]);
}
