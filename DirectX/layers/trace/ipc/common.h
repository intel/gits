// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include <cstring>
#include <algorithm>

constexpr int SHARED_MEMORY_SIZE = 10000000;

struct SharedCircularBuffer {
  int head;
  int tail;
  char buffer[SHARED_MEMORY_SIZE - sizeof(head) - sizeof(tail)];
};

size_t write(SharedCircularBuffer* sharedBuffer, const char* data, size_t bytes) {
  if (bytes == 0) {
    return 0;
  }

  constexpr size_t capacity = sizeof(sharedBuffer->buffer);
  size_t size1 = 0;
  size_t size2 = 0;

  if (sharedBuffer->head > sharedBuffer->tail) {
    size1 = capacity - sharedBuffer->head;
    size2 = sharedBuffer->tail;
  } else {
    size1 = sharedBuffer->tail - sharedBuffer->head;
    size1 = std::min(size1, capacity - sharedBuffer->head);
  }

  if (size1 + size2 == 0) {
    return 0;
  }

  size_t bytesToWrite = std::min(bytes, size1 + size2);

  if (bytesToWrite <= size1) {
    memcpy(sharedBuffer->buffer + sharedBuffer->head, data, bytesToWrite);
    sharedBuffer->head += bytesToWrite;
    if (sharedBuffer->head == capacity) {
      sharedBuffer->head = 0;
    }
  } else {
    memcpy(sharedBuffer->buffer + sharedBuffer->head, data, size1);
    const size_t remaining = bytesToWrite - size1;
    memcpy(sharedBuffer->buffer, data + size1, remaining);
    sharedBuffer->head = remaining;
  }

  return bytesToWrite;
}

void writeAll(SharedCircularBuffer* sharedBuffer, const char* data, size_t bytes) {
  size_t remaining = bytes;
  const char* currentData = data;
  while (remaining > 0) {
    size_t written = write(sharedBuffer, currentData, remaining);
    currentData += written;
    remaining -= written;
  }
}

size_t read(SharedCircularBuffer* sharedBuffer, char* data, size_t bytes) {
  if (bytes == 0) {
    return 0;
  }

  constexpr size_t capacity = sizeof(sharedBuffer->buffer);
  size_t size1 = 0;
  size_t size2 = 0;

  if (sharedBuffer->head > sharedBuffer->tail) {
    size1 = (sharedBuffer->head - sharedBuffer->tail) - 1;
    size1 = std::min(size1, (capacity - sharedBuffer->tail) - 1);
  } else {
    size1 = (capacity - sharedBuffer->tail) - 1;
    size2 = sharedBuffer->head;
  }

  if (size1 + size2 == 0) {
    return 0;
  }

  size_t bytesToRead = std::min(bytes, size1 + size2);

  if (bytesToRead <= size1) {
    memcpy(data, sharedBuffer->buffer + sharedBuffer->tail + 1, bytesToRead);
    sharedBuffer->tail += bytesToRead;
  } else {
    memcpy(data, sharedBuffer->buffer + sharedBuffer->tail + 1, size1);
    const size_t remaining = bytesToRead - size1;
    memcpy(data + size1, sharedBuffer->buffer, remaining);
    sharedBuffer->tail = remaining - 1;
  }

  return bytesToRead;
}
