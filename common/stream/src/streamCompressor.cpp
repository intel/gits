// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamCompressor.h"

#include "configurator.h"
#include "lz4.h"

namespace gits {
namespace stream {

LZ4StreamCompressor::LZ4StreamCompressor() {
  m_Acceleration = 1;
  m_CompressionState.reset(new char[LZ4_sizeofState()]);
}

uint64_t LZ4StreamCompressor::CompressBound(uint64_t uncompressedSize) {
  return LZ4_compressBound(uncompressedSize);
}

uint64_t LZ4StreamCompressor::Compress(const char* src,
                                       char* dest,
                                       uint64_t srcSize,
                                       uint64_t destCapacity) {
  return LZ4_compress_fast_extState(m_CompressionState.get(), src, dest, srcSize, destCapacity,
                                    m_Acceleration);
}

uint64_t LZ4StreamDecompressor::Decompress(const char* src,
                                           char* dest,
                                           uint64_t srcSize,
                                           uint64_t destCapacity) {
  int ret = LZ4_decompress_safe(src, dest, srcSize, destCapacity);
  return ret >= 0 ? ret : 0;
}

ZSTDStreamCompressor::ZSTDStreamCompressor()
    : m_CompressionLevel{3}, m_ZSTDContext{ZSTD_createCCtx(), ZSTD_freeCCtx} {
  GITS_ASSERT(m_ZSTDContext != nullptr, "Failed to create a ZSTD compression context.");
}

uint64_t ZSTDStreamCompressor::CompressBound(uint64_t uncompressedSize) {
  return ZSTD_compressBound(uncompressedSize);
}

uint64_t ZSTDStreamCompressor::Compress(const char* src,
                                        char* dest,
                                        uint64_t srcSize,
                                        uint64_t destCapacity) {
  return ZSTD_compressCCtx(m_ZSTDContext.get(), dest, destCapacity, src, srcSize,
                           m_CompressionLevel);
}

uint64_t ZSTDStreamDecompressor::Decompress(const char* src,
                                            char* dest,
                                            uint64_t srcSize,
                                            uint64_t destCapacity) {
  return ZSTD_decompress(dest, destCapacity, src, srcSize);
}

} // namespace stream
} // namespace gits
