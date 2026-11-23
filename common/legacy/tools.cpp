// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <fstream>

#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if !defined GITS_ARCH_ARM && !defined GITS_ARCH_A64

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
      LOG_ERROR << "Can't get name of the process.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    processName = buffer;
  } else {
    LOG_ERROR << "Can't open handle to the process.";
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

} //namespace gits
