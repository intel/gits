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

  virtual int CompressBound(int uncompressedSize) = 0;
  virtual int Compress(const char* src, char* dest, int srcSize, int destCapacity) = 0;
};

class StreamDecompressor {
public:
  StreamDecompressor() = default;
  virtual ~StreamDecompressor() {}
  StreamDecompressor(const StreamDecompressor&) = delete;
  StreamDecompressor& operator=(const StreamDecompressor&) = delete;

  virtual int Decompress(const char* src, char* dest, int srcSize, int destCapacity) = 0;
};

class LZ4StreamCompressor : public StreamCompressor {
public:
  LZ4StreamCompressor();
  int CompressBound(int uncompressedSize) override;
  int Compress(const char* src, char* dest, int srcSize, int destCapacity) override;

private:
  int m_Acceleration{};
  std::unique_ptr<char[]> m_CompressionState;
};

class LZ4StreamDecompressor : public StreamDecompressor {
public:
  int Decompress(const char* src, char* dest, int srcSize, int destCapacity) override;
};

class ZSTDStreamCompressor : public StreamCompressor {
public:
  ZSTDStreamCompressor();
  int CompressBound(int uncompressedSize) override;
  int Compress(const char* src, char* dest, int srcSize, int destCapacity) override;

private:
  int m_CompressionLevel{};
  ZSTD_CCtx* m_ZSTDContext{};
};

class ZSTDStreamDecompressor : public StreamDecompressor {
public:
  int Decompress(const char* src, char* dest, int srcSize, int destCapacity) override;
};

} // namespace stream
} // namespace gits
