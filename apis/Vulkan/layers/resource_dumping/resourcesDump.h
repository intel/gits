// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dumpTarget.h"
#include "vulkanHeader2.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gits {
namespace vulkan {

struct VkDeviceLevelDispatchTable;

// The fence and semaphore signal operations of a queue submission after which resources are
// dumped. They are moved to the submission of the copies, so the work and the host waiting for
// them cannot change the resources before the copies read them.
struct SubmitSignals {
  enum class Function {
    QueueSubmit,
    QueueSubmit2,
    QueueSubmit2KHR
  };
  Function SubmittedWith{};
  // vkQueueSubmit: the submission chains VkTimelineSemaphoreSubmitInfo.
  bool TimelineValues{};
  VkFence Fence{};
  // The signal operations of each batch that signals semaphores, in submission order. A batch may
  // not signal a semaphore twice, but separate batches may.
  std::vector<std::vector<VkSemaphoreSubmitInfo>> Batches;
};

struct StagedResource {
  DumpTarget Target;
  VkBuffer Staging{};
  VkDeviceMemory Memory{};
  bool Coherent{};
  VkDeviceSize Size{};
  std::vector<VkBufferImageCopy> Regions;
};

// A fence signaled once the work submitted to a queue before it completes.
class ExecutionFence {
public:
  ExecutionFence(VkDevice device, VkDeviceLevelDispatchTable& dispatchTable, VkFence fence)
      : m_Device(device), m_DispatchTable(dispatchTable), m_Fence(fence) {}
  ~ExecutionFence();
  ExecutionFence(const ExecutionFence&) = delete;
  ExecutionFence& operator=(const ExecutionFence&) = delete;

  VkResult Wait(uint64_t timeout) const;
  VkFence Handle() const {
    return m_Fence;
  }

private:
  VkDevice m_Device{};
  VkDeviceLevelDispatchTable& m_DispatchTable;
  VkFence m_Fence{};
};

// The staging copies recorded at one dump point. The staging memory lives until the command
// buffer the copies were recorded into is reset, as every submission of it executes them.
class DumpBatch {
public:
  DumpBatch(VkDevice device, VkDeviceLevelDispatchTable& dispatchTable)
      : m_Device(device), m_DispatchTable(dispatchTable) {}
  ~DumpBatch();
  DumpBatch(const DumpBatch&) = delete;
  DumpBatch& operator=(const DumpBatch&) = delete;

  // Takes over writing the batch, unless it is taken over already.
  bool Claim();
  // Marks the batch as handled. Unless its execution completed, the device may still use its
  // staging memory.
  void MarkWritten(bool executed);
  void WaitWritten();
  bool IsWritten();
  // Whether the batch is written and its execution completed.
  bool IsExecuted();

  std::vector<StagedResource> Resources;
  // Set once the batch is handed over to be written after the submission that executes it.
  bool Submitted{};
  // Signaled once the latest submission that executes the batch completes.
  std::shared_ptr<ExecutionFence> Execution;

private:
  VkDevice m_Device{};
  VkDeviceLevelDispatchTable& m_DispatchTable;
  std::mutex m_Mutex;
  std::condition_variable m_WrittenCV;
  bool m_Claimed{};
  bool m_Written{};
  bool m_Executed{};
};

// Copies resources of one device to host visible staging memory and writes them to files on a
// worker thread.
class ResourcesDump {
public:
  ResourcesDump(VkDevice device,
                VkDeviceLevelDispatchTable& dispatchTable,
                const VkPhysicalDeviceMemoryProperties& memoryProperties,
                bool separateDepthStencilLayouts);
  ~ResourcesDump();
  ResourcesDump(const ResourcesDump&) = delete;
  ResourcesDump& operator=(const ResourcesDump&) = delete;

  // Records the copies of the targets into a command buffer. It records nothing if a staging
  // buffer cannot be created.
  std::shared_ptr<DumpBatch> RecordDumps(VkCommandBuffer commandBuffer,
                                         std::vector<DumpTarget> targets);
  // Writes the batches once the work submitted to the queue so far completes. The batches executed
  // again are written already, but their execution writes their staging memory, so they are kept
  // until it completes.
  void WriteAfterExecution(VkQueue queue,
                           std::vector<std::shared_ptr<DumpBatch>> batches,
                           std::vector<std::shared_ptr<DumpBatch>> executedAgain);
  // Submits the copies of the targets after the work submitted to the queue so far, followed by
  // the signal operations of that work, and writes them once they complete. The signals are
  // submitted even if nothing is copied.
  void DumpAfterSubmissions(VkQueue queue,
                            uint32_t queueFamilyIndex,
                            std::vector<DumpTarget> targets,
                            const SubmitSignals& signals);
  // Writes the batch, whose execution the application waited for, unless the worker writes it,
  // and waits until it is written. The fence of the batch may wait for later batches of its
  // submission, which may wait for later replayed calls.
  void WriteExecuted(DumpBatch& batch);
  // Once the replay ends or the device is destroyed, the work of the batches may wait for semaphore
  // values that no replayed call signals anymore. Waits until every batch handed over so far is
  // written, unless none is written for the timeout, stops the worker, and returns the number of
  // dumped resources not written. The resources the device may still use are then never released.
  size_t Finish(std::chrono::milliseconds timeout);
  // Takes the first failure of the worker since the last call, or returns an empty string.
  std::string TakeWorkerError();

private:
  struct Job {
    std::shared_ptr<ExecutionFence> Fence;
    std::vector<std::shared_ptr<DumpBatch>> Batches;
    // Batches written already and executed again before the fence, kept until it signals.
    std::vector<std::shared_ptr<DumpBatch>> ExecutedAgain;
  };

  void RecordCopies(VkCommandBuffer commandBuffer, DumpBatch& batch);
  void CreateStaging(StagedResource& resource);
  uint32_t FindStagingMemoryType(uint32_t memoryTypeBits, bool& coherent) const;
  std::shared_ptr<ExecutionFence> CreateFence();
  // Submits the fence after the work submitted to the queue so far.
  void SubmitFence(VkQueue queue, const ExecutionFence& fence, const char* after);
  VkCommandBuffer BeginCopies(uint32_t queueFamilyIndex);
  VkResult SubmitWithSignals(VkQueue queue,
                             VkCommandBuffer commandBuffer,
                             const SubmitSignals& signals);
  void FreeWrittenCopies();
  void Enqueue(Job job);
  void WorkerThread();
  // Writes the batches of the job, whose fence signaled.
  void WriteJob(Job& job);
  void WriteBatch(DumpBatch& batch);
  void WriteResource(StagedResource& resource);
  void WriteImage(const StagedResource& resource, const uint8_t* data);

  VkDevice m_Device{};
  VkDeviceLevelDispatchTable& m_DispatchTable;
  VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
  bool m_SeparateDepthStencilLayouts{};
  std::map<uint32_t, VkCommandPool> m_CommandPools;
  // Command buffers of DumpAfterSubmissions, with their pool and batch, freed once their execution
  // completes and they are written.
  struct SubmittedCopies {
    VkCommandPool CommandPool{};
    VkCommandBuffer CommandBuffer{};
    std::shared_ptr<DumpBatch> Batch;
  };
  std::vector<SubmittedCopies> m_SubmittedCopies;
  // Batches of application command buffers whose execution no fence tracks, as submitting one
  // failed.
  std::vector<std::shared_ptr<DumpBatch>> m_UntrackedBatches;

  std::thread m_Worker;
  std::mutex m_Mutex;
  std::condition_variable m_JobsCV;
  std::condition_variable m_IdleCV;
  std::deque<Job> m_Jobs;
  // The jobs enqueued and not written yet.
  size_t m_UnfinishedJobs{};
  bool m_Stop{};
  std::string m_WorkerError;
  // Jobs whose fence wait failed, so their work may still be pending. Only the worker uses them
  // until it stops.
  std::vector<Job> m_FailedJobs;
  // Jobs whose fence had not signaled when the worker stopped.
  std::vector<Job> m_AbandonedJobs;
  // The dumped resources of the abandoned jobs, not reported yet.
  std::vector<std::string> m_AbandonedDumps;
};

} // namespace vulkan
} // namespace gits
