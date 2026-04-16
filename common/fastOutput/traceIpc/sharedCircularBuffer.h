// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstring>
#include <algorithm>

constexpr int SHARED_MEMORY_SIZE = 10000000;

struct SharedCircularBuffer {
  int Head;
  int Tail;
  char Buffer[SHARED_MEMORY_SIZE - sizeof(Head) - sizeof(Tail)];
};

inline size_t Write(SharedCircularBuffer* sharedBuffer, const char* data, size_t bytes) {
  if (bytes == 0) {
    return 0;
  }

  constexpr size_t capacity = sizeof(sharedBuffer->Buffer);
  size_t size1 = 0;
  size_t size2 = 0;

  if (sharedBuffer->Head > sharedBuffer->Tail) {
    size1 = capacity - sharedBuffer->Head;
    size2 = sharedBuffer->Tail;
  } else {
    size1 = sharedBuffer->Tail - sharedBuffer->Head;
    size1 = std::min(size1, capacity - sharedBuffer->Head);
  }

  if (size1 + size2 == 0) {
    return 0;
  }

  size_t bytesToWrite = std::min(bytes, size1 + size2);

  if (bytesToWrite <= size1) {
    std::memcpy(sharedBuffer->Buffer + sharedBuffer->Head, data, bytesToWrite);
    sharedBuffer->Head += static_cast<int>(bytesToWrite);
    if (static_cast<size_t>(sharedBuffer->Head) == capacity) {
      sharedBuffer->Head = 0;
    }
  } else {
    std::memcpy(sharedBuffer->Buffer + sharedBuffer->Head, data, size1);
    const size_t remaining = bytesToWrite - size1;
    std::memcpy(sharedBuffer->Buffer, data + size1, remaining);
    sharedBuffer->Head = static_cast<int>(remaining);
  }

  return bytesToWrite;
}

inline void WriteAll(SharedCircularBuffer* sharedBuffer, const char* data, size_t bytes) {
  size_t remaining = bytes;
  const char* currentData = data;
  while (remaining > 0) {
    size_t written = Write(sharedBuffer, currentData, remaining);
    currentData += written;
    remaining -= written;
  }
}

inline size_t Read(SharedCircularBuffer* sharedBuffer, char* data, size_t bytes) {
  if (bytes == 0) {
    return 0;
  }

  constexpr size_t capacity = sizeof(sharedBuffer->Buffer);
  size_t size1 = 0;
  size_t size2 = 0;

  if (sharedBuffer->Head > sharedBuffer->Tail) {
    size1 = (sharedBuffer->Head - sharedBuffer->Tail) - 1;
    size1 = std::min(size1, (capacity - sharedBuffer->Tail) - 1);
  } else {
    size1 = (capacity - sharedBuffer->Tail) - 1;
    size2 = sharedBuffer->Head;
  }

  if (size1 + size2 == 0) {
    return 0;
  }

  size_t bytesToRead = std::min(bytes, size1 + size2);

  if (bytesToRead <= size1) {
    std::memcpy(data, sharedBuffer->Buffer + sharedBuffer->Tail + 1, bytesToRead);
    sharedBuffer->Tail += static_cast<int>(bytesToRead);
  } else {
    std::memcpy(data, sharedBuffer->Buffer + sharedBuffer->Tail + 1, size1);
    const size_t remaining = bytesToRead - size1;
    std::memcpy(data + size1, sharedBuffer->Buffer, remaining);
    sharedBuffer->Tail = static_cast<int>(remaining) - 1;
  }

  return bytesToRead;
}
