// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helper.h"
#include "gitsApi.h"
#include "platform.h"
#include "log.h"
#include "config.h"
#include "gits.h"

#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if defined GITS_API_OCL
#include "openclDrivers.h"
#endif
#if defined WITH_LEVELZERO and defined GITS_API_L0
#include "l0Drivers.h"
#endif

#include <cstdio>
#include <iomanip>
#include <png.h>
#include <cstdint>
#include <cstring>

#define GET_RESOURCE_DATA(fileName, fileOffset, size)                                              \
  const char* fileData;                                                                            \
  fileData = LoadResourcesImmediately::GetInstance().GetData(fileName, fileOffset, (unsigned)size);

#define RELEASE_RESOURCE_DATA                                                                      \
  if (!LoadResourcesImmediately::GetInstance().initialized)                                        \
    FreeMem(fileData);

BinaryResource::BinaryResource(size_t size, size_t offset, const char* filename) {
  if (LoadResourcesImmediately::GetInstance().initialized) {
    _ptr =
        LoadResourcesImmediately::GetInstance().GetData(filename, (unsigned)offset, (unsigned)size);
  } else {
    _ptr = LoadFileDense(filename, (unsigned)offset, (unsigned)size);
  }
}

BinaryResource::~BinaryResource() {
  try {
    if (!LoadResourcesImmediately::GetInstance().initialized) {
      FreeMem((const char*)_ptr);
    }
  } catch (...) {
    topmost_exception_handler("BinaryResource::~BinaryResource");
  }
}

LoadResourcesImmediately::LoadResourcesImmediately() : initialized(false) {
  fileMap["gitsTextures.dat"] = NULL;
  fileMap["gitsBuffers.dat"] = NULL;
  fileMap["gitsDataIndex.dat"] = NULL;
  fileMap["gitsClientSizes.dat"] = NULL;
}

LoadResourcesImmediately::~LoadResourcesImmediately() {
  ReleaseResources();
}

char* LoadResourcesImmediately::LoadFile(const std::string& filePath) {
  const std::uintmax_t size = std::filesystem::file_size(filePath);
  return LoadFileDense(filePath, 0, size);
}

void LoadResourcesImmediately::ReleaseResources() {
  ResourcesMap::iterator it;
  for (it = fileMap.begin(); it != fileMap.end(); ++it) {
    if (it->second != NULL) {
      FreeMem(it->second);
    }
  }
}

void LoadResourcesImmediately::LoadResources() {
  ResourcesMap::iterator it;

  initialized = true;
  for (it = fileMap.begin(); it != fileMap.end(); ++it) {
    it->second = LoadFile(it->first);
    if (it->second == NULL) {
      ReleaseResources();
      initialized = false;
      break;
    }
  }
}

char* LoadResourcesImmediately::GetData(const std::string& fileName,
                                        unsigned offset,
                                        unsigned size) {
  char* fileData = NULL;

  if (LoadResourcesImmediately::GetInstance().initialized) {
    fileData = fileMap[fileName] + offset;
  } else {
    fileData = LoadFileDense(fileName, offset, (unsigned)size);
  }

  if (fileData == NULL) {
    exit(EXIT_FAILURE);
  }

  return fileData;
}

LoadResourcesImmediately& LoadResourcesImmediately::GetInstance() {
  static LoadResourcesImmediately instance;
  return instance;
}

void SetPointer(PointerDescr& pointer, char* dataArray, int dataArraySize, int stride) {
  FreeMem(pointer.pointer);
  FreeMem(pointer.copy);

  pointer.pointer = dataArray;
  pointer.size = dataArraySize;
  pointer.copy = CopyArray(pointer.pointer, pointer.size);
  pointer.stride = stride;
}

struct ResourceHandle {
  uint64_t offset;
  uint32_t file_id;
  uint32_t size;
};

void LoadPointer(PointerDescr& pointer, int identifier, int stride) {
  //Check if pointer loaded
  if (identifier == pointer.identifier) {
    return;
  }

  static std::map<uint32_t, uint64_t> ids_map;
  static std::map<uint64_t, ResourceHandle> res_map;

  if (ids_map.empty()) {
    // Maps block identifier to its hash.
    ids_map = read_map<std::map<uint32_t, uint64_t>>("gitsClientSizes.dat");
  }
  if (res_map.empty()) {
    res_map = read_map<std::map<uint64_t, ResourceHandle>>("gitsDataIndex.dat");
  }

  uint64_t hash = ids_map[identifier];
  ResourceHandle handle = res_map[hash];

  // Read index / get
  pointer.copy = 0;
  FreeMem(pointer.pointer);
  pointer.pointer = LoadFileDense("gitsBuffers.dat", (unsigned int)handle.offset, handle.size);
  pointer.stride = stride;
  pointer.size = handle.size;
  pointer.identifier = identifier;
}

void UpdatePointer(PointerDescr& pointer,
                   const int* indexArray,
                   int indicesCount,
                   void* updateData,
                   int elemSize) {
  char* ptrDat = pointer.pointer;
  char* updDat = static_cast<char*>(updateData);
  int stride = pointer.stride;

  if (stride == 0) {
    stride = elemSize;
  }

  for (int i = 0; i < indicesCount; ++i) {
    int currIdx = indexArray[i];
    std::copy(updDat + i * elemSize, updDat + (i + 1) * elemSize, ptrDat + currIdx * stride);
  }
}

uint64_t FileSize(const std::string& fileName) {
  std::filesystem::path filePath = Config::Get().common.streamDir / fileName;
  if (!std::filesystem::exists(filePath)) {
    std::cerr << "File: " << filePath << " does not exist!";
  }
  uint64_t fileSize = std::filesystem::file_size(filePath);
  return fileSize;
}

char* LoadFile(const std::string& fileName) {
  uint64_t fileSize = FileSize(fileName);
  char* retval = NULL;
  try {
    if (fileSize > 0) {
      retval = new char[(unsigned)fileSize + 1];
    } else {
      return NULL;
    }
  } catch (std::bad_alloc) {
    delete[] retval;
    std::cerr << "Failed to allocate memory" << std::endl;
    return NULL;
  }

  retval[fileSize] = 0; //null terminate to simplify glShaderSource handling
  std::string filePath = Config::Get().common.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    delete[] retval;
    std::cerr << "Failed to open file: " << filePath << "\n";
    return NULL;
  }

  file.read(retval, fileSize);
  file.close();
  return retval;
}

char* LoadFileDense(const std::string& fileName, unsigned offset, unsigned size) {
  std::string filePath = Config::Get().common.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << "\n";
    return NULL;
  }

  file.seekg(offset, std::ios::beg);
  char* retval = NULL;
  try {
    retval = new char[size];
  } catch (std::bad_alloc) {
    delete[] retval;
    std::cerr << "Failed to allocate memory" << std::endl;
    return NULL;
  }

  file.read(retval, size);

  if (!file) {
    delete[] retval;
    std::cerr << "Failed reading: " << filePath << "\n";
    return NULL;
  }

  file.close();

  return retval;
}

char* AllocMem(int size) {
  return new char[size];
}

void FreeMem(const char* memBlock) {
  delete[] (char*)memBlock;
}

char* CopyArray(const char* src, int size) {
  if (size == 0 || src == 0) {
    return 0;
  }
  char* ptr = AllocMem(size);
  memcpy(ptr, src, size);
  return ptr;
}

void ProcessMessages() {
#ifdef GITS_PLATFORM_WINDOWS
  MSG msg;
  //getting all messages from queue
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
#endif
}

void OnFrameEnd() {
  CGits::Instance().FrameCountUp();
  ProcessMessages();
  uint32_t currentFrame = CGits::Instance().CurrentFrame();
  unsigned benchmarkStartFrame = Config::Get().ccode.benchmarkStartFrame;
  if (currentFrame > benchmarkStartFrame) {
    CGits::Instance().SetLastFrameTime();
  } else if (currentFrame == benchmarkStartFrame) {
    CGits::Instance().StartPlaybackTimer();
  }
}

#if defined GITS_API_OCL && !defined CCODE_FOR_EGL

namespace cl {

void CL_CALLBACK CallbackContext(const char*, const void*, size_t, void*) {}
void CL_CALLBACK CallbackProgram(cl_program, void*) {}
void CL_CALLBACK CallbackEvent(cl_event, cl_int, void*) {}
void CL_CALLBACK CallbackMem(cl_mem, void*) {}
cl::TUserData userData;
} // namespace cl

void Load(const std::string& fileName, size_t bufferSize, char* buffer) {
  int fileSize = (int)FileSize(fileName);
  if (fileSize != bufferSize) {
    std::cerr << "Buffer size (" << bufferSize << ") do not match file size (" << fileSize
              << ") of: " << fileName << "\n";
    exit(EXIT_FAILURE);
  }

  std::string filePath = Config::Get().common.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }

  file.read(buffer, bufferSize);
  if (!file) {
    std::cerr << "Failed reading: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }
}

void Load(const std::string& fileName, unsigned offset, size_t bufferSize, char* buffer) {
  uint64_t fileSize = FileSize(fileName);
  if (fileSize < bufferSize) {
    std::cerr << "Buffer size is bigger than the file size of: " << fileName << "\n";
    exit(EXIT_FAILURE);
  }
  std::string filePath = Config::Get().common.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }

  file.seekg(offset, std::ios::beg);
  file.read(buffer, bufferSize);
  if (!file) {
    std::cerr << "Failed reading: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }
}

void SleepIf(bool exprResult, float miliseconds) {
  if (exprResult)
#ifdef GITS_PLATFORM_WINDOWS
    Sleep((DWORD)miliseconds);
#else
    sleep(miliseconds);
#endif
}

void CLInit() {
  gits::OpenCL::drvOcl.Initialize();
}

#endif /* GITS_API_OCL */

#ifdef WITH_LEVELZERO
void InitL0() {
  gits::l0::drv.Initialize();
}
#endif

const std::string getDumpFrameFileName(int frameNumber) {
  std::stringstream fileName;
  fileName << "frame" << std::setw(8) << std::setfill('0') << frameNumber;
  return fileName.str();
}

MemTracker::TMemoryAreas& MemTracker::Instance() {
  static TMemoryAreas memArea;
  return memArea;
}

uint64_t GetCArraySizeFromId(uint64_t id) {
  typedef std::map<uint64_t, uint64_t> map_t;
  INIT_NEW_STATIC_OBJ(idsMap, map_t);
  CALL_ONCE[&] {
    idsMap = read_map<std::map<uint64_t, uint64_t>>("gitsClientSizes.dat");
  };
  if (idsMap.find(id) == idsMap.end()) {
    return 0;
  }
  return idsMap[id];
}

ResourceHandle GetResourceHandle(uint64_t hash) {
  typedef std::map<uint64_t, ResourceHandle> res_map_t;
  INIT_NEW_STATIC_OBJ(res_map, res_map_t);
  CALL_ONCE[&] {
    res_map = read_map<std::map<uint64_t, ResourceHandle>>("gitsDataIndex.dat");
  };
  return res_map[hash];
}

void DataUpdate(uint64_t recarea, uint64_t offset, uint64_t hash) {
  uint64_t areaPtrRec = recarea;
  uint64_t updateOffset = offset;
  auto& memTracker = MemTracker::Instance();

  //Allocate memory if needed
  if (memTracker.find(areaPtrRec) == memTracker.end()) {
    memTracker[areaPtrRec].resize((size_t)GetCArraySizeFromId(areaPtrRec), 0);
  }

  //Apply diff
  char* dataPtr = &memTracker[areaPtrRec][0] + updateOffset;
  mapped_file storedDataPtr;
  storedDataPtr = GetResourceManager().get(hash);

  memcpy(dataPtr, storedDataPtr.address(), storedDataPtr.size());
}

void* GetDataPtr(uint64_t recptr) {
  uint64_t areaOffset = GetAreaOffset(recptr);
  uint64_t areaRecPtr = GetAreaPtr(recptr);
  auto& memTracker = MemTracker::Instance();

  // Alloc client data if needed.
  if (memTracker.find(areaRecPtr) == memTracker.end()) {
    memTracker[areaRecPtr].resize((int)GetCArraySizeFromId(areaRecPtr), 0);
  }

  // Return client data ptr.
  if (memTracker[areaRecPtr].size() <= areaOffset) {
    Log(WARN) << "Pointer " << recptr
              << " passed to GL API doesn't seem to be used, it's likely invalid.";
    return (char*)recptr;
  } else {
    char* ptr = &memTracker.at(areaRecPtr)[0] + areaOffset;
    return ptr;
  }
}

Resource::Resource(uint64_t hash) {
  _data = GetResourceManager().get(hash);
}
