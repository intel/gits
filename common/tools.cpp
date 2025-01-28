// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools.cpp
 *
 * @brief Common tools for GITS project.
 *
 */

#include "tools.h"
#include "MurmurHash3.h"
#include "xxhash.h"

#include <cstdint>
#include <regex>
#include <fstream>

#ifndef BUILD_FOR_CCODE
#include "MemorySniffer.h"
#include "token.h"
#endif
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include <process.h>
#else
#include <sys/mman.h>
#endif

#include <png.h>
#include <pngpriv.h>
#include <pnginfo.h>

#if !defined GITS_ARCH_ARM && !defined GITS_ARCH_A64 && !defined BUILD_FOR_CCODE

#include <smmintrin.h>

uint32_t rotate_left(uint32_t value, uint32_t shift) {
  return (value << shift) | (value >> (32 - shift));
}

uint32_t gits::HwCrc32ishHash(const void* data, size_t size, uint32_t hash) {
  // we operate on uint32_t's
  uint32_t num_32b_words = (uint32_t)size / sizeof(uint32_t);
  // we unroll 4 loops
  num_32b_words = num_32b_words / 4 * 4;

  uint32_t accum[4] = {0, 0, 0, 0};
  auto elems32 = static_cast<const uint32_t*>(data);
  for (uint32_t i = 0; i < num_32b_words; i += 4) {
    accum[0] = _mm_crc32_u32(accum[0], elems32[i + 0]);
    accum[1] = _mm_crc32_u32(accum[1], elems32[i + 1]);
    accum[2] = _mm_crc32_u32(accum[2], elems32[i + 2]);
    accum[3] = _mm_crc32_u32(accum[3], elems32[i + 3]);
  }

  auto elems8 = static_cast<const unsigned char*>(data);
  uint32_t accum_rest = hash;
  for (uint32_t i = num_32b_words * sizeof(uint32_t); i < size; ++i) {
    accum_rest = _mm_crc32_u8(accum_rest, elems8[i]);
  }

  return rotate_left(accum[0], 3) ^ rotate_left(accum[1], 5) ^ rotate_left(accum[2], 7) ^
         rotate_left(accum[3], 11) ^ rotate_left(accum_rest, 13);
}

#else

uint32_t gits::HwCrc32ishHash(const void* data, size_t size, uint32_t hash) {
  return XXH32(data, size, hash);
}

#endif

namespace gits {
void gits_assert(bool condition,
                 const char* condition_string,
                 const char* message,
                 const char* function,
                 const char* file,
                 uint32_t line) {
  if (!condition) {
    Log(ERR) << "Assertion failed: " << condition_string << " " << message
             << "\n  Function: " << function << "\n  File: " << file << "\n  Line: " << line;
    fast_exit(EXIT_FAILURE);
  }
}

uint64_t ComputeHash(const void* data,
                     size_t size,
                     THashType type,
                     bool hashPartially,
                     uint32_t partialHashCutoff,
                     uint32_t partialHashChunks,
                     uint32_t partialHashRatio) {
  union u32u64 {
    uint64_t u64;
    uint32_t u32[2];
  };

  if (type == THashType::INCREMENTAL_NUMBER) {
    // Use static int as hasing value.
    static uint64_t hash_val = 0;
    return ++hash_val;
  } else if (hashPartially && size > partialHashCutoff) {
    // Derive hash from only part of data. We only hope that this
    // doesn't generate collisions. Should be used only when verified
    // that it produces correct results and is actually needed.
    uint32_t hash = 0;
    uint32_t hash2 = static_cast<uint32_t>(size);
    const uint32_t chunk_num = partialHashChunks;
    const uint32_t chunk_size = static_cast<uint32_t>(size) / (partialHashRatio * chunk_num);
    const uint32_t chunk_stride = static_cast<uint32_t>(size) / chunk_num;
    const char* cdata = (const char*)data;

    for (uint32_t i = 0; i < chunk_num; ++i) {
      if (type == THashType::XX) {
        hash = XXH32(cdata + i * chunk_stride, chunk_size, hash);
      } else if (type == THashType::XXCRC32) {
        hash = XXH32(cdata + i * chunk_stride, chunk_size, hash);
        hash2 = HwCrc32ishHash(cdata + i * chunk_stride, chunk_size, hash);
      } else if (type == THashType::MURMUR) {
        MurmurHash3_x86_32(cdata + i * chunk_stride, chunk_size, hash, &hash);
      } else {
        hash = HwCrc32ishHash(cdata + i * chunk_stride, chunk_size, hash);
      }
    }

    u32u64 val;
    val.u32[0] = hash;
    val.u32[1] = hash2;
    return val.u64;
  } else {
    u32u64 val;
    if (type == THashType::XX || type == THashType::XXCRC32) {
      val.u32[0] = XXH32(data, size, 0);
    } else if (type == THashType::MURMUR) {
      MurmurHash3_x86_32(data, static_cast<int>(size), 0, &val.u32[0]);
    } else {
      val.u32[0] = HwCrc32ishHash(data, size, 0);
    }

    if (type == THashType::XXCRC32) {
      // XxCrc32 combines XXHash with Crc32Ish (first 32 bit - Xxhash, second - Crc32Ish)
      val.u32[1] = HwCrc32ishHash(data, size, 0);
    } else {
      val.u32[1] = static_cast<uint32_t>(size);
    }
    return val.u64;
  }
}

uint64_t ComputeHash(const void* data, size_t size, THashType type) {
  return ComputeHash(data, size, type, false, 0, 0, 0);
}

std::string CommandOutput(const std::string& command, bool isRecorder) {
#ifdef GITS_PLATFORM_WINDOWS
  // Windows can't handle popen correctly in non-console applications.
  if (isRecorder) {
    return "<couldn't exec: " + command + ">";
  }
#define popen  _popen
#define pclose _pclose
#endif
  std::string content;
  char buffer[256];

  errno = 0;
  FILE* out = popen(command.c_str(), "r");
  if (out == nullptr) {
    const auto msg = "Couldn't open process, errno: " + std::to_string(errno);
    throw std::runtime_error(std::string(EXCEPTION_MESSAGE) + msg);
  }
  while (!feof(out)) {
    // Linux generates warning for not using the return value of fgets function.
    if (fgets(buffer, sizeof(buffer), out) != nullptr) {
      content += buffer;
    }
  }
  pclose(out);

  return content;
}

// Png writing code taken from Corona Library (with modifications).
namespace {
void PNG_write(png_structp png_ptr, png_bytep data, png_size_t length) {
  std::ofstream* file = (std::ofstream*)png_get_io_ptr(png_ptr);
  if (file == nullptr) {
    png_error(png_ptr, "png_get_io_ptr failed initialization");
    return;
  }
  if (!file->write((const char*)data, length)) {
    png_error(png_ptr, "Write error");
  }
}
void PNG_flush(png_structp png_ptr) {}
} // namespace
// Expects tightly packed RGB8 or RGBA8 data.
bool SavePng(const std::string& filename,
             size_t width,
             size_t height,
             bool hasAlpha,
             const void* bytes,
             bool flip,
             bool bgr,
             bool sRGB) {
  char* pixels = (char*)bytes;
  if (pixels == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out);
  // create write struct
  png_const_charp userPngVer = PNG_LIBPNG_VER_STRING;
  png_structp png_ptr = png_create_write_struct(userPngVer, nullptr, nullptr, nullptr);
  if (!png_ptr) {
    return false;
  }

  // error handling!
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, nullptr);
    return false;
  }

  // create info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, nullptr);
    return false;
  }

  // set image characteristics
  png_set_write_fn(png_ptr, &file, PNG_write, PNG_flush);

  int color_format = PNG_COLOR_TYPE_RGB;
  int color_format_bpp = 3;

  png_set_IHDR(png_ptr, info_ptr, ensure_unsigned32bit_representible<size_t>(width),
               ensure_unsigned32bit_representible<size_t>(height), 8, color_format,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  if (sRGB) {
    png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_SATURATION);
  }

  // build rows
  void** rows = (void**)png_malloc(
      png_ptr, ensure_unsigned32bit_representible<size_t>(sizeof(void*) * height));

  for (unsigned i = 0; i < height; ++i) {
    size_t elem;
    if (flip) {
      elem = height - i - 1;
    } else {
      elem = i;
    }
    rows[elem] =
        png_malloc(png_ptr, ensure_unsigned32bit_representible<size_t>(color_format_bpp * width));
    if (hasAlpha) {
      char* row = (char*)rows[elem];
      for (unsigned j = 0; j < width * color_format_bpp; j += 3, pixels += 4) {
        row[j + 0] = pixels[0];
        row[j + 1] = pixels[1];
        row[j + 2] = pixels[2];
      }
    } else {
      memcpy(rows[elem], pixels, color_format_bpp * width);
      pixels += width * color_format_bpp;
    }
  }

  png_set_rows(png_ptr, info_ptr, (png_bytepp)rows);
  info_ptr->valid |= PNG_INFO_IDAT;

  // actually write the image
  int pngTransforms;
  if (bgr) {
    pngTransforms = PNG_TRANSFORM_BGR;
  } else {
    pngTransforms = PNG_TRANSFORM_IDENTITY;
  }
  png_write_png(png_ptr, info_ptr, pngTransforms, nullptr);

  // clean up memory
  for (unsigned i = 0; i < height; ++i) {
    png_free(png_ptr, rows[i]);
  }
  png_free(png_ptr, rows);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  return true;
}

#ifndef BUILD_FOR_CCODE
void SaveJsonFile(const nlohmann::ordered_json& json, const std::filesystem::path& path) {
  try {
    std::filesystem::create_directory(path.parent_path());
    std::ofstream file(path);
    constexpr auto indent = 4U;
    file << json.dump(indent);
    file.close();
  } catch (std::filesystem::filesystem_error& fe) {
    Log(ERR) << "Exception during creating directory. System error code: " << fe.code();
  }
}
#endif

void CheckMinimumAvailableDiskSize() {
  auto config = Config::Get();
  auto& path = config.common.recorder.dumpPath;
  auto diskSpaceInfo = std::filesystem::space(path);
  uintmax_t minDiskSize = 104857600;
  if (diskSpaceInfo.available <= minDiskSize) {
    auto mebiByteSize = diskSpaceInfo.available >> 20;
    Log(ERR) << "Disk might run out of space, available (MebiBytes): " << mebiByteSize;
  }
}

void fast_exit(int code) {
#if defined GITS_PLATFORM_WINDOWS
  _exit(code);
#else
  _Exit(code);
#endif
}

#ifdef GITS_PLATFORM_X11
pid_t GetPIDFromWindow(Display* display, Window w) {
  Atom atomPID, type;
  int format;
  unsigned long nItems, bytesAfter;
  unsigned char* propPID = nullptr;

  atomPID = XInternAtom(display, "_NET_WM_PID", 1);
  if (Success == XGetWindowProperty(display, w, atomPID, 0, 1, False, XA_CARDINAL, &type, &format,
                                    &nItems, &bytesAfter, &propPID)) {
    if (propPID != nullptr) {
      pid_t pid = *((pid_t*)propPID);
      XFree(propPID);
      return pid;
    }
  }
  return 0;
}

bool SearchForWindow(Display* display, pid_t pid, Window w, win_ptr_t& returnWindow) {
  bool found = false;
  pid_t propPID = GetPIDFromWindow(display, w);
  if (pid == propPID && returnWindow == 0) {
    returnWindow = w;
    return true;
  }

  Window wRoot, wParent, *wChild;
  unsigned int nChildren;
  if (XQueryTree(display, w, &wRoot, &wParent, &wChild, &nChildren) && returnWindow == 0) {
    for (unsigned int i = 0; i < nChildren; i++) {
      found = SearchForWindow(display, pid, wChild[i], returnWindow);
      if (found) {
        break;
      }
    }
  }
  return found;
}
#endif

#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
win_ptr_t GetWindowHandle() {
  win_ptr_t window = NULL;
#ifdef GITS_PLATFORM_WINDOWS
  window = FindWindow((LPCSTR) "my_window_class_nameX", NULL);
#elif defined GITS_PLATFORM_X11
  pid_t pid = getpid();
  Display* display = XOpenDisplay(nullptr);
  Window root = XDefaultRootWindow(display);
  SearchForWindow(display, pid, root, window);
  XCloseDisplay(display);
#endif
  return window;
}
#endif

#ifdef GITS_PLATFORM_WINDOWS
std::string GetWindowsProcessName(int processID) {
  std::string processName;
  HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
  if (handle) {
    DWORD buffSize = 1024;
    CHAR buffer[1024];
    BOOL retVal = QueryFullProcessImageNameA(handle, 0, buffer, &buffSize);
    CloseHandle(handle);
    if (retVal == false) {
      Log(ERR) << "Can't get name of the process.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    processName = buffer;
  } else {
    Log(ERR) << "Can't open handle to the process.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  std::string::size_type startNamePos = processName.find_last_of("/\\");
  processName = processName.substr(startNamePos + 1);
  std::string::size_type endNamePos = processName.find_last_of('.');
  processName = processName.substr(0, endNamePos);
  return processName;
}
#endif

#ifdef GITS_PLATFORM_LINUX
std::string GetLinuxProcessName(pid_t processID) {
  std::string processName;
  std::ifstream file("/proc/" + std::to_string(processID) + "/comm");
  file >> processName;
  return processName;
}
#endif

std::string GetLinuxProcessNamePath() {
#ifdef GITS_PLATFORM_LINUX
  pid_t processId = getpid();
  std::string processName;
  std::ifstream file("/proc/" + std::to_string(processId) + "/cmdline");
  file >> processName;
  processName.erase(processName.end() - 1);
  return processName;
#else
  return "";
#endif
}

unsigned int stoui(const std::string& str) {
  const unsigned long as_ulong = std::stoul(str, nullptr, 10);
  const unsigned int as_uint = as_ulong;
  if (as_uint != as_ulong) {
    throw std::out_of_range("The number " + str + " does not fit in an unsigned int.");
  }

  return as_uint;
}

std::vector<std::string> GetStringsWithRegex(std::string src,
                                             const char* regex,
                                             const char* rmRegex) {
  std::vector<std::string> foundStrings;
  std::regex expr(regex);
  std::smatch what;
  while (std::regex_search(src, what, expr)) {
    foundStrings.push_back(std::regex_replace(what.str(0), std::regex(rmRegex), ""));
    src = what.suffix().str();
  }
  return foundStrings;
}

std::vector<std::string> GetIncludePaths(const char* buildOptions) {
  std::vector<std::string> includePaths;
  if (buildOptions != nullptr) {
    includePaths = GetStringsWithRegex(std::string(buildOptions), "-I\\s*[^\\s]+", "-I\\s*");
  }
  includePaths.push_back(std::filesystem::current_path().string());
  return includePaths;
}

void CreateHeaderFiles(const std::vector<std::string>& sourceNamesToScan,
                       const std::vector<std::string>& searchPaths,
                       std::set<std::string>& alreadyCreatedHeaders,
                       const bool includeMainFiles) {
  for (const auto& header : sourceNamesToScan) {
    if (alreadyCreatedHeaders.find(header) != alreadyCreatedHeaders.end()) {
      continue;
    }
    for (const auto& searchPath : searchPaths) {
      std::filesystem::path headerPath = header;
      if (!std::filesystem::exists(headerPath)) {
        headerPath = std::filesystem::path(searchPath) / header;
      }
      std::ifstream loadHeader(headerPath);
      if (loadHeader.is_open()) {
        if (includeMainFiles) {
          const auto headerFileName = headerPath.filename();
          std::filesystem::path path =
              gits::Config::Get().common.recorder.dumpPath / "gitsFiles" / headerFileName;
          if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path.parent_path());
            std::filesystem::copy_file(headerPath, path);
          }
        }
        std::string srcHeader(std::istreambuf_iterator<char>(loadHeader),
                              (std::istreambuf_iterator<char>()));
        alreadyCreatedHeaders.insert(header);
        const auto otherIncludeFiles = GetStringsWithRegex(
            std::move(srcHeader), R"(#include\s*["<]([^">]+))", "#include\\s*[<\"]*");
        CreateHeaderFiles(otherIncludeFiles, searchPaths, alreadyCreatedHeaders, true);
      }
    }
  }
}

const std::filesystem::path GetDumpPath(const Config& cfg) {
  if (Config::IsRecorder()) {
    return cfg.common.recorder.dumpPath / "dump";
  }
  return cfg.common.player.outputDir.empty() ? cfg.common.player.streamDir / "dump"
                                             : cfg.common.player.outputDir;
}

} //namespace gits

#ifndef BUILD_FOR_CCODE
gits::ShadowBuffer::~ShadowBuffer() {
  if (_shadow.get() != nullptr && _shadow.use_count() == 1) {
    if (_pagealigned) {
#ifdef GITS_PLATFORM_WINDOWS
      VirtualFree(*_shadow, 0, MEM_RELEASE);
#else
      munmap(*_shadow, _size);
#endif
    } else {
      free(*_shadow);
    }
  }
}

void gits::ShadowBuffer::Init(bool pagealigned, size_t size, void* orig, bool isWriteWatch) {
  if (Initialized()) {
    return;
  }
  _size = size;
  _pagealigned = pagealigned;
  void* shadow;
  if (_pagealigned) {
#ifdef GITS_PLATFORM_WINDOWS
    if (isWriteWatch) {
      shadow =
          VirtualAlloc(nullptr, _size, MEM_COMMIT | MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
    } else {
      shadow = VirtualAlloc(nullptr, _size, MEM_COMMIT, PAGE_READWRITE);
    }
#else
    shadow = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
#endif
  } else {
    shadow = malloc(_size);
  }
  _shadow.reset(new void*(shadow));
  _orig = orig;
}

void gits::ShadowBuffer::Flush(size_t offset, size_t size) {
  assert(_shadow != 0 && _orig != 0);
  char* shadowPtr = (char*)*_shadow;
  char* origPtr = (char*)_orig;
  shadowPtr = shadowPtr + offset;
  origPtr = origPtr + offset;
  memcpy(origPtr, shadowPtr, size);
}

void gits::ShadowBuffer::UpdateShadow(size_t offset, size_t size) {
  char* origPtr = (char*)_orig;
  origPtr = origPtr + offset;
  UpdateShadowFromSource(origPtr, offset, size);
}

void gits::ShadowBuffer::UpdateShadowFromSource(void* ptr, size_t offset, size_t size) {
  assert(_shadow != 0 && _orig != 0);
  char* shadowPtr = (char*)*_shadow;
  shadowPtr = shadowPtr + offset;
  memcpy(shadowPtr, ptr, size);
}

// GetSubrangeOverlappingMemoryPages - function takes a set of pages and a
// single memory range. It returns a smallest possible range that includes all of
// the bytes belonging to the pages, that has been included in the input range.
std::pair<const void*, size_t> gits::GetSubrangeOverlappingMemoryPages(
    std::pair<const void*, size_t> range, const std::set<const void*>& pages) {
  const void* ptr = range.first;
  size_t size = range.second;
  const void* ptrEnd = (void*)((char*)ptr + size);

  std::set<const void*> overlappingPages;
  size_t pageSize = GetVirtualMemoryPageSize();
  // Get overlapping pages
  for (auto pagePtr : pages) {
    auto pagePtrEnd = (const void*)((const char*)pagePtr + pageSize);
    size_t sizesSum = size + pageSize;
    size_t maxRange = (size_t)(std::max((uint64_t)pagePtrEnd, (uint64_t)ptrEnd) -
                               std::min((uint64_t)pagePtr, (uint64_t)ptr));
    if (sizesSum > maxRange) {
      overlappingPages.insert(pagePtr);
    }
  }

  // Get one pages range
  std::pair<const void*, size_t> newRange;
  if (overlappingPages.size() > 0) {
    const void* pageRangePtr = *overlappingPages.begin();
    const void* pageRangeEndPtr = (const void*)((const char*)*overlappingPages.rbegin() + pageSize);
    const void* rangePtr = std::max(ptr, pageRangePtr);
    const void* rangeEndPtr = std::min(ptrEnd, pageRangeEndPtr);
    newRange.first = rangePtr;
    newRange.second = (size_t)((size_t)rangeEndPtr - (size_t)rangePtr);
  } else {
    newRange.first = ptr;
    newRange.second = 0;
  }
  return newRange;
}

std::vector<std::pair<uint64_t, uint64_t>> gits::GetIntervalSetFromMemoryPages(
    std::pair<const void*, size_t> range, const std::set<const void*>& pages) {
  const void* ptr = range.first;
  size_t size = range.second;
  const void* ptrEnd = (void*)((char*)ptr + size);

  size_t pageSize = GetVirtualMemoryPageSize();
  std::vector<std::pair<uint64_t, uint64_t>> pagesMap;

  for (auto pagePtr : pages) {
    uint64_t addr_begin = std::max((uint64_t)ptr, (uint64_t)pagePtr);
    uint64_t addr_end = std::min((uint64_t)((char*)pagePtr + pageSize), (uint64_t)ptrEnd);

    if (addr_begin < addr_end) {
      if ((pagesMap.size() > 0) && (addr_begin >= pagesMap.back().first) &&
          (addr_begin <= pagesMap.back().second + 1)) {
        if (addr_end > pagesMap.back().second) {
          pagesMap.back().second = (uint64_t)addr_end;
        }
      } else {
        pagesMap.push_back({(uint64_t)addr_begin, (uint64_t)addr_end});
      }
    }
  }
  return pagesMap;
}

std::vector<std::pair<const uint8_t*, const uint8_t*>> gits::GetChangedMemorySubranges(
    const void* oldData, const void* newRangeData, uint64_t length, size_t stepSize) {
  const uint8_t* newPtr = (const uint8_t*)newRangeData;
  const uint8_t* newPtrEnd = newPtr + length;
  const uint8_t* oldPtr = (const uint8_t*)oldData;

  std::vector<std::pair<const uint8_t*, const uint8_t*>> pagesMap;
  bool continuous = false;

  while (newPtr < newPtrEnd) {
    const uint8_t* max_page_value = std::min(newPtr + stepSize, newPtrEnd);
    const size_t range_size = max_page_value - newPtr;
    const int out_res = memcmp(oldPtr, newPtr, range_size);
    if (out_res != 0) {
      if (continuous) {
        pagesMap.back().second = max_page_value;
      } else {
        pagesMap.push_back({newPtr, max_page_value});
        continuous = true;
      }
    } else {
      continuous = false;
    }
    newPtr += stepSize;
    oldPtr += stepSize;
  }

  return pagesMap;
}

void gits::GetMemoryDiffSubRange(const void* oldData,
                                 const void* newRangeData,
                                 uint64_t& length,
                                 uint64_t& offset) {
  const uint8_t* minOldPtr = (const uint8_t*)((const uint8_t*)oldData + offset); //+ offset
  const uint8_t* maxOldPtr = (const uint8_t*)(minOldPtr + length);
  const uint8_t* minNewPtr = (const uint8_t*)((const uint8_t*)newRangeData + offset);
  const uint8_t* maxNewPtr = (const uint8_t*)(minNewPtr + length);

  while (minOldPtr < maxOldPtr && *minOldPtr == *minNewPtr) {
    minNewPtr++;
    minOldPtr++;
  }
  while (minOldPtr < maxOldPtr) {
    maxNewPtr--;
    maxOldPtr--;
    if (*maxOldPtr != *maxNewPtr) {
      maxNewPtr++;
      maxOldPtr++;
      break;
    }
  }

  offset = minNewPtr - (const uint8_t*)newRangeData;
  length = maxOldPtr - minOldPtr;
}
#endif

uint64_t gits::LZ4StreamCompressor::Compress(const char* uncompressedData,
                                             const uint64_t uncompressedDataSize,
                                             std::vector<char>* compressedData) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (uncompressedDataSize > INT_MAX) {
    Log(ERR) << "LZ4 Compress failed due to int overflow.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  uint64_t returnValue = 0;
  uint64_t lz4MaxCompressedSize =
      static_cast<uint64_t>(LZ4_compressBound(static_cast<int>(uncompressedDataSize)));
  if (lz4MaxCompressedSize > compressedData->size()) {
    compressedData->resize(lz4MaxCompressedSize);
  }
  int returnedCompressedSize = LZ4_compress_fast_extState(
      &ctx, uncompressedData, compressedData->data(), static_cast<int32_t>(uncompressedDataSize),
      static_cast<int32_t>(lz4MaxCompressedSize),
      perfModes.at(Config::Get().common.recorder.compression.level));
  if (returnedCompressedSize <= 0) {
    Log(ERR) << "LZ4 Compress failed.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  } else {
    returnValue = returnedCompressedSize;
  }
  return returnValue;
}

uint64_t gits::LZ4StreamCompressor::Decompress(const std::vector<char>& compressedData,
                                               const uint64_t compressedDataSize,
                                               const uint64_t expectedUncompressedSize,
                                               char* uncompressedData) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (compressedDataSize > INT_MAX || expectedUncompressedSize > INT_MAX) {
    Log(ERR) << "LZ4 Decompress failed due to int overflow.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  uint64_t returnValue = 0;
  int returnedUncompressedSize = LZ4_decompress_safe(
      compressedData.data(), uncompressedData, static_cast<int32_t>(compressedDataSize),
      static_cast<int32_t>(expectedUncompressedSize));
  if (returnedUncompressedSize <= 0) {
    Log(ERR) << "LZ4 Decompress failed with error code:" << returnedUncompressedSize;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  } else {
    returnValue = returnedUncompressedSize;
  }
  return returnValue;
}

uint64_t gits::LZ4StreamCompressor::MaxCompressedSize(const uint64_t dataSize) {
  return static_cast<uint64_t>(LZ4_compressBound(static_cast<int>(dataSize)));
}

gits::ZSTDStreamCompressor::ZSTDStreamCompressor() {
  ZSTDContext = ZSTD_createCCtx();
}

gits::ZSTDStreamCompressor::~ZSTDStreamCompressor() {
  ZSTD_freeCCtx(ZSTDContext);
}

uint64_t gits::ZSTDStreamCompressor::Compress(const char* uncompressedData,
                                              const uint64_t uncompressedDataSize,
                                              std::vector<char>* compressedData) {
  std::unique_lock<std::mutex> lock(mutex_);
  uint64_t zstdMaxCompressedSize = ZSTD_compressBound(uncompressedDataSize);
  if (zstdMaxCompressedSize > compressedData->size()) {
    compressedData->resize(zstdMaxCompressedSize);
  }
  uint64_t returnedCompressedSize = ZSTD_compressCCtx(
      ZSTDContext, compressedData->data(), zstdMaxCompressedSize, uncompressedData,
      uncompressedDataSize, perfModes.at(Config::Get().common.recorder.compression.level));
  if (ZSTD_isError(returnedCompressedSize)) {
    Log(ERR) << "ZSTD Compress failed with error code:" << returnedCompressedSize;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return returnedCompressedSize;
}

uint64_t gits::ZSTDStreamCompressor::Decompress(const std::vector<char>& compressedData,
                                                const uint64_t compressedDataSize,
                                                const uint64_t expectedUncompressedSize,
                                                char* uncompressedData) {
  std::unique_lock<std::mutex> lock(mutex_);
  uint64_t returnedUncompressedSize = ZSTD_decompress(uncompressedData, expectedUncompressedSize,
                                                      compressedData.data(), compressedDataSize);
  if (ZSTD_isError(returnedUncompressedSize)) {
    Log(ERR) << "ZSTD Decompress failed with error code:" << returnedUncompressedSize;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return returnedUncompressedSize;
}

uint64_t gits::ZSTDStreamCompressor::MaxCompressedSize(const uint64_t dataSize) {
  return static_cast<uint64_t>(ZSTD_compressBound(dataSize));
}

#if defined(GITS_PLATFORM_WINDOWS)
std::string gits::GetRenderDocDllPath() {
  std::string dllpath = "";
  HKEY hKey = HKEY_LOCAL_MACHINE;
  LPCSTR subKey = "SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers";
  LSTATUS status;
  HKEY hkResult;
  status = RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &hkResult);
  DWORD valueCount;
  DWORD maxValueNameLen;
  status = RegQueryInfoKey(hkResult, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           &valueCount, &maxValueNameLen, nullptr, nullptr, nullptr);
  maxValueNameLen++;
  auto nameBuffer = std::make_unique<char[]>(maxValueNameLen);
  DWORD valueNameLen;
  for (DWORD i = 0; i < valueCount; i++) {
    valueNameLen = maxValueNameLen;
    status = RegEnumValue(hkResult, i, nameBuffer.get(), &valueNameLen, nullptr, nullptr, nullptr,
                          nullptr);
    std::string name = std::string(nameBuffer.get());
    if (name.substr(name.find_last_of('\\') + 1) == "renderdoc.json") {
      dllpath = name.replace(name.find_last_of('.') + 1, 4, "dll");
      break;
    }
  }
  RegCloseKey(hkResult);
  if (dllpath == "") {
    throw std::runtime_error("Cannot find renderdoc.dll. Use 'renderDocDllPath' option to specify "
                             "renderdoc.dll location.");
  }
  return dllpath;
}
#endif
