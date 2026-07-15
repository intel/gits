// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "restoreContentService.h"
#include "playerManager.h"
#include "handleMapService.h"
#include "log.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace gits {
namespace vulkan {

namespace {

// Per-slot staging cap.  Larger slots mean fewer batches and therefore fewer
// submit/barrier/fence round-trips; the cap is only an upper bound - the
// live-memory budget below shrinks the slot (and the ring) on constrained
// platforms, so this stays safe while being fast when there is headroom.
constexpr VkDeviceSize kMaxStagingBytes = 128ULL * 1024 * 1024;
// Never claim more than this fraction of the *available* host-visible memory
// (live free bytes when VK_EXT_memory_budget is present, otherwise the total
// heap size) for all staging slots combined.
constexpr VkDeviceSize kHostHeapDivisor = 4;
// Alignment for each resource's start inside the staging buffer.  512 is a
// multiple of every texel-block size we copy (and of the 4-byte minimum for
// depth/stencil), so vkCmdCopyBufferToImage bufferOffset stays valid.
constexpr VkDeviceSize kResourceAlign = 512;
// Allocation granularity for the staging buffer itself.
constexpr VkDeviceSize kStagingAlign = 64ULL * 1024;
// Max pipeline depth: staging slots cycled so GPU upload of one batch overlaps
// host fill of the next.  The combined footprint (ring * StagingSize) is capped
// to the memory budget below, so the ring shrinks toward a single slot when a
// slot is a large fraction of available memory.
constexpr size_t kRingDepth = 3;

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

// Query live free bytes on a specific memory heap via VK_EXT_memory_budget.  At
// state-restore time the real restored resources already occupy memory, so the
// total heap size over-estimates what we can actually allocate for staging.  We
// budget against the heap the staging buffer will actually be allocated from
// (not the largest host-visible heap, which may be a different, unrelated pool).
// Returns 0 when the extension or the properties2 entry point is unavailable, so
// the caller can fall back to the reported heap size.
VkDeviceSize QueryHeapFreeBytes(VkInstanceLevelDispatchTable& instDt,
                                VkPhysicalDevice physDev,
                                uint32_t heapIndex) {
  auto getProps2 = instDt.vkGetPhysicalDeviceMemoryProperties2
                       ? instDt.vkGetPhysicalDeviceMemoryProperties2
                       : instDt.vkGetPhysicalDeviceMemoryProperties2KHR;
  if (getProps2 == nullptr || instDt.vkEnumerateDeviceExtensionProperties == nullptr) {
    return 0;
  }

  // The budget struct is only populated when VK_EXT_memory_budget is advertised.
  uint32_t count = 0;
  if (instDt.vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, nullptr) !=
          VK_SUCCESS ||
      count == 0) {
    return 0;
  }
  std::vector<VkExtensionProperties> exts(count);
  if (instDt.vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, exts.data()) !=
      VK_SUCCESS) {
    return 0;
  }
  bool haveBudget = false;
  for (const auto& e : exts) {
    if (std::strcmp(e.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
      haveBudget = true;
      break;
    }
  }
  if (!haveBudget) {
    return 0;
  }

  VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{};
  budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
  VkPhysicalDeviceMemoryProperties2 props2{};
  props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
  props2.pNext = &budget;
  getProps2(physDev, &props2);

  const VkDeviceSize used = budget.heapUsage[heapIndex];
  const VkDeviceSize cap = budget.heapBudget[heapIndex];
  return cap > used ? cap - used : VkDeviceSize{0};
}

} // namespace

RestoreContentService::~RestoreContentService() {
  for (auto& [key, session] : m_Sessions) {
    if (session) {
      DestroySession(*session);
    }
  }
  m_Sessions.clear();
}

void RestoreContentService::Manifest(const RestoreContentManifestCommand& command) {
  auto session = std::make_unique<Session>();
  session->DeviceKey = command.m_DeviceKey;

  auto& hms = HandleMapService::Get();
  session->Device = reinterpret_cast<VkDevice>(hms.TryGetHandle(command.m_DeviceKey));
  session->PhysDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(command.m_PhysDevKey));
  session->Queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(command.m_QueueKey));
  session->Pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(command.m_CommandPoolKey));

  if (!session->Device || !session->PhysDevice || !session->Queue || !session->Pool) {
    LOG_WARNING << "RestoreContentService: manifest: could not resolve device/physDev/queue/pool "
                   "for device key="
                << command.m_DeviceKey << " - skipping content restore.";
    m_Sessions[command.m_DeviceKey] = std::move(session);
    return;
  }

  session->Dt = &m_Player.GetDeviceDispatchTable(session->Device);
  auto& dt = *session->Dt;

  // Flatten the manifest into the dense resource-index order the recorder used
  // (buffers first, then images).
  VkDeviceSize maxResource = 0;
  for (const auto& buf : command.m_Buffers) {
    ResourceDesc desc;
    desc.IsImage = false;
    desc.DstKey = buf.DstBufferKey;
    desc.Size = buf.Size;
    session->Resources.push_back(std::move(desc));
    maxResource = std::max(maxResource, buf.Size);
  }
  for (const auto& img : command.m_Images) {
    ResourceDesc desc;
    desc.IsImage = true;
    desc.DstKey = img.DstImageKey;
    desc.Size = img.Size;
    desc.Format = static_cast<VkFormat>(img.Format);
    desc.FinalLayout = static_cast<VkImageLayout>(img.FinalLayout);
    desc.AspectMask = static_cast<VkImageAspectFlags>(img.AspectMask);
    desc.Regions = img.Regions;
    session->Resources.push_back(std::move(desc));
    maxResource = std::max(maxResource, img.Size);
  }

  if (session->Resources.empty() || maxResource == 0) {
    m_Sessions[command.m_DeviceKey] = std::move(session);
    return;
  }

  // ---- Size the staging buffer from live device memory ----
  VkInstanceLevelDispatchTable& instDt = m_Player.GetInstanceDispatchTable(session->PhysDevice);

  VkPhysicalDeviceMemoryProperties memProps{};
  instDt.vkGetPhysicalDeviceMemoryProperties(session->PhysDevice, &memProps);

  // Pick the staging memory type up front so we can budget against the heap we
  // will actually allocate from.  A plain TRANSFER_SRC buffer's memory-type bits
  // do not depend on its size, so a nominal probe is enough; BuildSlots sets the
  // real size before creating the slots.
  VkBufferCreateInfo probeCi{};
  probeCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  probeCi.size = kStagingAlign;
  probeCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  probeCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer probe = VK_NULL_HANDLE;
  if (dt.vkCreateBuffer(session->Device, &probeCi, nullptr, &probe) != VK_SUCCESS) {
    LOG_WARNING << "RestoreContentService: manifest: probe vkCreateBuffer failed for device key="
                << command.m_DeviceKey;
    m_Sessions[command.m_DeviceKey] = std::move(session);
    return;
  }
  VkMemoryRequirements req{};
  dt.vkGetBufferMemoryRequirements(session->Device, probe, &req);
  dt.vkDestroyBuffer(session->Device, probe, nullptr);

  constexpr VkMemoryPropertyFlags kPreferred =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  uint32_t memType = UINT32_MAX;
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((req.memoryTypeBits & (1u << i)) &&
        (memProps.memoryTypes[i].propertyFlags & kPreferred) == kPreferred) {
      memType = i;
      break;
    }
  }
  if (memType == UINT32_MAX) {
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
      if ((req.memoryTypeBits & (1u << i)) &&
          (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        memType = i;
        break;
      }
    }
  }
  if (memType == UINT32_MAX) {
    LOG_WARNING << "RestoreContentService: manifest: no HOST_VISIBLE memory type for staging on "
                   "device key="
                << command.m_DeviceKey;
    m_Sessions[command.m_DeviceKey] = std::move(session);
    return;
  }

  // Budget against the specific heap backing the chosen staging memory type.
  // Prefer live free bytes; the real restored resources already occupy memory at
  // this point, so the total heap size over-estimates what we can allocate.  The
  // total heap size is only a fallback when the live budget cannot be queried.
  const uint32_t stagingHeap = memProps.memoryTypes[memType].heapIndex;
  VkDeviceSize available = QueryHeapFreeBytes(instDt, session->PhysDevice, stagingHeap);
  if (available == 0) {
    available = memProps.memoryHeaps[stagingHeap].size;
  }
  // Slice of available memory we allow ourselves for ALL staging slots combined.
  const VkDeviceSize stagingBudget = available / kHostHeapDivisor;

  VkDeviceSize totalBytes = 0;
  for (const auto& r : session->Resources) {
    totalBytes += AlignUp(r.Size, kResourceAlign);
  }

  VkDeviceSize target = std::min(totalBytes, kMaxStagingBytes);
  if (stagingBudget > 0) {
    target = std::min(target, stagingBudget);
  }
  target = std::max(target, maxResource); // a single resource must always fit
  session->StagingSize = AlignUp(target, kStagingAlign);

  // ---- Pack resources into batches that each fit in one staging buffer ----
  size_t batch = 0;
  VkDeviceSize cursor = 0;
  for (auto& r : session->Resources) {
    VkDeviceSize base = AlignUp(cursor, kResourceAlign);
    if (cursor != 0 && base + r.Size > session->StagingSize) {
      ++batch;
      base = 0;
    }
    r.BatchIdx = batch;
    r.BaseOffset = base;
    cursor = base + r.Size;
  }
  session->NumBatches = batch + 1;

  // ---- Choose the pipeline depth ----
  // Start from the configured depth (bounded by the batch count) and shrink it
  // so the combined footprint of all slots stays within our staging budget.  A
  // slot that is a large fraction of available memory therefore collapses the
  // ring to a single slot (no pipelining, minimum footprint).
  size_t ring = std::max<size_t>(1, std::min(kRingDepth, session->NumBatches));
  while (ring > 1 && stagingBudget > 0 &&
         static_cast<VkDeviceSize>(ring) * session->StagingSize > stagingBudget) {
    --ring;
  }

  // The staging slots use the size decided above (memType was already chosen
  // from a nominal probe, which is size-independent for a plain buffer).
  probeCi.size = session->StagingSize;

  bool ok = BuildSlots(*session, ring, memType, probeCi);
  if (!ok && ring > 1) {
    // Live budget is only an estimate and the heap may be fragmented, so an
    // allocation can still fail.  Retry with a single slot: this halves (or
    // more) the footprint at the cost of pipelining, and is the only fallback
    // that helps when the whole ring, but not one slot, exceeds what is free.
    LOG_WARNING << "RestoreContentService: manifest: staging allocation for ring=" << ring
                << " failed on device key=" << command.m_DeviceKey
                << " - retrying with a single slot (no pipelining).";
    ring = 1;
    ok = BuildSlots(*session, ring, memType, probeCi);
  }

  session->Valid = ok;
  if (!ok) {
    LOG_WARNING << "RestoreContentService: manifest: staging slot creation failed for device key="
                << command.m_DeviceKey;
  } else {
    LOG_TRACE << "RestoreContentService: manifest device key=" << command.m_DeviceKey
              << " resources=" << session->Resources.size()
              << " stagingSize=" << session->StagingSize << " batches=" << session->NumBatches
              << " ring=" << ring;
  }

  m_Sessions[command.m_DeviceKey] = std::move(session);
}

bool RestoreContentService::BuildSlots(Session& session,
                                       size_t ring,
                                       uint32_t memType,
                                       const VkBufferCreateInfo& probeCi) {
  auto& dt = *session.Dt;
  session.Slots.assign(ring, Slot{});
  bool ok = true;
  for (auto& slot : session.Slots) {
    VkBufferCreateInfo bci = probeCi;
    if (dt.vkCreateBuffer(session.Device, &bci, nullptr, &slot.Buffer) != VK_SUCCESS) {
      ok = false;
      break;
    }
    VkMemoryRequirements slotReq{};
    dt.vkGetBufferMemoryRequirements(session.Device, slot.Buffer, &slotReq);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = slotReq.size;
    mai.memoryTypeIndex = memType;
    if (dt.vkAllocateMemory(session.Device, &mai, nullptr, &slot.Memory) != VK_SUCCESS) {
      ok = false;
      break;
    }
    if (dt.vkBindBufferMemory(session.Device, slot.Buffer, slot.Memory, 0) != VK_SUCCESS) {
      ok = false;
      break;
    }
    if (dt.vkMapMemory(session.Device, slot.Memory, 0, VK_WHOLE_SIZE, 0, &slot.Mapped) !=
        VK_SUCCESS) {
      ok = false;
      break;
    }
    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (dt.vkCreateFence(session.Device, &fci, nullptr, &slot.Fence) != VK_SUCCESS) {
      ok = false;
      break;
    }
  }
  if (!ok) {
    // Tear down whatever partial slots we created before returning so the caller
    // can retry with a smaller ring from a clean slate.  Nothing is in flight
    // yet, so this only destroys resources (no fence waits matter here).
    DestroySession(session);
  }
  return ok;
}

void RestoreContentService::BeginBatch(Session& session, size_t batchIdx) {
  Slot& slot = SlotFor(session, batchIdx);
  auto& dt = *session.Dt;
  if (slot.InFlight) {
    dt.vkWaitForFences(session.Device, 1, &slot.Fence, VK_TRUE, UINT64_MAX);
    dt.vkResetFences(session.Device, 1, &slot.Fence);
    if (slot.Cb != VK_NULL_HANDLE) {
      dt.vkFreeCommandBuffers(session.Device, session.Pool, 1, &slot.Cb);
      slot.Cb = VK_NULL_HANDLE;
    }
    slot.InFlight = false;
  }
}

void RestoreContentService::FlushBatch(Session& session, size_t batchIdx) {
  Slot& slot = SlotFor(session, batchIdx);
  auto& dt = *session.Dt;
  auto& hms = HandleMapService::Get();

  std::vector<ResourceDesc*> items;
  for (auto& r : session.Resources) {
    if (r.BatchIdx == batchIdx && r.Received) {
      items.push_back(&r);
    }
  }
  if (items.empty()) {
    return;
  }

  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = session.Pool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;
  if (dt.vkAllocateCommandBuffers(session.Device, &ai, &slot.Cb) != VK_SUCCESS) {
    LOG_WARNING << "RestoreContentService: FlushBatch: vkAllocateCommandBuffers failed";
    slot.Cb = VK_NULL_HANDLE;
    return;
  }

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  dt.vkBeginCommandBuffer(slot.Cb, &bi);

  // Make the host writes into the staging buffer visible to the transfer reads.
  VkMemoryBarrier hostBarrier{};
  hostBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  hostBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
  hostBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  dt.vkCmdPipelineBarrier(slot.Cb, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                          &hostBarrier, 0, nullptr, 0, nullptr);

  for (ResourceDesc* r : items) {
    if (!r->IsImage) {
      auto dstBuffer = reinterpret_cast<VkBuffer>(hms.TryGetHandle(r->DstKey));
      if (!dstBuffer) {
        LOG_WARNING << "RestoreContentService: unresolved dst buffer key=" << r->DstKey;
        continue;
      }
      VkBufferMemoryBarrier toDst{};
      toDst.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      toDst.srcAccessMask = 0;
      toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toDst.buffer = dstBuffer;
      toDst.offset = 0;
      toDst.size = VK_WHOLE_SIZE;
      dt.vkCmdPipelineBarrier(slot.Cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &toDst, 0, nullptr);

      VkBufferCopy region{r->BaseOffset, 0, r->Size};
      dt.vkCmdCopyBuffer(slot.Cb, slot.Buffer, dstBuffer, 1, &region);

      VkBufferMemoryBarrier toRead = toDst;
      toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      toRead.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      dt.vkCmdPipelineBarrier(slot.Cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &toRead, 0,
                              nullptr);
    } else {
      auto dstImage = reinterpret_cast<VkImage>(hms.TryGetHandle(r->DstKey));
      if (!dstImage) {
        LOG_WARNING << "RestoreContentService: unresolved dst image key=" << r->DstKey;
        continue;
      }
      VkImageMemoryBarrier toDst{};
      toDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      toDst.srcAccessMask = 0;
      toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toDst.image = dstImage;
      toDst.subresourceRange = {r->AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0,
                                VK_REMAINING_ARRAY_LAYERS};
      dt.vkCmdPipelineBarrier(slot.Cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toDst);

      // Regions store bufferOffset relative to this resource's data start; add
      // the resource's base offset inside the shared staging buffer.
      std::vector<VkBufferImageCopy> regions = r->Regions;
      for (auto& reg : regions) {
        reg.bufferOffset += r->BaseOffset;
      }
      dt.vkCmdCopyBufferToImage(slot.Cb, slot.Buffer, dstImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                static_cast<uint32_t>(regions.size()), regions.data());

      VkImageMemoryBarrier toFinal{};
      toFinal.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      toFinal.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      toFinal.newLayout = r->FinalLayout;
      toFinal.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toFinal.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      toFinal.image = dstImage;
      toFinal.subresourceRange = {r->AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0,
                                  VK_REMAINING_ARRAY_LAYERS};
      dt.vkCmdPipelineBarrier(slot.Cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                              &toFinal);
    }
  }

  dt.vkEndCommandBuffer(slot.Cb);

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &slot.Cb;
  if (dt.vkQueueSubmit(session.Queue, 1, &si, slot.Fence) != VK_SUCCESS) {
    LOG_WARNING << "RestoreContentService: FlushBatch: vkQueueSubmit failed";
    dt.vkFreeCommandBuffers(session.Device, session.Pool, 1, &slot.Cb);
    slot.Cb = VK_NULL_HANDLE;
    return;
  }
  slot.InFlight = true;
}

void RestoreContentService::OnData(const RestoreContentDataCommand& command) {
  auto it = m_Sessions.find(command.m_DeviceKey);
  if (it == m_Sessions.end() || !it->second) {
    return;
  }
  Session& session = *it->second;

  // An invalid session (staging setup failed) still counts data tokens so it is
  // torn down once the recorder's per-resource tokens have all been consumed.
  for (const auto& region : command.m_Regions.Regions) {
    ++session.Processed;

    if (!session.Valid) {
      continue;
    }

    const size_t resourceIndex = static_cast<size_t>(region.Offset);
    if (resourceIndex >= session.Resources.size()) {
      LOG_WARNING << "RestoreContentService: OnData: resource index " << resourceIndex
                  << " out of range for device key=" << command.m_DeviceKey;
      continue;
    }
    ResourceDesc& r = session.Resources[resourceIndex];
    const size_t batchIdx = r.BatchIdx;

    // Advance batches even for empty (failed-readback) resources so earlier
    // batches flush at the right time and CurrentBatch is always set.
    if (batchIdx != session.CurrentBatch) {
      if (session.CurrentBatch != SIZE_MAX) {
        FlushBatch(session, session.CurrentBatch);
      }
      BeginBatch(session, batchIdx);
      session.CurrentBatch = batchIdx;
    }

    if (region.Size == 0) {
      continue; // readback failed at record time: nothing to upload
    }

    Slot& slot = SlotFor(session, batchIdx);
    if (slot.Mapped == nullptr) {
      continue;
    }
    VkDeviceSize copySize = std::min<VkDeviceSize>(region.Size, r.Size);
    std::memcpy(static_cast<char*>(slot.Mapped) + r.BaseOffset, region.Data,
                static_cast<size_t>(copySize));
    r.Received = true;
  }

  // The manifest declared exactly Resources.size() resources and the recorder
  // emits one data token per resource, so once we have consumed that many we
  // flush the final batch, wait for all in-flight uploads, and tear down.
  if (session.Processed >= session.Resources.size()) {
    if (session.Valid && session.CurrentBatch != SIZE_MAX) {
      FlushBatch(session, session.CurrentBatch);
    }
    DestroySession(session);
    m_Sessions.erase(it);
  }
}

void RestoreContentService::DestroySession(Session& session) {
  if (session.Dt == nullptr || session.Device == VK_NULL_HANDLE) {
    return;
  }
  auto& dt = *session.Dt;
  for (auto& slot : session.Slots) {
    if (slot.InFlight && slot.Fence != VK_NULL_HANDLE) {
      dt.vkWaitForFences(session.Device, 1, &slot.Fence, VK_TRUE, UINT64_MAX);
      slot.InFlight = false;
    }
    if (slot.Cb != VK_NULL_HANDLE) {
      dt.vkFreeCommandBuffers(session.Device, session.Pool, 1, &slot.Cb);
      slot.Cb = VK_NULL_HANDLE;
    }
    if (slot.Mapped != nullptr && slot.Memory != VK_NULL_HANDLE) {
      dt.vkUnmapMemory(session.Device, slot.Memory);
      slot.Mapped = nullptr;
    }
    if (slot.Fence != VK_NULL_HANDLE) {
      dt.vkDestroyFence(session.Device, slot.Fence, nullptr);
      slot.Fence = VK_NULL_HANDLE;
    }
    if (slot.Buffer != VK_NULL_HANDLE) {
      dt.vkDestroyBuffer(session.Device, slot.Buffer, nullptr);
      slot.Buffer = VK_NULL_HANDLE;
    }
    if (slot.Memory != VK_NULL_HANDLE) {
      dt.vkFreeMemory(session.Device, slot.Memory, nullptr);
      slot.Memory = VK_NULL_HANDLE;
    }
  }
  session.Slots.clear();
}

} // namespace vulkan
} // namespace gits
