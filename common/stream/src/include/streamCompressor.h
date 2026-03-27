// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "zstd.h"

#include <memory>

namespace gits {
namespace stream {

class StreamCompressor {
public:
  StreamCompressor() = default;
  virtual ~StreamCompressor() {}
  StreamCompressor(const StreamCompressor&) = delete;
  StreamCompressor& operator=(const StreamCompressor&) = delete;

  virtual uint64_t CompressBound(uint64_t uncompressedSize) = 0;
  virtual uint64_t Compress(const char* src,
                            char* dest,
                            uint64_t srcSize,
                            uint64_t destCapacity) = 0;
};

class StreamDecompressor {
public:
  StreamDecompressor() = default;
  virtual ~StreamDecompressor() {}
  StreamDecompressor(const StreamDecompressor&) = delete;
  StreamDecompressor& operator=(const StreamDecompressor&) = delete;

  virtual uint64_t Decompress(const char* src,
                              char* dest,
                              uint64_t srcSize,
                              uint64_t destCapacity) = 0;
};

class LZ4StreamCompressor : public StreamCompressor {
public:
  LZ4StreamCompressor();
  uint64_t CompressBound(uint64_t uncompressedSize) override;
  uint64_t Compress(const char* src, char* dest, uint64_t srcSize, uint64_t destCapacity) override;

private:
  int m_Acceleration{};
  std::unique_ptr<char[]> m_CompressionState;
};

class LZ4StreamDecompressor : public StreamDecompressor {
public:
  uint64_t Decompress(const char* src,
                      char* dest,
                      uint64_t srcSize,
                      uint64_t destCapacity) override;
};

class ZSTDStreamCompressor : public StreamCompressor {
public:
  ZSTDStreamCompressor();
  uint64_t CompressBound(uint64_t uncompressedSize) override;
  uint64_t Compress(const char* src, char* dest, uint64_t srcSize, uint64_t destCapacity) override;

private:
  int m_CompressionLevel{};
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> m_ZSTDContext;
};

class ZSTDStreamDecompressor : public StreamDecompressor {
public:
  uint64_t Decompress(const char* src,
                      char* dest,
                      uint64_t srcSize,
                      uint64_t destCapacity) override;
};

} // namespace stream
} // namespace gits
