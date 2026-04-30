// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamReader.h"
#include "streamHeader.h"
#include "log.h"

namespace gits {
namespace stream {

StreamReader::StreamReader(std::vector<CommandFactory*>& commandFactories, std::istream& stream)
    : m_CommandFactories(commandFactories), m_Stream(stream) {
  for (unsigned i = 0; i < NUMBER_OF_BLOCKS; ++i) {
    m_UncompressedBlocks[i].Data.reset(new char[INITIAL_BLOCK_ALLOC]);
    m_UncompressedBlocks[i].DataAlloc = INITIAL_BLOCK_ALLOC;
    m_UncompressedBlocks[i].Index = i;
    m_CompressedBlocks[i].Data.reset(new char[INITIAL_BLOCK_ALLOC]);
    m_CompressedBlocks[i].DataAlloc = INITIAL_BLOCK_ALLOC;
    m_CompressedBlocks[i].Index = i;
  }
  CompressionType compressionType = StreamHeader::Get().GetCompressionType();
  if (compressionType == CompressionType::ZSTD) {
    m_Decompressor.reset(new ZSTDStreamDecompressor());
  } else {
    m_Decompressor.reset(new LZ4StreamDecompressor());
  }
}

void StreamReader::Close() {
  m_Closed = true;
}

void StreamReader::ReadCompressedBlocks() {
  unsigned blockId{};
  while (m_Stream && !m_Closed) {
    uint64_t compressedSize{};
    m_Stream.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
    if (!m_Stream) {
      break;
    }
    CompressedBlock* block{};
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      block = FindBlockForRead(lock, compressedSize);
    }
    GITS_ASSERT(block);
    if (block->DataAlloc < compressedSize) {
      uint64_t size = Align(compressedSize);
      block->Data.reset(new char[size]);
      block->DataAlloc = size;
    }
    block->Id = ++blockId;
    block->DataSize = compressedSize;
    m_Stream.read(reinterpret_cast<char*>(&block->UncompressedDataSize),
                  sizeof(block->UncompressedDataSize));
    m_Stream.read(block->Data.get(), compressedSize);
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_LastRead = blockId;
      block->Full = true;
      m_DecompressionQueue.push(block->Index);
      m_ReadDoneCondition.notify_one();
    }
  }
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_ReadFinished = true;
    m_ReadDoneCondition.notify_all();
  }
}

StreamReader::CompressedBlock* StreamReader::FindBlockForRead(std::unique_lock<std::mutex>& lock,
                                                              uint64_t size) {
  unsigned blockIndex = 0;
  uint64_t maxAlloc = 0;
  int foundBlock = -1;
  while (!m_ReadFinished) {
    Block& block = m_CompressedBlocks[blockIndex];
    if (!block.Full && !block.Decompressing) {
      if (size <= block.DataAlloc) {
        return &m_CompressedBlocks[blockIndex];
      } else if (block.DataAlloc > maxAlloc) {
        maxAlloc = block.DataAlloc;
        foundBlock = blockIndex;
      }
    }

    ++blockIndex;
    if (blockIndex == NUMBER_OF_BLOCKS) {
      if (foundBlock >= 0) {
        return &m_CompressedBlocks[foundBlock];
      }
      blockIndex = 0;
      m_DecompressionDoneCondition.wait(lock);
    }
  }
  return nullptr;
}

void StreamReader::Decompress(unsigned threadIndex) {
  while (true) {
    CompressedBlock* compressedBlock{};
    UncompressedBlock* uncompressedBlock{};

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      unsigned blockIndex{};

      while (!compressedBlock) {
        if (!m_DecompressionQueue.empty()) {
          blockIndex = m_DecompressionQueue.front();
          m_DecompressionQueue.pop();
          compressedBlock = &m_CompressedBlocks[blockIndex];
        } else if (m_ReadFinished) {
          return;
        } else {
          m_ReadDoneCondition.wait(lock);
        }
      }
      GITS_ASSERT(compressedBlock);
      compressedBlock->Decompressing = true;

      uncompressedBlock = &m_UncompressedBlocks[blockIndex];
      if (uncompressedBlock->Full || uncompressedBlock->Decompressing) {
        if (!m_RunFinished) {
          WaitForRunDone(lock, threadIndex, uncompressedBlock->Index);
        } else {
          return;
        }
      }
      GITS_ASSERT(uncompressedBlock);
      uncompressedBlock->Decompressing = true;
    }

    if (compressedBlock->UncompressedDataSize > uncompressedBlock->DataAlloc) {
      uint64_t size = Align(compressedBlock->UncompressedDataSize);
      uncompressedBlock->Data.reset(new char[size]);
      uncompressedBlock->DataAlloc = size;
    }

    int size =
        m_Decompressor->Decompress(compressedBlock->Data.get(), uncompressedBlock->Data.get(),
                                   compressedBlock->DataSize, uncompressedBlock->DataAlloc);
    if (size != static_cast<int>(compressedBlock->UncompressedDataSize)) {
      LOG_ERROR << "Decompressed " << size << " instead of "
                << compressedBlock->UncompressedDataSize << " for compressed "
                << compressedBlock->DataSize;
      std::quick_exit(EXIT_FAILURE);
    }

    uncompressedBlock->Id = compressedBlock->Id;
    uncompressedBlock->DataSize = compressedBlock->UncompressedDataSize;
    compressedBlock->DataSize = 0;
    compressedBlock->UncompressedDataSize = 0;

    DecodeBlock(*uncompressedBlock);

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      compressedBlock->Full = false;
      uncompressedBlock->Full = true;
      uncompressedBlock->Decompressing = false;
      compressedBlock->Decompressing = false;
      m_DecompressionDoneCondition.notify_all();
    }
  }
}

void StreamReader::DecodeBlock(UncompressedBlock& block) {
  uint64_t offset = 0;
  while (offset < block.DataSize) {
    unsigned id = *reinterpret_cast<unsigned*>(block.Data.get() + offset);
    offset += sizeof(id);
    uint64_t size = *reinterpret_cast<uint64_t*>(block.Data.get() + offset);
    offset += sizeof(size);
    for (CommandFactory* commandFactory : m_CommandFactories) {
      CommandRunner* runner = commandFactory->CreateCommand(id);
      if (runner) {
        runner->DecodeData(block.Data.get() + offset);
        block.Runners.emplace_back(runner);
      }
    }
    offset += size;
  }
}

void StreamReader::Run() {
  for (unsigned i = 0; i < NUMBER_OF_DECOMPRESSION_THREADS; ++i) {
    m_DecompressionThreads[i] = std::thread{&StreamReader::Decompress, this, i};
  }
  m_ReadingThread = std::thread{&StreamReader::ReadCompressedBlocks, this};

  unsigned blockId{};
  while (!m_Closed) {
    ++blockId;
    UncompressedBlock* block{};
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      if (m_ReadFinished && blockId > m_LastRead) {
        break;
      }
      block = FindBlockForRun(lock, blockId);
    }
    GITS_ASSERT(block);

    for (auto& runner : block->Runners) {
      runner->Run();
      if (m_Closed) {
        break;
      }
    }
    block->Runners.clear();

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      block->Id = 0;
      block->DataSize = 0;
      block->Full = false;
      NotifyRunDone(block->Index);
    }
  }

  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_RunFinished = true;
    NotifyRunDoneAll();
  }

  for (unsigned i = 0; i < NUMBER_OF_DECOMPRESSION_THREADS; ++i) {
    if (m_DecompressionThreads[i].joinable()) {
      m_DecompressionThreads[i].join();
    }
  }
  if (m_ReadingThread.joinable()) {
    m_ReadingThread.join();
  }
}

StreamReader::UncompressedBlock* StreamReader::FindBlockForRun(std::unique_lock<std::mutex>& lock,
                                                               unsigned blockId) {
  unsigned blockIndex = 0;
  while (!m_RunFinished) {
    UncompressedBlock& block = m_UncompressedBlocks[blockIndex];
    if (block.Full && block.Id == blockId) {
      return &m_UncompressedBlocks[blockIndex];
    }

    ++blockIndex;
    if (blockIndex == NUMBER_OF_BLOCKS) {
      blockIndex = 0;
      m_DecompressionDoneCondition.wait(lock);
    }
  }
  return nullptr;
}

void StreamReader::WaitForRunDone(std::unique_lock<std::mutex>& lock,
                                  unsigned threadIndex,
                                  unsigned blockIndex) {
  m_WaitsForRunDone[threadIndex].Waiting = true;
  m_WaitsForRunDone[threadIndex].BlockIndex = blockIndex;
  m_WaitsForRunDone[threadIndex].Condition.wait(lock);
}

void StreamReader::NotifyRunDone(unsigned blockIndex) {
  for (unsigned i = 0; i < NUMBER_OF_DECOMPRESSION_THREADS; ++i) {
    if (m_WaitsForRunDone[i].Waiting && m_WaitsForRunDone[i].BlockIndex == blockIndex) {
      m_WaitsForRunDone[i].Waiting = false;
      m_WaitsForRunDone[i].Condition.notify_one();
      break;
    }
  }
}

void StreamReader::NotifyRunDoneAll() {
  for (WaitForRunDoneInfo& wait : m_WaitsForRunDone) {
    wait.Waiting = false;
    wait.Condition.notify_one();
  }
}

uint64_t StreamReader::Align(uint64_t value) {
  return ((value - 1) / 4096 + 1) * 4096;
}

} // namespace stream
} // namespace gits
