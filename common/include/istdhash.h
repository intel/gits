// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include <vector>
#include <cstdint>
#include <cstring>

#define HASH_JENKINS_MIX(a, b, c)                                                                  \
  {                                                                                                \
    a -= b;                                                                                        \
    a -= c;                                                                                        \
    a ^= (c >> 13);                                                                                \
    b -= c;                                                                                        \
    b -= a;                                                                                        \
    b ^= (a << 8);                                                                                 \
    c -= a;                                                                                        \
    c -= b;                                                                                        \
    c ^= (b >> 13);                                                                                \
    a -= b;                                                                                        \
    a -= c;                                                                                        \
    a ^= (c >> 12);                                                                                \
    b -= c;                                                                                        \
    b -= a;                                                                                        \
    b ^= (a << 16);                                                                                \
    c -= a;                                                                                        \
    c -= b;                                                                                        \
    c ^= (b >> 5);                                                                                 \
    a -= b;                                                                                        \
    a -= c;                                                                                        \
    a ^= (c >> 3);                                                                                 \
    b -= c;                                                                                        \
    b -= a;                                                                                        \
    b ^= (a << 10);                                                                                \
    c -= a;                                                                                        \
    c -= b;                                                                                        \
    c ^= (b >> 15);                                                                                \
  }

inline uint64_t ISTDHash(const char* data, uint32_t count) {
  uint32_t size_uint32_t = count / sizeof(uint32_t);
  const uint32_t* data_uint32 = reinterpret_cast<const uint32_t*>(data);

  DWORD a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
  while (size_uint32_t--) {
    a ^= *(data_uint32++);
    HASH_JENKINS_MIX(a, hi, lo);
  }
  return (((uint64_t)hi) << 32) | lo;
}

inline uint64_t ISTDHashPadding(const char* data, uint32_t count) {
  uint32_t count_new = count;
  while (count_new % sizeof(uint32_t)) {
    count_new++;
  }
  std::vector<char> new_source;
  new_source.resize(count_new);
  std::memcpy(new_source.data(), data, count);
  uint32_t count_diff = count_new - count;
  if (count_diff) {
    char* pointerToData = &new_source[count];
    std::memset(pointerToData, 0, count_diff);
  }
  uint32_t size_uint32_t = count_new / sizeof(uint32_t);
  const uint32_t* data_uint32 = reinterpret_cast<const uint32_t*>(&new_source[0]);

  DWORD a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
  while (size_uint32_t--) {
    a ^= *(data_uint32++);
    HASH_JENKINS_MIX(a, hi, lo);
  }
  return (((uint64_t)hi) << 32) | lo;
}
#undef HASH_JENKINS_MIX
