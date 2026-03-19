// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "streamCompressor.h"
#include "enumsAuto.h"

#include <fstream>
#include <memory>
#include <thread>
#include <array>
#include <condition_variable>
#include <mutex>
#include <filesystem>
#include <queue>

namespace gits {
namespace stream {

class StreamWriter {
public:
  StreamWriter(const std::filesystem::path& streamDir, CompressionType compressionType);
  ~StreamWriter();

  void Record(const CommandSerializer& commandSerializer);
  void Close();

private:
  std::ofstream m_Stream;
  std::string m_StreamDir;
  const unsigned TRIGGER_BLOCK_SIZE = 1024 * 1024 * 4;
  const unsigned INITIAL_BLOCK_ALLOC = TRIGGER_BLOCK_SIZE * 2;
  unsigned m_RecordedBlockId{};
  unsigned m_WrittenBlockId{};
  unsigned m_CurrentRecordBlock{};

  static const unsigned NUMBER_OF_COMPRESSION_THREADS{4};
  std::array<std::unique_ptr<StreamCompressor>, NUMBER_OF_COMPRESSION_THREADS> m_Compressors;
  std::array<std::thread, NUMBER_OF_COMPRESSION_THREADS> m_CompressionThreads;
  std::thread m_WritingThread;

  struct Block {
    unsigned Index{};
    unsigned Id{};
    std::unique_ptr<char[]> Data;
    unsigned DataAlloc{};
    unsigned DataSize{};
    bool Full{};
    bool Compressing{};
  };
  struct CompressedBlock : Block {
    unsigned UncompressedDataSize{};
  };

  static const unsigned NUMBER_OF_BLOCKS{NUMBER_OF_COMPRESSION_THREADS * 2};
  std::array<Block, NUMBER_OF_BLOCKS> m_UncompressedBlocks;
  std::array<CompressedBlock, NUMBER_OF_BLOCKS> m_CompressedBlocks;

  std::condition_variable m_RecordDoneCondition;
  std::condition_variable m_CompressionDoneCondition;

  struct WaitForWriteDoneInfo {
    bool Waiting{};
    std::condition_variable Condition;
    unsigned BlockId{};
    unsigned BlockSize{};
  };
  std::array<WaitForWriteDoneInfo, NUMBER_OF_COMPRESSION_THREADS> m_WaitsForWriteDone;

  std::mutex m_Mutex;
  std::queue<unsigned> m_CompressionQueue;

  std::atomic<bool> m_StopThreads{};

private:
  void WriteCompressedBlocks();
  void Compress(unsigned threadIndex);
  Block* FindBlockForRecord(std::unique_lock<std::mutex>& lock, unsigned size);
  void WaitForWriteDone(std::unique_lock<std::mutex>& lock,
                        unsigned threadIndex,
                        unsigned blockId,
                        unsigned blockSize);
  void NotifyWriteDone(unsigned blockId, unsigned blockAllocSize);
  unsigned Align(unsigned value);
};

} // namespace stream
} // namespace gits
