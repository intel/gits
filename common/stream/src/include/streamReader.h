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
#include "streamCompressor.h"

#include <iostream>
#include <memory>
#include <thread>
#include <array>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

namespace gits {
namespace stream {

class BaseStreamReader {
public:
  BaseStreamReader() = default;
  virtual ~BaseStreamReader() {}
  BaseStreamReader(const BaseStreamReader&) = delete;
  BaseStreamReader& operator=(const BaseStreamReader&) = delete;

  virtual void Run() = 0;
  virtual void Close() = 0;
};

class StreamReader : public BaseStreamReader {
public:
  StreamReader(std::vector<CommandFactory*>& commandFactories, std::istream& stream);
  void Run() override;
  void Close() override;

private:
  std::vector<CommandFactory*>& m_CommandFactories;
  std::istream& m_Stream;
  const unsigned INITIAL_BLOCK_ALLOC = 1024 * 1024 * 4 * 2;

  static const unsigned NUMBER_OF_DECOMPRESSION_THREADS{4};
  std::unique_ptr<StreamDecompressor> m_Decompressor;
  std::array<std::thread, NUMBER_OF_DECOMPRESSION_THREADS> m_DecompressionThreads;
  std::thread m_ReadingThread;

  struct Block {
    unsigned Index{};
    unsigned Id{};
    std::unique_ptr<char[]> Data;
    uint64_t DataAlloc{};
    uint64_t DataSize{};
    bool Full{};
    bool Decompressing{};
  };
  struct CompressedBlock : Block {
    uint64_t UncompressedDataSize{};
  };
  struct UncompressedBlock : Block {
    std::vector<std::unique_ptr<CommandRunner>> Runners;
  };

  static const unsigned NUMBER_OF_BLOCKS{NUMBER_OF_DECOMPRESSION_THREADS * 2};
  std::array<UncompressedBlock, NUMBER_OF_BLOCKS> m_UncompressedBlocks;
  std::array<CompressedBlock, NUMBER_OF_BLOCKS> m_CompressedBlocks;

  std::condition_variable m_ReadDoneCondition;
  std::condition_variable m_DecompressionDoneCondition;

  struct WaitForRunDoneInfo {
    bool Waiting{};
    std::condition_variable Condition;
    unsigned BlockIndex{};
  };
  std::array<WaitForRunDoneInfo, NUMBER_OF_DECOMPRESSION_THREADS> m_WaitsForRunDone;

  std::mutex m_Mutex;
  std::queue<unsigned> m_DecompressionQueue;

  bool m_ReadFinished{};
  bool m_RunFinished{};
  unsigned m_LastRead{};
  std::atomic<bool> m_Closed{};

private:
  void ReadCompressedBlocks();
  void Decompress(unsigned threadIndex);
  void DecodeBlock(UncompressedBlock& block);
  CompressedBlock* FindBlockForRead(std::unique_lock<std::mutex>& lock, uint64_t size);
  UncompressedBlock* FindBlockForRun(std::unique_lock<std::mutex>& lock, unsigned blockId);
  void WaitForRunDone(std::unique_lock<std::mutex>& lock,
                      unsigned threadIndex,
                      unsigned blockIndex);
  void NotifyRunDone(unsigned blockIndex);
  void NotifyRunDoneAll();
  uint64_t Align(uint64_t value);
};

} // namespace stream
} // namespace gits
