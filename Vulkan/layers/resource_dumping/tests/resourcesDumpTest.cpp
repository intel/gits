// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests of the Vulkan resources dump: the keys of the configuration, the layouts the
// copies transition the subresources from, the copied regions, the queue family ownership, the
// signals moved out of submissions and the conversion of texels to images.

#include "dumpKeys.h"
#include "dumpTarget.h"
#include "imageLayouts.h"
#include "imageDumpWriter.h"
#include "queueFamilyOwnership.h"
#include "submitSignalsDeferral.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <exception>
#include <stdexcept>
#include <type_traits>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

using namespace gits::vulkan;

bool Throws(const std::string& keys) {
  try {
    ParseDumpKeys(keys);
  } catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

void TestKeys() {
  Check(ParseDumpKeys("").empty(), "keys: an empty list has no keys");
  Check(ParseDumpKeys(" , ").empty(), "keys: blank entries are skipped");

  const auto keys = ParseDumpKeys("12, O34,S5,OS6");
  Check(keys.size() == 4, "keys: four keys are parsed");
  Check(keys.count(12) == 1, "keys: a decimal key");
  Check(keys.count(34) == 1, "keys: an object key with the O prefix");
  Check(keys.count(5 | STATE_RESTORE_KEY_MASK) == 1, "keys: a state restore key");
  Check(keys.count(6 | STATE_RESTORE_KEY_MASK) == 1, "keys: a state restore object key");

  Check(Throws("12a"), "keys: trailing characters are rejected");
  Check(Throws("S"), "keys: a prefix without digits is rejected");
  Check(Throws("-3"), "keys: a negative key is rejected");
  Check(Throws("99999999999999999999999"), "keys: a key out of range is rejected");
  Check(Throws("9223372036854775808"), "keys: a key with the state restore bit is rejected");

  Check(DumpKeyToString(42) == "42", "keys: a key prints as decimal");
  Check(DumpKeyToString(7 | STATE_RESTORE_KEY_MASK) == "S7",
        "keys: a state restore key prints with the S prefix");
}

void TestLayouts() {
  ImageLayouts color(VK_FORMAT_R8G8B8A8_UNORM, false, 3, 2, VK_IMAGE_LAYOUT_UNDEFINED);
  Check(color.AspectSlots() == 1, "layouts: a color format has one aspect slot");
  color.Set({VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_REMAINING_MIP_LEVELS, 1, 1}, VK_IMAGE_LAYOUT_GENERAL);
  Check(color.Get(0, 0, 1) == VK_IMAGE_LAYOUT_UNDEFINED, "layouts: mip 0 is outside the range");
  Check(color.Get(0, 1, 1) == VK_IMAGE_LAYOUT_GENERAL, "layouts: mip 1 of layer 1 is set");
  Check(color.Get(0, 2, 1) == VK_IMAGE_LAYOUT_GENERAL, "layouts: remaining mips are set");
  Check(color.Get(0, 2, 0) == VK_IMAGE_LAYOUT_UNDEFINED, "layouts: layer 0 is outside the range");

  color.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 10, 0, VK_REMAINING_ARRAY_LAYERS},
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  Check(color.Get(0, 2, 0) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        "layouts: a level count past the image is clamped");

  ImageLayouts depthStencil(VK_FORMAT_D24_UNORM_S8_UINT, false, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED);
  Check(depthStencil.AspectSlots() == 2, "layouts: depth and stencil have separate slots");
  depthStencil.Set({VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
                   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  Check(depthStencil.Get(0, 0, 0) == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        "layouts: a depth barrier sets the depth slot");
  Check(depthStencil.Get(1, 0, 0) == VK_IMAGE_LAYOUT_UNDEFINED,
        "layouts: a depth barrier leaves the stencil slot");
  Check(depthStencil.SlotOf(VK_IMAGE_ASPECT_STENCIL_BIT) == 1,
        "layouts: stencil copies use the stencil slot");

  ImageLayouts depth(VK_FORMAT_D32_SFLOAT, false, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED);
  Check(depth.AspectSlots() == 1, "layouts: a depth only format has one slot");
  Check(depth.SlotAspect(0) == VK_IMAGE_ASPECT_DEPTH_BIT,
        "layouts: barriers name the depth aspect of a depth only format");

  ImageLayouts planar(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, false, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED);
  Check(planar.AspectSlots() == 1, "layouts: the planes of a non-disjoint image share a slot");
  Check(planar.SlotAspect(0) == VK_IMAGE_ASPECT_COLOR_BIT,
        "layouts: barriers name the color aspect of a non-disjoint multi-planar image");

  ImageLayouts disjoint(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, true, 2, 1, VK_IMAGE_LAYOUT_UNDEFINED);
  Check(disjoint.AspectSlots() == 3, "layouts: each plane of a disjoint image has a slot");
  Check(disjoint.SlotAspect(1) == VK_IMAGE_ASPECT_PLANE_1_BIT,
        "layouts: barriers name the plane of a disjoint image slot");
  Check(disjoint.SlotOf(VK_IMAGE_ASPECT_PLANE_2_BIT) == 2,
        "layouts: plane copies use the slot of their plane");
  disjoint.Set({VK_IMAGE_ASPECT_PLANE_1_BIT, 0, 1, 0, 1}, VK_IMAGE_LAYOUT_GENERAL);
  Check(disjoint.Get(1, 0, 0) == VK_IMAGE_LAYOUT_GENERAL, "layouts: a plane barrier sets its slot");
  Check(disjoint.Get(0, 0, 0) == VK_IMAGE_LAYOUT_UNDEFINED &&
            disjoint.Get(2, 0, 0) == VK_IMAGE_LAYOUT_UNDEFINED,
        "layouts: a plane barrier leaves the other planes");
  disjoint.Set({VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, 0, 1}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  Check(disjoint.Get(0, 1, 0) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            disjoint.Get(1, 1, 0) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            disjoint.Get(2, 1, 0) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        "layouts: a color barrier on a disjoint image sets every plane");

  const VkImageSubresourceRange twoLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 2, 2};
  auto rendered = RenderedLayers(twoLayers, 4, 1, 0);
  Check(rendered.size() == 1 && rendered[0].baseArrayLayer == 2 && rendered[0].layerCount == 1,
        "rendered layers: the framebuffer layers start at the base layer of the view");
  rendered = RenderedLayers(twoLayers, 4, 5, 0);
  Check(rendered.size() == 1 && rendered[0].layerCount == 2,
        "rendered layers: no more layers than the view has");
  rendered = RenderedLayers(twoLayers, 4, 1, 0b10);
  Check(rendered.size() == 1 && rendered[0].baseArrayLayer == 3 && rendered[0].layerCount == 1,
        "rendered layers: a view mask selects the layers of its views");
  rendered = RenderedLayers({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS}, 6, 1,
                            0b11011);
  Check(rendered.size() == 2 && rendered[0].baseArrayLayer == 0 && rendered[0].layerCount == 2 &&
            rendered[1].baseArrayLayer == 3 && rendered[1].layerCount == 2,
        "rendered layers: the views of a mask are grouped into contiguous ranges");
  Check(RenderedLayers(twoLayers, 4, 1, 0b100).empty(),
        "rendered layers: views beyond the view layers are ignored");
}

// Non-dispatchable handles are 64-bit integers instead of pointers on 32-bit platforms.
template <typename T>
T FakeHandle(uint64_t value) {
  if constexpr (std::is_pointer_v<T>) {
    return reinterpret_cast<T>(static_cast<uintptr_t>(value));
  } else {
    return static_cast<T>(value);
  }
}

DumpTarget ImageDumpTarget(VkFormat format, VkExtent3D extent, ImageLayouts layouts) {
  DumpTarget target;
  target.Image = FakeHandle<VkImage>(1);
  target.Format = format;
  target.Extent = extent;
  target.Layouts = std::move(layouts);
  return target;
}

void TestBarriers() {
  // The render pass stores only the depth of a depth/stencil attachment.
  DumpTarget target =
      ImageDumpTarget(VK_FORMAT_D24_UNORM_S8_UINT, {4, 4, 1},
                      ImageLayouts(VK_FORMAT_D24_UNORM_S8_UINT, false, 1, 1,
                                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
  target.Selection.push_back({VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

  target.Layouts.Set({VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}, VK_IMAGE_LAYOUT_UNDEFINED);
  std::vector<VkImageMemoryBarrier> barriers = TransferSrcBarriers(target, false);
  Check(barriers.size() == 1 &&
            barriers[0].subresourceRange.aspectMask ==
                (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) &&
            barriers[0].oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        "barriers: without separate layouts both aspects are transitioned");

  barriers = TransferSrcBarriers(target, true);
  Check(barriers.size() == 1 &&
            barriers[0].subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT &&
            barriers[0].oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        "barriers: with separate layouts only the copied aspect is transitioned");

  target.Layouts.Set({VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1},
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  barriers = TransferSrcBarriers(target, true);
  Check(barriers.size() == 1 &&
            barriers[0].subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT,
        "barriers: an aspect that is not copied is not transitioned, whatever its layout");
  target.Layouts.Set({VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1}, VK_IMAGE_LAYOUT_UNDEFINED);

  std::vector<VkBufferImageCopy> regions;
  CopyRegions(target, regions);
  Check(regions.size() == 1 && regions[0].imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT,
        "regions: only the selected aspect is copied");

  // The stencil has no transfer source usage.
  target.Selection.clear();
  target.Layouts.Set({VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1},
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  target.ExcludedAspects = VK_IMAGE_ASPECT_STENCIL_BIT;
  barriers = TransferSrcBarriers(target, true);
  regions.clear();
  CopyRegions(target, regions);
  Check(barriers.size() == 1 &&
            barriers[0].subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT &&
            regions.size() == 1 &&
            regions[0].imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT,
        "barriers: an excluded aspect is neither transitioned nor copied");

  DumpTarget color = ImageDumpTarget(VK_FORMAT_R8G8B8A8_UNORM, {4, 4, 1},
                                     ImageLayouts(VK_FORMAT_R8G8B8A8_UNORM, false, 1, 3,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
  barriers = TransferSrcBarriers(color, false);
  Check(barriers.size() == 1 && barriers[0].subresourceRange.layerCount == 3,
        "barriers: consecutive layers share a barrier");
  color.Layouts.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, 1}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  ImageLayouts actual = color.Layouts;
  Check(!FindLayoutMismatch(color, actual), "mismatch: the layouts the copies expect");
  actual.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 2, 1}, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  const std::optional<LayoutMismatch> mismatch = FindLayoutMismatch(color, actual);
  Check(mismatch && mismatch->Expected == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
            mismatch->Actual == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        "mismatch: a copied layer in another layout");
  actual.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 2, 1}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  color.Selection.push_back({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
  actual.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, 2}, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  Check(!FindLayoutMismatch(color, actual), "mismatch: layers that are not copied are ignored");

  // 2D views of different slices of a 3D image both select its only layer.
  DumpTarget slices = ImageDumpTarget(VK_FORMAT_R8G8B8A8_UNORM, {4, 4, 4},
                                      ImageLayouts(VK_FORMAT_R8G8B8A8_UNORM, false, 1, 1,
                                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  slices.SliceCount = 1;
  DumpTarget otherSlices = slices;
  otherSlices.BaseSlice = 2;
  barriers = TransferSrcBarriers(std::vector<const DumpTarget*>{&slices, &otherSlices}, false);
  Check(barriers.size() == 1 && barriers[0].subresourceRange.layerCount == 1,
        "barriers: a layer that several targets copy is transitioned once");
  DumpTarget firstLayers = ImageDumpTarget(VK_FORMAT_R8G8B8A8_UNORM, {4, 4, 1},
                                           ImageLayouts(VK_FORMAT_R8G8B8A8_UNORM, false, 1, 3,
                                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
  DumpTarget lastLayers = firstLayers;
  firstLayers.Selection = {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2}};
  lastLayers.Selection = {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, 2}};
  barriers = TransferSrcBarriers(std::vector<const DumpTarget*>{&firstLayers, &lastLayers}, false);
  Check(barriers.size() == 1 && barriers[0].subresourceRange.baseArrayLayer == 0 &&
            barriers[0].subresourceRange.layerCount == 3,
        "barriers: overlapping selections transition each layer once");

  DumpTarget inPlace = ImageDumpTarget(
      VK_FORMAT_B8G8R8A8_UNORM, {4, 4, 1},
      ImageLayouts(VK_FORMAT_B8G8R8A8_UNORM, false, 1, 3, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR));
  inPlace.Layouts.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, 1}, VK_IMAGE_LAYOUT_GENERAL);
  inPlace.Layouts.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 2, 1},
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  barriers = TransferSrcBarriers(inPlace, false);
  regions.clear();
  CopyRegions(inPlace, regions);
  constexpr VkImageLayout copiedIn[] = {VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR, VK_IMAGE_LAYOUT_GENERAL,
                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
  bool copiedInTheirLayouts = regions.size() == 3;
  for (const VkBufferImageCopy& region : regions) {
    copiedInTheirLayouts =
        copiedInTheirLayouts && CopySourceLayout(inPlace, region.imageSubresource) ==
                                    copiedIn[region.imageSubresource.baseArrayLayer];
  }
  Check(barriers.size() == 1 && barriers[0].subresourceRange.baseArrayLayer == 2 &&
            barriers[0].subresourceRange.layerCount == 1 &&
            barriers[0].oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
            copiedInTheirLayouts,
        "barriers: general and shared presentable layers are copied in their layouts without a "
        "transition");
}

void TestRegions() {
  // A 2D view of slices 2 to 4 of mip 1 of a 3D image.
  DumpTarget target =
      ImageDumpTarget(VK_FORMAT_R8G8B8A8_UNORM, {8, 8, 16},
                      ImageLayouts(VK_FORMAT_R8G8B8A8_UNORM, false, 2, 1, VK_IMAGE_LAYOUT_GENERAL));
  target.Selection.push_back({VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, 0, 1});
  target.BaseSlice = 2;
  target.SliceCount = 3;
  std::vector<VkBufferImageCopy> regions;
  const VkDeviceSize size = CopyRegions(target, regions);
  Check(regions.size() == 1, "regions: one mip of the 3D image is copied");
  Check(regions.size() == 1 && regions[0].imageSubresource.mipLevel == 1 &&
            regions[0].imageOffset.z == 2 && regions[0].imageExtent.depth == 3 &&
            regions[0].bufferOffset == 0,
        "regions: the slices of the view are copied");
  Check(size == 4 * 4 * 3 * 4, "regions: the staging holds the copied slices");

  target.BaseSlice = 6;
  target.SliceCount = VK_REMAINING_ARRAY_LAYERS;
  regions.clear();
  CopyRegions(target, regions);
  Check(regions.size() == 1 && regions[0].imageOffset.z == 6 && regions[0].imageExtent.depth == 2,
        "regions: the remaining slices of the mip are copied");

  DumpTarget layers = ImageDumpTarget(
      VK_FORMAT_R8G8B8A8_UNORM, {4, 4, 1},
      ImageLayouts(VK_FORMAT_R8G8B8A8_UNORM, false, 1, 2, VK_IMAGE_LAYOUT_UNDEFINED));
  layers.Layouts.Set({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1, 1}, VK_IMAGE_LAYOUT_GENERAL);
  regions.clear();
  Check(CopyRegions(layers, regions) == 4 * 4 * 4 && regions.size() == 1 &&
            regions[0].imageSubresource.baseArrayLayer == 1 && regions[0].bufferOffset == 0,
        "regions: undefined layers are not copied and take no staging");
}

void TestOwnership() {
  const auto use = [](uint32_t family, bool whole) {
    return OwnershipChange{OwnershipChange::Kind::Use, family, whole};
  };
  Ownership ownership;
  ApplyOwnershipChange(ownership, use(0, false));
  Check(ownership.Is == Ownership::State::Owned && ownership.QueueFamilyIndex == 0,
        "ownership: a use establishes the owner");

  ApplyOwnershipChange(ownership, {OwnershipChange::Kind::Release, 1, true});
  Check(ownership.Is == Ownership::State::Released && ownership.QueueFamilyIndex == 1,
        "ownership: a release leaves the resource without an owner");
  ApplyOwnershipChange(ownership, {OwnershipChange::Kind::Acquire, 1, true});
  Check(ownership.Is == Ownership::State::Owned && ownership.QueueFamilyIndex == 1,
        "ownership: the acquire gives the resource to the destination");

  ApplyOwnershipChange(ownership, {OwnershipChange::Kind::Release, 0, false});
  Check(ownership.Is == Ownership::State::Mixed, "ownership: a partial release mixes ownership");
  ApplyOwnershipChange(ownership, {OwnershipChange::Kind::Acquire, 0, false});
  Check(ownership.Is == Ownership::State::Mixed, "ownership: a partial acquire keeps it mixed");
  ApplyOwnershipChange(ownership, use(2, true));
  Check(ownership.Is == Ownership::State::Owned && ownership.QueueFamilyIndex == 2,
        "ownership: a use of the whole resource takes it");
  ApplyOwnershipChange(ownership, use(0, false));
  Check(ownership.Is == Ownership::State::Mixed,
        "ownership: a partial use on another queue family mixes ownership");
}

void TestSignalsDeferral() {
  const VkSemaphore binary = FakeHandle<VkSemaphore>(10);
  const VkSemaphore timeline = FakeHandle<VkSemaphore>(11);
  const VkFence fence = FakeHandle<VkFence>(12);
  const VkSemaphore semaphores[] = {binary, timeline};
  const uint64_t values[] = {0, 7};
  // The timeline semaphore has reached the value 2, and the other semaphores are binary.
  const MayWaitForLaterSignals laterSignals = [timeline](VkSemaphore semaphore, uint64_t value) {
    return semaphore == timeline && value > 2;
  };

  VkTimelineSemaphoreSubmitInfo timelineInfo{};
  timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo.signalSemaphoreValueCount = 2;
  timelineInfo.pSignalSemaphoreValues = values;
  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = &timelineInfo;
  submit.signalSemaphoreCount = 2;
  submit.pSignalSemaphores = semaphores;
  VkFence submitFence = fence;

  SubmitSignalsDeferral deferral;
  Check(deferral.Defer(1, &submit, submitFence, laterSignals, false) == nullptr,
        "signals: they are deferred");
  Check(submit.signalSemaphoreCount == 0 && timelineInfo.signalSemaphoreValueCount == 0 &&
            submitFence == VK_NULL_HANDLE,
        "signals: the submission signals nothing");
  std::optional<SubmitSignals> signals = deferral.Restore();
  Check(signals && signals->Fence == fence && signals->TimelineValues &&
            signals->Batches.size() == 1 && signals->Batches[0].size() == 2 &&
            signals->Batches[0][1].semaphore == timeline && signals->Batches[0][1].value == 7,
        "signals: the deferred signals keep their semaphores, values and fence");
  Check(submit.signalSemaphoreCount == 2 && timelineInfo.signalSemaphoreValueCount == 2 &&
            submitFence == fence,
        "signals: the submission is restored");
  Check(!deferral.Restore(), "signals: they are restored once");

  // Two batches share their VkTimelineSemaphoreSubmitInfo.
  VkSubmitInfo sharing[2] = {submit, submit};
  sharing[1].signalSemaphoreCount = 1;
  sharing[1].pSignalSemaphores = &timeline;
  const uint64_t sharedValues[] = {7};
  VkTimelineSemaphoreSubmitInfo sharedInfo{};
  sharedInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  sharedInfo.signalSemaphoreValueCount = 1;
  sharedInfo.pSignalSemaphoreValues = sharedValues;
  sharing[0].pNext = &sharedInfo;
  sharing[0].signalSemaphoreCount = 1;
  sharing[0].pSignalSemaphores = &timeline;
  sharing[1].pNext = &sharedInfo;
  Check(deferral.Defer(2, sharing, submitFence, laterSignals, false) == nullptr,
        "signals: batches sharing their timeline values are deferred");
  signals = deferral.Restore();
  Check(signals && signals->Batches.size() == 2 && signals->Batches[0][0].value == 7 &&
            signals->Batches[1][0].value == 7,
        "signals: batches sharing their timeline values keep them");
  Check(sharedInfo.signalSemaphoreValueCount == 1 && sharing[1].signalSemaphoreCount == 1,
        "signals: shared timeline values are restored");

  VkProtectedSubmitInfo protectedInfo{};
  protectedInfo.sType = VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO;
  protectedInfo.protectedSubmit = VK_TRUE;
  submit.pNext = &protectedInfo;
  Check(deferral.Defer(1, &submit, submitFence, laterSignals, false) != nullptr &&
            submit.signalSemaphoreCount == 2 && submitFence == fence,
        "signals: a protected submission is left as it is");
  Check(!deferral.Restore(), "signals: nothing is restored when they are not deferred");

  VkSubmitInfo2 submit2{};
  submit2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit2.flags = VK_SUBMIT_PROTECTED_BIT;
  Check(deferral.Defer(1, &submit2, submitFence, SubmitSignals::Function::QueueSubmit2,
                       laterSignals, false) != nullptr,
        "signals: a protected submission with synchronization2 is left as it is");

  // Two batches signal the same timeline semaphore, and the second one waits for the first.
  VkSemaphoreSubmitInfo first{};
  first.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  first.semaphore = timeline;
  first.value = 1;
  first.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  VkSemaphoreSubmitInfo second = first;
  second.value = 2;
  VkSubmitInfo2 batches[2]{};
  for (VkSubmitInfo2& batch : batches) {
    batch.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    batch.signalSemaphoreInfoCount = 1;
  }
  batches[0].pSignalSemaphoreInfos = &first;
  batches[1].pSignalSemaphoreInfos = &second;
  batches[1].waitSemaphoreInfoCount = 1;
  batches[1].pWaitSemaphoreInfos = &first;
  Check(deferral.Defer(2, batches, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) != nullptr &&
            batches[0].signalSemaphoreInfoCount == 1 && batches[1].signalSemaphoreInfoCount == 1,
        "signals: a batch waiting for an earlier batch keeps the submission as it is");

  // Another queue may wait for the first signal before it signals what the second batch waits for.
  VkSemaphoreSubmitInfo other = first;
  other.semaphore = FakeHandle<VkSemaphore>(12);
  batches[1].pWaitSemaphoreInfos = &other;
  Check(deferral.Defer(2, batches, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) != nullptr &&
            batches[0].signalSemaphoreInfoCount == 1,
        "signals: a batch waiting for anything after an earlier signal keeps the submission");

  batches[1].waitSemaphoreInfoCount = 0;
  Check(deferral.Defer(2, batches, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) == nullptr,
        "signals: batches that do not wait after a signal are deferred");
  signals = deferral.Restore();
  Check(signals && signals->Batches.size() == 2 && signals->Batches[0][0].value == 1 &&
            signals->Batches[1][0].value == 2,
        "signals: a semaphore signaled by two batches stays in two batches");
  Check(signals && signals->Batches[0][0].stageMask == VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        "signals: deferred signals wait for all commands, the copies included");

  // A batch without command buffers waits at the transfer stage for a binary semaphore, whose
  // signal operation is submitted before the wait.
  VkSemaphoreSubmitInfo signal = first;
  signal.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  VkSemaphoreSubmitInfo wait = other;
  wait.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  VkSubmitInfo2 staged{};
  staged.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  staged.waitSemaphoreInfoCount = 1;
  staged.pWaitSemaphoreInfos = &wait;
  staged.signalSemaphoreInfoCount = 1;
  staged.pSignalSemaphoreInfos = &signal;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) != nullptr &&
            staged.signalSemaphoreInfoCount == 1,
        "signals: signals at specific stages are not deferred");
  signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) == nullptr,
        "signals: a binary semaphore wait of a batch without command buffers is deferred");
  deferral.Restore();

  // Another queue may wait for the signal before it signals the timeline value, and having command
  // buffers does not make the signal wait for the timeline value.
  wait.semaphore = timeline;
  wait.value = 3;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) != nullptr &&
            staged.signalSemaphoreInfoCount == 1,
        "signals: a wait for a timeline value not reached yet keeps the submission as it is");
  VkCommandBufferSubmitInfo commandBufferInfo{};
  commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  staged.commandBufferInfoCount = 1;
  staged.pCommandBufferInfos = &commandBufferInfo;
  wait.stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) != nullptr,
        "signals: a wait for a timeline value not reached yet keeps the submission as it is, "
        "whatever its stages and command buffers");
  wait.value = 2;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) == nullptr,
        "signals: a wait for a timeline value reached already is deferred");
  deferral.Restore();
  wait.value = 3;
  staged.signalSemaphoreInfoCount = 0;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       false) == nullptr,
        "signals: a submission signaling only a fence is deferred whatever its waits");
  deferral.Restore();

  // The copies wait for the semaphore waits of the earlier submissions to the queue, which neither
  // the fence nor the semaphore signals of the submission wait for.
  staged.waitSemaphoreInfoCount = 0;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       true) != nullptr &&
            submitFence == fence,
        "signals: a fence is not deferred after earlier work waiting for later signals");
  staged.signalSemaphoreInfoCount = 1;
  Check(deferral.Defer(1, &staged, submitFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       true) != nullptr &&
            staged.signalSemaphoreInfoCount == 1,
        "signals: semaphore signals are not deferred after earlier work waiting for later "
        "signals");
  VkFence noFence = VK_NULL_HANDLE;
  staged.signalSemaphoreInfoCount = 0;
  Check(deferral.Defer(1, &staged, noFence, SubmitSignals::Function::QueueSubmit2, laterSignals,
                       true) == nullptr,
        "signals: a submission signaling nothing is deferred after any earlier work");
  deferral.Restore();
  staged.waitSemaphoreInfoCount = 1;

  const VkPipelineStageFlags transferStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo waiting{};
  waiting.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  waiting.waitSemaphoreCount = 1;
  waiting.pWaitSemaphores = &other.semaphore;
  waiting.pWaitDstStageMask = &transferStage;
  waiting.signalSemaphoreCount = 1;
  waiting.pSignalSemaphores = &binary;
  Check(deferral.Defer(1, &waiting, submitFence, laterSignals, false) == nullptr,
        "signals: a binary semaphore wait of a vkQueueSubmit without command buffers is deferred");
  deferral.Restore();
  uint64_t waitValue = 3;
  VkTimelineSemaphoreSubmitInfo waitValues{};
  waitValues.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  waitValues.waitSemaphoreValueCount = 1;
  waitValues.pWaitSemaphoreValues = &waitValue;
  waiting.pNext = &waitValues;
  waiting.pWaitSemaphores = &timeline;
  const VkCommandBuffer commandBuffer = FakeHandle<VkCommandBuffer>(13);
  waiting.commandBufferCount = 1;
  waiting.pCommandBuffers = &commandBuffer;
  Check(deferral.Defer(1, &waiting, submitFence, laterSignals, false) != nullptr &&
            waiting.signalSemaphoreCount == 1,
        "signals: a vkQueueSubmit waiting for a timeline value not reached yet is kept as it is");
  waitValue = 2;
  Check(deferral.Defer(1, &waiting, submitFence, laterSignals, false) == nullptr,
        "signals: a vkQueueSubmit waiting for a timeline value reached already is deferred");
  deferral.Restore();
}

void TestConversion() {
  std::vector<uint8_t> rgba;

  const uint8_t bgra[] = {10, 20, 30, 40};
  Check(ConvertToRgba8(VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, bgra, 1, 1, rgba),
        "convert: B8G8R8A8 is converted");
  Check(rgba == std::vector<uint8_t>{30, 20, 10, 40}, "convert: B8G8R8A8 swaps red and blue");

  const uint8_t red[] = {200};
  Check(ConvertToRgba8(VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, red, 1, 1, rgba),
        "convert: R8 is converted");
  Check(rgba == std::vector<uint8_t>{200, 200, 200, 255}, "convert: one channel is gray");

  float floats[4] = {0.0f, 0.5f, 2.0f, -1.0f};
  uint8_t floatTexel[sizeof(floats)];
  std::memcpy(floatTexel, floats, sizeof(floats));
  Check(ConvertToRgba8(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, floatTexel, 1, 1,
                       rgba),
        "convert: R32G32B32A32_SFLOAT is converted");
  Check(rgba == std::vector<uint8_t>{0, 128, 255, 0}, "convert: floats are clamped to 0-1");

  const uint16_t halfOne = 0x3c00;
  uint8_t halfTexel[2];
  std::memcpy(halfTexel, &halfOne, sizeof(halfOne));
  Check(ConvertToRgba8(VK_FORMAT_R16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, halfTexel, 1, 1, rgba),
        "convert: R16_SFLOAT is converted");
  Check(rgba[0] == 255, "convert: half 1.0 is white");

  const int8_t snorm[] = {-127, 0, 127, 127};
  uint8_t snormTexel[4];
  std::memcpy(snormTexel, snorm, sizeof(snorm));
  Check(ConvertToRgba8(VK_FORMAT_R8G8B8A8_SNORM, VK_IMAGE_ASPECT_COLOR_BIT, snormTexel, 1, 1, rgba),
        "convert: R8G8B8A8_SNORM is converted");
  Check(rgba[0] == 0 && rgba[1] == 128 && rgba[2] == 255,
        "convert: signed normalized values map from -1 to 1");

  const uint32_t packed = (3u << 30) | (1023u << 20) | (0u << 10) | 512u;
  uint8_t packedTexel[4];
  std::memcpy(packedTexel, &packed, sizeof(packed));
  Check(ConvertToRgba8(VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_IMAGE_ASPECT_COLOR_BIT, packedTexel,
                       1, 1, rgba),
        "convert: A2B10G10R10 is converted");
  Check(rgba == std::vector<uint8_t>{128, 0, 255, 255}, "convert: A2B10G10R10 channels");

  const uint32_t depth24 = 0xff000000u | 0xffffffu;
  uint8_t depthTexel[4];
  std::memcpy(depthTexel, &depth24, sizeof(depth24));
  Check(ConvertToRgba8(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, depthTexel, 1, 1,
                       rgba),
        "convert: the depth of D24_UNORM_S8_UINT is converted");
  Check(rgba == std::vector<uint8_t>{255, 255, 255, 255},
        "convert: the depth ignores the upper byte");

  const uint8_t stencil[] = {7};
  Check(
      ConvertToRgba8(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_STENCIL_BIT, stencil, 1, 1, rgba),
      "convert: stencil is converted");
  Check(rgba[0] == 7, "convert: stencil values are written as they are");

  const uint8_t integer[] = {1, 2, 3, 4};
  rgba.clear();
  Check(!ConvertToRgba8(VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, integer, 1, 1, rgba),
        "convert: integer formats have no conversion");
  Check(!ConvertToRgba8(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, VK_IMAGE_ASPECT_PLANE_0_BIT, integer, 1,
                        1, rgba),
        "convert: planes have no conversion");
  Check(rgba.empty(), "convert: formats without a conversion allocate nothing");
}

} // namespace

int main() {
  try {
    TestKeys();
    TestLayouts();
    TestBarriers();
    TestRegions();
    TestOwnership();
    TestSignalsDeferral();
    TestConversion();
  } catch (const std::exception& e) {
    std::printf("resourcesDumpTest: unexpected exception: %s\n", e.what());
    return 1;
  } catch (...) {
    std::printf("resourcesDumpTest: unexpected exception\n");
    return 1;
  }

  if (g_Failures != 0) {
    std::printf("resourcesDumpTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("resourcesDumpTest: all checks passed\n");
  return 0;
}
