// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "commandFactory.h"
#include "streamReader.h"
#include "streamCompressor.h"

#include <iostream>
#include <memory>
#include <thread>
#include <array>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace gits {
namespace stream {

class StreamLegacyReader : public BaseStreamReader {
public:
  StreamLegacyReader(std::vector<CommandFactory*>& commandFactories, std::istream& stream);
  void Run() override;
  void Close() override;

private:
  std::vector<CommandFactory*>& m_CommandFactories;
  std::istream& m_Stream;
  const unsigned INITIAL_BLOCK_ALLOC = 1024 * 1024 * 4 * 2;
  const unsigned INITIAL_CROSSING_DATA_ALLOC = 1024;

  std::unique_ptr<StreamDecompressor> m_Decompressor;
  std::thread m_DecompressionThread;
  std::thread m_ReadingThread;

  struct Block {
    unsigned Index{};
    unsigned Id{};
    std::unique_ptr<char[]> Data;
    unsigned DataAlloc{};
    unsigned DataSize{};
    std::atomic<bool> DataFull{};
  };
  struct CompressedBlock : Block {
    unsigned UncompressedDataSize{};
  };
  struct UncompressedBlock : Block {
    std::vector<std::unique_ptr<CommandRunner>> Runners;
    std::atomic<bool> RunnersFull{};
    unsigned Spanning{};
    std::unique_ptr<char[]> SpanningData;
    unsigned SpanningDataAlloc{};
  };

  static const unsigned NUMBER_OF_BLOCKS{8};
  std::array<UncompressedBlock, NUMBER_OF_BLOCKS> m_UncompressedBlocks;
  std::array<CompressedBlock, NUMBER_OF_BLOCKS> m_CompressedBlocks;

  std::condition_variable m_ReadDoneCondition;
  std::condition_variable m_DecompressionDoneCondition;
  std::condition_variable m_RunDoneCondition;

  std::mutex m_Mutex;

  bool m_ReadFinished{};
  bool m_RunFinished{};
  unsigned m_LastRead{};
  unsigned m_LastDecompressed{};
  std::atomic<bool> m_Closed{};

private:
  void ReadCompressedBlocks();
  void ReadCompressedBlock(unsigned blockId, uint64_t compressedSize, uint64_t uncompressedSize);
  void Decompress();
  void DecodeBlock(UncompressedBlock& block, std::vector<UncompressedBlock*>& previousBlocks);
  CompressedBlock* FindBlockForRead(std::unique_lock<std::mutex>& lock, unsigned size);
  UncompressedBlock* FindBlockForRun(std::unique_lock<std::mutex>& lock, unsigned blockId);
  unsigned Align(unsigned value);
};

} // namespace stream
} // namespace gits
