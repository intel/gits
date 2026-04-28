// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamWriter.h"
#include "streamHeader.h"
#include "log.h"
#include "configurator.h"

namespace gits {
namespace stream {

StreamWriter::StreamWriter(const std::filesystem::path& streamDir,
                           CompressionType compressionType) {
  m_StreamDir = streamDir.string();
  std::filesystem::create_directories(streamDir);
  m_Stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  m_Stream.open(streamDir / "stream.gits2", std::ios::out | std::ios::binary);

  for (unsigned i = 0; i < NUMBER_OF_COMPRESSION_THREADS; ++i) {
    m_CompressionThreads[i] = std::thread{&StreamWriter::Compress, this, i};
    if (compressionType == CompressionType::ZSTD) {
      m_Compressors[i].reset(new ZSTDStreamCompressor());
    } else {
      m_Compressors[i].reset(new LZ4StreamCompressor());
    }
  }

  StreamHeader::Get().WriteHeader(m_Stream, compressionType);

  for (unsigned i = 0; i < NUMBER_OF_BLOCKS; ++i) {
    m_UncompressedBlocks[i].Data.reset(new char[INITIAL_BLOCK_ALLOC]);
    m_UncompressedBlocks[i].DataAlloc = INITIAL_BLOCK_ALLOC;
    m_UncompressedBlocks[i].Index = i;
    uint64_t compressedSize = m_Compressors[0]->CompressBound(INITIAL_BLOCK_ALLOC);
    m_CompressedBlocks[i].Data.reset(new char[Align(compressedSize)]);
    m_CompressedBlocks[i].DataAlloc = compressedSize;
    m_CompressedBlocks[i].Index = i;
  }

  m_WritingThread = std::thread{&StreamWriter::WriteCompressedBlocks, this};
}

StreamWriter::~StreamWriter() {
  Close();
}

void StreamWriter::Close() {
  {
    if (m_StopThreads) {
      return;
    }

    std::unique_lock<std::mutex> lock(m_Mutex);
    m_StopThreads = true;
    Block* block = FindBlockForRecord(lock, 0);
    if (block->DataSize) {
      ++m_RecordedBlockId;
      block->Id = m_RecordedBlockId;
      block->Full = true;
      m_CompressionQueue.push(block->Index);
      m_RecordDoneCondition.notify_one();
    }
  }

  m_CompressionDoneCondition.notify_all();
  if (m_WritingThread.joinable()) {
    m_WritingThread.join();
  }
  m_RecordDoneCondition.notify_all();
  for (unsigned i = 0; i < NUMBER_OF_COMPRESSION_THREADS; ++i) {
    if (m_CompressionThreads[i].joinable()) {
      m_CompressionThreads[i].join();
    }
  }
  m_Stream.close();
}

void StreamWriter::Record(const CommandSerializer& commandSerializer) {
  if (m_StopThreads) {
    return;
  }

  unsigned id = commandSerializer.Id();
  uint64_t size = commandSerializer.Size();
  uint64_t totalSize = sizeof(id) + sizeof(size) + size;

  Block* block{};
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (!m_ApiWritten) {
      ApiId apiId = ExtractApiIdentifier(id);
      if (apiId != ApiId::ID_COMMON) {
        StreamHeader::Get().WriteApi(m_Stream, apiId);
        m_ApiWritten = true;
      }
    }
    block = FindBlockForRecord(lock, totalSize);
  }
  if (block->DataAlloc - block->DataSize < totalSize) {
    std::unique_ptr<char[]> tempData;
    tempData.swap(block->Data);
    uint64_t size = Align(totalSize + block->DataSize);
    block->Data.reset(new char[size]);
    block->DataAlloc = size;
    memcpy(block->Data.get(), tempData.get(), block->DataSize);
  }

  memcpy(block->Data.get() + block->DataSize, &id, sizeof(id));
  block->DataSize += sizeof(id);
  memcpy(block->Data.get() + block->DataSize, &size, sizeof(size));
  block->DataSize += sizeof(size);
  if (size) {
    memcpy(block->Data.get() + block->DataSize, commandSerializer.Data(), size);
    block->DataSize += size;
  }

  if (block->DataSize > TRIGGER_BLOCK_SIZE) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    ++m_RecordedBlockId;
    block->Id = m_RecordedBlockId;
    block->Full = true;
    m_CompressionQueue.push(block->Index);
    m_RecordDoneCondition.notify_one();
  }
}

StreamWriter::Block* StreamWriter::FindBlockForRecord(std::unique_lock<std::mutex>& lock,
                                                      uint64_t size) {
  if (!m_UncompressedBlocks[m_CurrentRecordBlock].Full) {
    return &m_UncompressedBlocks[m_CurrentRecordBlock];
  }

  unsigned blockIndex = 0;
  unsigned maxAlloc = 0;
  int foundBlock = -1;
  while (true) {
    Block& block = m_UncompressedBlocks[blockIndex];
    if (!block.Full && !block.Compressing) {
      if (size <= block.DataAlloc) {
        m_CurrentRecordBlock = blockIndex;
        return &m_UncompressedBlocks[m_CurrentRecordBlock];
      } else if (block.DataAlloc > maxAlloc) {
        maxAlloc = block.DataAlloc;
        foundBlock = blockIndex;
      }
    }

    ++blockIndex;
    if (blockIndex == NUMBER_OF_BLOCKS) {
      if (foundBlock >= 0) {
        m_CurrentRecordBlock = foundBlock;
        return &m_UncompressedBlocks[m_CurrentRecordBlock];
      }
      blockIndex = 0;
      m_CompressionDoneCondition.wait(lock);
    }
  }
}

void StreamWriter::Compress(unsigned threadIndex) {
  while (true) {
    Block* uncompressedBlock{};
    CompressedBlock* compressedBlock{};
    uint64_t compressedSize = 0;

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      unsigned blockIndex{};

      while (!uncompressedBlock) {
        if (!m_CompressionQueue.empty()) {
          blockIndex = m_CompressionQueue.front();
          m_CompressionQueue.pop();
          uncompressedBlock = &m_UncompressedBlocks[blockIndex];
        } else if (m_StopThreads && m_WrittenBlockId == m_RecordedBlockId) {
          m_CompressionDoneCondition.notify_all();
          return;
        } else {
          m_RecordDoneCondition.wait(lock);
        }
      }
      GITS_ASSERT(uncompressedBlock);
      uncompressedBlock->Compressing = true;

      compressedSize = m_Compressors[threadIndex]->CompressBound(uncompressedBlock->DataSize);

      compressedBlock = &m_CompressedBlocks[blockIndex];
      if (compressedBlock->Full || compressedBlock->Compressing) {
        if (m_StopThreads && m_WrittenBlockId == m_RecordedBlockId) {
          uncompressedBlock->Compressing = false;
          m_CompressionDoneCondition.notify_all();
          return;
        } else {
          WaitForWriteDone(lock, threadIndex, compressedBlock->Index);
        }
      }
      GITS_ASSERT(compressedBlock);
      compressedBlock->Compressing = true;

      if (m_StopThreads && m_WrittenBlockId == m_RecordedBlockId) {
        m_CompressionDoneCondition.notify_all();
        return;
      }
    }

    if (compressedSize > compressedBlock->DataAlloc) {
      uint64_t alignedSize = Align(compressedSize);
      compressedBlock->Data.reset(new char[alignedSize]);
      compressedBlock->DataAlloc = alignedSize;
    }

    compressedBlock->DataSize = m_Compressors[threadIndex]->Compress(
        uncompressedBlock->Data.get(), compressedBlock->Data.get(), uncompressedBlock->DataSize,
        compressedBlock->DataAlloc);
    if (!compressedBlock->DataSize) {
      LOG_ERROR << "StreamWriter - cannot compress " << uncompressedBlock->DataSize
                << " bytes of data.";
      std::quick_exit(EXIT_FAILURE);
    }

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      compressedBlock->Id = uncompressedBlock->Id;
      compressedBlock->UncompressedDataSize = uncompressedBlock->DataSize;
      uncompressedBlock->DataSize = 0;
      uncompressedBlock->Full = false;
      compressedBlock->Full = true;
      uncompressedBlock->Compressing = false;
      compressedBlock->Compressing = false;
    }
    m_CompressionDoneCondition.notify_all();
  }
}

void StreamWriter::WriteCompressedBlocks() {
  unsigned blockIndex = 0;
  unsigned readyBlocks = 0;
  while (true) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    CompressedBlock& block = m_CompressedBlocks[blockIndex];
    if (block.Full) {
      if (block.Id == m_WrittenBlockId + 1) {
        lock.unlock();
        try {
          m_Stream.write(reinterpret_cast<char*>(&block.DataSize), sizeof(block.DataSize));
          m_Stream.write(reinterpret_cast<char*>(&block.UncompressedDataSize),
                         sizeof(block.UncompressedDataSize));
          m_Stream.write(block.Data.get(), block.DataSize);
        } catch (std::exception& e) {
          LOG_ERROR << "Stream writting failure - " << e.what();
          std::quick_exit(EXIT_FAILURE);
        }
        block.DataSize = 0;
        block.UncompressedDataSize = 0;

        lock.lock();

        block.Full = false;
        ++m_WrittenBlockId;
        NotifyWriteDone(block.Index);
      }
      ++readyBlocks;
    }

    if (m_StopThreads && m_WrittenBlockId == m_RecordedBlockId) {
      break;
    }

    ++blockIndex;
    if (blockIndex == NUMBER_OF_BLOCKS) {
      blockIndex = 0;
      if (!readyBlocks) {
        m_CompressionDoneCondition.wait(lock);
      }
      readyBlocks = 0;
    }
  }
}

void StreamWriter::WaitForWriteDone(std::unique_lock<std::mutex>& lock,
                                    unsigned threadIndex,
                                    unsigned blockIndex) {
  m_WaitsForWriteDone[threadIndex].Waiting = true;
  m_WaitsForWriteDone[threadIndex].BlockIndex = blockIndex;
  m_WaitsForWriteDone[threadIndex].Condition.wait(lock);
}

void StreamWriter::NotifyWriteDone(unsigned blockIndex) {
  for (unsigned i = 0; i < NUMBER_OF_COMPRESSION_THREADS; ++i) {
    if (m_WaitsForWriteDone[i].Waiting && m_WaitsForWriteDone[i].BlockIndex == blockIndex) {
      m_WaitsForWriteDone[i].Waiting = false;
      m_WaitsForWriteDone[i].Condition.notify_one();
      break;
    }
  }
}

uint64_t StreamWriter::Align(uint64_t value) {
  return ((value - 1) / 4096 + 1) * 4096;
}

} // namespace stream
} // namespace gits
