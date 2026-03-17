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

int LZ4StreamCompressor::CompressBound(int uncompressedSize) {
  return LZ4_compressBound(uncompressedSize);
}

int LZ4StreamCompressor::Compress(const char* src, char* dest, int srcSize, int destCapacity) {
  return LZ4_compress_fast_extState(m_CompressionState.get(), src, dest, srcSize, destCapacity,
                                    m_Acceleration);
}

int LZ4StreamDecompressor::Decompress(const char* src, char* dest, int srcSize, int destCapacity) {
  return LZ4_decompress_safe(src, dest, srcSize, destCapacity);
}

ZSTDStreamCompressor::ZSTDStreamCompressor() {
  m_CompressionLevel = Configurator::Get().common.recorder.compression.level;
  m_ZSTDContext = ZSTD_createCCtx();
}

int ZSTDStreamCompressor::CompressBound(int uncompressedSize) {
  return ZSTD_compressBound(uncompressedSize);
}

int ZSTDStreamCompressor::Compress(const char* src, char* dest, int srcSize, int destCapacity) {
  return ZSTD_compressCCtx(m_ZSTDContext, dest, destCapacity, src, srcSize, m_CompressionLevel);
}

int ZSTDStreamDecompressor::Decompress(const char* src, char* dest, int srcSize, int destCapacity) {
  return ZSTD_decompress(dest, destCapacity, src, srcSize);
}

} // namespace stream
} // namespace gits
