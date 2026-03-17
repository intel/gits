// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamLegacyReader.h"
#include "commandId.h"
#include "log.h"

namespace gits {
namespace stream {

static bool g_Log = false;

StreamLegacyReader::StreamLegacyReader(std::vector<CommandFactory*>& commandFactories,
                                       std::istream& stream)
    : m_CommandFactories(commandFactories), m_Stream(stream) {
  for (unsigned i = 0; i < NUMBER_OF_BLOCKS; ++i) {
    m_UncompressedBlocks[i].Data.reset(new char[INITIAL_BLOCK_ALLOC]);
    m_UncompressedBlocks[i].DataAlloc = INITIAL_BLOCK_ALLOC;
    m_UncompressedBlocks[i].SpanningData.reset(new char[INITIAL_CROSSING_DATA_ALLOC]);
    m_UncompressedBlocks[i].SpanningDataAlloc = INITIAL_CROSSING_DATA_ALLOC;
    m_UncompressedBlocks[i].Index = i;
    m_CompressedBlocks[i].Data.reset(new char[INITIAL_BLOCK_ALLOC]);
    m_CompressedBlocks[i].DataAlloc = INITIAL_BLOCK_ALLOC;
    m_CompressedBlocks[i].Index = i;
  }
  m_Decompressor.reset(new LZ4StreamDecompressor());
}

void StreamLegacyReader::Close() {
  m_Closed = true;
}

void StreamLegacyReader::ReadCompressedBlocks() {
  unsigned blockId{};
  while (m_Stream && !m_Closed) {
    uint8_t writeType{};
    uint64_t compressedSize{};
    uint64_t uncompressedSize{};
    m_Stream.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));
    if (!m_Stream) {
      break;
    }
    m_Stream.read(reinterpret_cast<char*>(&writeType), sizeof(writeType));
    if (writeType == 0 || writeType == 1) {
      m_Stream.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
      ReadCompressedBlock(++blockId, compressedSize, uncompressedSize);
      if (g_Log) {
        LOG_INFO << "READ " << blockId << " " << static_cast<int>(writeType) << " "
                 << uncompressedSize;
      }
    } else {
      uint64_t chunksNumber{};
      m_Stream.read(reinterpret_cast<char*>(&chunksNumber), sizeof(chunksNumber));
      for (unsigned i = 0; i < chunksNumber; ++i) {
        m_Stream.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));
        m_Stream.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
        ReadCompressedBlock(++blockId, compressedSize, uncompressedSize);
        if (g_Log) {
          LOG_INFO << "READ " << blockId << " " << static_cast<int>(writeType) << " "
                   << uncompressedSize;
        }
      }
    }
  }
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_ReadFinished = true;
    m_ReadDoneCondition.notify_all();
  }
}

void StreamLegacyReader::ReadCompressedBlock(unsigned blockId,
                                             uint64_t compressedSize,
                                             uint64_t uncompressedSize) {
  CompressedBlock* block{};
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    block = FindBlockForRead(lock, compressedSize);
  }
  GITS_ASSERT(block);
  if (block->DataAlloc < compressedSize) {
    unsigned size = Align(compressedSize);
    block->Data.reset(new char[size]);
    block->DataAlloc = size;
  }
  block->Id = blockId;
  block->DataSize = compressedSize;
  block->UncompressedDataSize = uncompressedSize;
  m_Stream.read(block->Data.get(), compressedSize);
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_LastRead = blockId;
    block->DataFull = true;
    m_ReadDoneCondition.notify_one();
  }
}

StreamLegacyReader::CompressedBlock* StreamLegacyReader::FindBlockForRead(
    std::unique_lock<std::mutex>& lock, unsigned size) {
  unsigned blockIndex = 0;
  unsigned maxAlloc = 0;
  int foundBlock = -1;
  while (!m_ReadFinished) {
    Block& block = m_CompressedBlocks[blockIndex];
    if (!block.DataFull) {
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

void StreamLegacyReader::Decompress() {
  while (true) {
    CompressedBlock* compressedBlock{};
    UncompressedBlock* uncompressedBlock{};

    {
      std::unique_lock<std::mutex> lock(m_Mutex);

      unsigned blockIndex = 0;
      while (true) {
        CompressedBlock& block = m_CompressedBlocks[blockIndex];
        if (block.DataFull) {
          if (block.Id == m_LastDecompressed + 1) {
            compressedBlock = &block;
          }
        }
        ++blockIndex;
        if (blockIndex == NUMBER_OF_BLOCKS) {
          blockIndex = 0;
          if (!compressedBlock) {
            if (!m_ReadFinished) {
              m_ReadDoneCondition.wait(lock);
            } else {
              return;
            }
          } else {
            break;
          }
        }
      }
      GITS_ASSERT(compressedBlock);

      blockIndex = 0;
      unsigned maxAlloc = 0;
      while (!uncompressedBlock) {
        UncompressedBlock& block = m_UncompressedBlocks[blockIndex];
        if (!block.DataFull && !block.RunnersFull) {
          if (compressedBlock->UncompressedDataSize <= block.DataAlloc) {
            uncompressedBlock = &block;
            break;
          } else if (block.DataAlloc + block.SpanningDataAlloc > maxAlloc) {
            maxAlloc = block.DataAlloc + block.SpanningDataAlloc;
            uncompressedBlock = &block;
          }
        }
        ++blockIndex;
        if (blockIndex == NUMBER_OF_BLOCKS) {
          if (uncompressedBlock) {
            break;
          }
          blockIndex = 0;
          if (!m_RunFinished) {
            m_RunDoneCondition.wait(lock);
          } else {
            return;
          }
        }
      }
      GITS_ASSERT(uncompressedBlock);
    }

    if (g_Log) {
      LOG_INFO << "DECOMPRESSED START " << compressedBlock->Id;
    }

    if (compressedBlock->UncompressedDataSize > uncompressedBlock->DataAlloc) {
      unsigned size = Align(compressedBlock->UncompressedDataSize);
      uncompressedBlock->Data.reset(new char[size]);
      uncompressedBlock->DataAlloc = size;
    }

    int size =
        m_Decompressor->Decompress(compressedBlock->Data.get(), uncompressedBlock->Data.get(),
                                   compressedBlock->DataSize, uncompressedBlock->DataAlloc);
    if (size != static_cast<int>(compressedBlock->UncompressedDataSize)) {
      LOG_INFO << "Decompressed " << size << " instead of " << compressedBlock->UncompressedDataSize
               << " for compressed " << compressedBlock->DataSize;
      std::quick_exit(EXIT_FAILURE);
    }

    uncompressedBlock->Id = compressedBlock->Id;
    uncompressedBlock->DataSize = compressedBlock->UncompressedDataSize;
    compressedBlock->DataSize = 0;
    compressedBlock->UncompressedDataSize = 0;
    compressedBlock->DataFull = false;
    uncompressedBlock->DataFull = true;

    std::vector<UncompressedBlock*> previousBlocks;
    if (uncompressedBlock->Id > 1) {
      for (unsigned i = 0; i < NUMBER_OF_BLOCKS; ++i) {
        if (m_UncompressedBlocks[i].DataFull &&
            m_UncompressedBlocks[i].Id < uncompressedBlock->Id) {
          previousBlocks.push_back(&m_UncompressedBlocks[i]);
        }
      }
      GITS_ASSERT(previousBlocks.size());
      std::sort(previousBlocks.begin(), previousBlocks.end(),
                [](Block* lhs, Block* rhs) { return lhs->Id < rhs->Id; });
    }
    DecodeBlock(*uncompressedBlock, previousBlocks);
    ++m_LastDecompressed;
    if (g_Log) {
      LOG_INFO << "DECOMPRESSED END " << uncompressedBlock->Id << " "
               << uncompressedBlock->Spanning;
    }
    m_DecompressionDoneCondition.notify_all();
  }
}

void StreamLegacyReader::DecodeBlock(UncompressedBlock& block,
                                     std::vector<UncompressedBlock*>& previousBlocks) {
  unsigned spanningSize{};
  for (UncompressedBlock* previousBlock : previousBlocks) {
    spanningSize += previousBlock->Spanning;
  }
  if (spanningSize) {
    if (spanningSize > block.SpanningDataAlloc) {
      unsigned size = Align(spanningSize);
      block.SpanningData.reset(new char[size]);
      block.SpanningDataAlloc = size;
    }
    unsigned offset = 0;
    for (UncompressedBlock* previousBlock : previousBlocks) {
      memcpy(block.SpanningData.get() + offset,
             previousBlock->Data.get() + previousBlock->DataSize - previousBlock->Spanning,
             previousBlock->Spanning);
      offset += previousBlock->Spanning;
    }
  }

  unsigned offset = 0;
  if (spanningSize) {
    unsigned spanningOffset = 0;
    const unsigned COMMMAND_ID_SIZE = 3;
    if (spanningSize < COMMMAND_ID_SIZE) {
      memcpy(block.SpanningData.get() + spanningSize, block.Data.get(),
             COMMMAND_ID_SIZE - spanningSize);
      offset += COMMMAND_ID_SIZE - spanningSize;
      spanningSize = COMMMAND_ID_SIZE;
    }
    uint8_t apiId = *reinterpret_cast<uint8_t*>(block.SpanningData.get());
    spanningOffset += sizeof(apiId);
    uint16_t commandId = *reinterpret_cast<uint16_t*>(block.SpanningData.get() + spanningOffset);
    spanningOffset += sizeof(commandId);

    unsigned id = apiId * 0x10000 + commandId;

    unsigned commonCommandSize = 0;
    if (apiId == static_cast<uint8_t>(ApiId::ID_COMMON)) {
      if (id == static_cast<unsigned>(CommonCommandId::ID_MARKER_UINT64)) {
        commonCommandSize = sizeof(uint64_t);
        if (spanningSize < spanningOffset + commonCommandSize) {
          memcpy(block.SpanningData.get() + spanningSize, block.Data.get() + offset,
                 spanningOffset + commonCommandSize - spanningSize);
          offset += spanningOffset + commonCommandSize - spanningSize;
          spanningSize = spanningOffset + commonCommandSize;
        }
      }
      for (CommandFactory* commandFactory : m_CommandFactories) {
        CommandRunner* runner = commandFactory->CreateCommand(id);
        if (runner) {
          runner->DecodeData(block.SpanningData.get() + spanningOffset);
          block.Runners.emplace_back(runner);
        }
      }
      spanningOffset += sizeof(commonCommandSize);
    } else if (apiId == static_cast<uint8_t>(ApiId::ID_DIRECTX)) {
      const unsigned COMMAND_SIZE_SIZE = sizeof(unsigned);
      if (spanningSize < spanningOffset + COMMAND_SIZE_SIZE) {
        memcpy(block.SpanningData.get() + spanningSize, block.Data.get() + offset,
               spanningOffset + COMMAND_SIZE_SIZE - spanningSize);
        offset += spanningOffset + COMMAND_SIZE_SIZE - spanningSize;
        spanningSize = spanningOffset + COMMAND_SIZE_SIZE;
      }
      unsigned commandSize =
          *reinterpret_cast<unsigned*>(block.SpanningData.get() + spanningOffset);
      spanningOffset += sizeof(commandSize);

      unsigned leftSpanningCommandSize =
          spanningSize - COMMMAND_ID_SIZE - COMMAND_SIZE_SIZE - commonCommandSize;

      if (leftSpanningCommandSize) {
        unsigned rightSpanningCommandSize = commandSize - leftSpanningCommandSize;
        if (offset + rightSpanningCommandSize > block.DataSize) {
          block.Spanning = block.DataSize;
          block.RunnersFull = true;
          return;
        }
        if (spanningSize + rightSpanningCommandSize > block.SpanningDataAlloc) {
          std::unique_ptr<char[]> tempData;
          tempData.swap(block.SpanningData);
          unsigned size = Align(spanningSize + rightSpanningCommandSize);
          block.SpanningData.reset(new char[size]);
          block.SpanningDataAlloc = size;
          memcpy(block.SpanningData.get(), tempData.get(), spanningSize);
        }
        memcpy(block.SpanningData.get() + spanningOffset + leftSpanningCommandSize,
               block.Data.get() + offset, rightSpanningCommandSize);
        offset += rightSpanningCommandSize;
        for (CommandFactory* commandFactory : m_CommandFactories) {
          CommandRunner* runner = commandFactory->CreateCommand(id);
          if (runner) {
            runner->DecodeData(block.SpanningData.get() + spanningOffset);
            block.Runners.emplace_back(runner);
          }
        }
      } else {
        if (offset + commandSize > block.DataSize) {
          block.Spanning = block.DataSize;
          block.RunnersFull = true;
          return;
        }
        for (CommandFactory* commandFactory : m_CommandFactories) {
          CommandRunner* runner = commandFactory->CreateCommand(id);
          if (runner) {
            runner->DecodeData(block.Data.get() + offset);
            block.Runners.emplace_back(runner);
          }
        }
        offset += commandSize;
      }
    } else {
      // detecting api only
      for (CommandFactory* commandFactory : m_CommandFactories) {
        commandFactory->CreateCommand(id);
      }
      Close();
    }
  }

  for (UncompressedBlock* previousBlock : previousBlocks) {
    previousBlock->Spanning = 0;
    previousBlock->DataSize = 0;
    previousBlock->DataFull = false;
  }

  while (offset < block.DataSize) {
    uint8_t apiId{};
    unsigned id{};
    if (block.DataSize - offset >= sizeof(uint8_t) + sizeof(uint16_t)) {
      apiId = *reinterpret_cast<uint8_t*>(block.Data.get() + offset);
      offset += sizeof(apiId);
      uint16_t commandId = *reinterpret_cast<uint16_t*>(block.Data.get() + offset);
      offset += sizeof(commandId);
      id = apiId * 0x10000 + commandId;
    } else {
      block.Spanning = block.DataSize - offset;
      break;
    }

    if (apiId == static_cast<uint8_t>(ApiId::ID_COMMON)) {
      unsigned size{};
      if (id == static_cast<unsigned>(CommonCommandId::ID_MARKER_UINT64)) {
        size = sizeof(uint64_t);
      }
      for (CommandFactory* commandFactory : m_CommandFactories) {
        CommandRunner* runner = commandFactory->CreateCommand(id);
        if (runner) {
          runner->DecodeData(block.Data.get() + offset);
          block.Runners.emplace_back(runner);
        }
      }
      offset += size;
    } else if (apiId == static_cast<uint8_t>(ApiId::ID_DIRECTX)) {
      unsigned size{};
      if (block.DataSize - offset >= sizeof(unsigned)) {
        size = *reinterpret_cast<unsigned*>(block.Data.get() + offset);
        offset += sizeof(size);
      } else {
        block.Spanning = block.DataSize - offset + sizeof(uint8_t) + sizeof(uint16_t);
        break;
      }
      if (block.DataSize - offset >= size) {
        for (CommandFactory* commandFactory : m_CommandFactories) {
          CommandRunner* runner = commandFactory->CreateCommand(id);
          if (runner) {
            runner->DecodeData(block.Data.get() + offset);
            block.Runners.emplace_back(runner);
          }
        }
        offset += size;
      } else {
        block.Spanning =
            block.DataSize - offset + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(unsigned);
        break;
      }
    } else {
      // detecting api only
      for (CommandFactory* commandFactory : m_CommandFactories) {
        commandFactory->CreateCommand(id);
      }
      Close();
    }
  }

  block.RunnersFull = true;
}

void StreamLegacyReader::Run() {
  m_ReadingThread = std::thread{&StreamLegacyReader::ReadCompressedBlocks, this};
  m_DecompressionThread = std::thread{&StreamLegacyReader::Decompress, this};

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

    if (g_Log) {
      LOG_INFO << "RUN START " << block->Id;
    }
    for (auto& runner : block->Runners) {
      runner->Run();
      if (m_Closed) {
        break;
      }
    }
    if (g_Log) {
      LOG_INFO << "RUN END " << block->Id;
    }
    block->Runners.clear();

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      block->RunnersFull = false;
      m_RunDoneCondition.notify_all();
    }
  }

  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_RunFinished = true;
    m_RunDoneCondition.notify_all();
  }

  if (m_ReadingThread.joinable()) {
    m_ReadingThread.join();
  }
  if (m_DecompressionThread.joinable()) {
    m_DecompressionThread.join();
  }
}

StreamLegacyReader::UncompressedBlock* StreamLegacyReader::FindBlockForRun(
    std::unique_lock<std::mutex>& lock, unsigned blockId) {
  unsigned blockIndex = 0;
  while (!m_RunFinished) {
    UncompressedBlock& block = m_UncompressedBlocks[blockIndex];
    if (block.RunnersFull && block.Id == blockId) {
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

unsigned StreamLegacyReader::Align(unsigned value) {
  return ((value - 1) / 4096 + 1) * 4096;
}

} // namespace stream
} // namespace gits
