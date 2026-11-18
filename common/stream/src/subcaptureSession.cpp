// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureSession.h"

#include "commandId.h"
#include "log.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace gits {
namespace stream {

namespace {

class MarkerSerializer : public CommandSerializer {
public:
  explicit MarkerSerializer(CommonCommandId id) : m_Id(id) {}
  uint32_t Id() const override {
    return static_cast<uint32_t>(m_Id);
  }

private:
  CommonCommandId m_Id;
};

const char* ApiName(StreamHeader::Api api) {
  switch (api) {
  case StreamHeader::Api::API_OPENGL:
    return "opengl";
  case StreamHeader::Api::API_VULKAN:
    return "vulkan";
  case StreamHeader::Api::API_DIRECTX:
    return "directx";
  case StreamHeader::Api::API_OPENCL:
    return "opencl";
  case StreamHeader::Api::API_LEVELZERO:
    return "levelzero";
  default:
    return "unknown";
  }
}

} // namespace

SubcaptureSession& SubcaptureSession::Get() {
  static SubcaptureSession* session = new SubcaptureSession();
  return *session;
}

bool SubcaptureSession::IsShared() {
  return StreamHeader::Get().HasMultipleApis();
}

std::string SubcaptureSession::AnalysisFileName(const std::filesystem::path& streamDir,
                                                const std::string& frames,
                                                StreamHeader::Api api) {
  std::string name = streamDir.filename().string() + "_frames-" + frames;
  if (IsShared()) {
    name += std::string("_") + ApiName(api);
  }
  return name + "_analysis.yml";
}

std::vector<std::string> SubcaptureSession::AnalysisFileNames(
    const std::filesystem::path& streamDir, const std::string& frames) {
  std::vector<std::string> names;
  for (StreamHeader::Api api : StreamHeader::Get().GetApis()) {
    names.push_back(AnalysisFileName(streamDir, frames, api));
  }
  return names;
}

void SubcaptureSession::Open(const std::filesystem::path& path, CompressionType compressionType) {
  if (m_Writer || m_Closed) {
    return;
  }
  m_Path = path;
  m_Writer = std::make_unique<StreamWriter>(path, compressionType);
}

void SubcaptureSession::Record(const CommandSerializer& serializer) {
  if (m_Writer) {
    m_Writer->Record(serializer);
  }
}

bool SubcaptureSession::Close() {
  if (!m_Writer) {
    return false;
  }
  m_Writer->Close();
  m_Writer.reset();
  m_Closed = true;
  return true;
}

uint32_t SubcaptureSession::AddParticipant(Participant* participant,
                                           unsigned restoreOrder,
                                           uint32_t startFrame) {
  m_StartFrame = startFrame;
  const auto position = std::upper_bound(
      m_Participants.begin(), m_Participants.end(), restoreOrder,
      [](unsigned order, const Entry& entry) { return order < entry.RestoreOrder; });
  m_Participants.insert(position, {participant, restoreOrder});
  return m_CurrentFrame;
}

void SubcaptureSession::RemoveParticipant(Participant* participant) {
  std::erase_if(m_Participants,
                [participant](const Entry& entry) { return entry.Value == participant; });
}

void SubcaptureSession::EndFrame(StreamHeader::Api api) {
  if (m_FrameApi == StreamHeader::Api::API_NOT_SET) {
    m_FrameApi = api;
  } else if (api != m_FrameApi) {
    throw std::runtime_error("Subcapture of a stream in which more than one API ends frames is "
                             "not supported: the frames of the subcapture would not match the "
                             "stream.");
  }
  // A range starting at frame 1 needs no state restore.
  if (m_Writer && m_StartFrame > 1 && m_CurrentFrame + 1 == m_StartFrame) {
    for (const Entry& entry : m_Participants) {
      entry.Value->PrepareRestoreState();
    }
    LOG_INFO << "Subcapture: restoring the state of all APIs";
    Record(MarkerSerializer(CommonCommandId::ID_INIT_START));
    for (const Entry& entry : m_Participants) {
      entry.Value->RestoreState();
    }
    Record(MarkerSerializer(CommonCommandId::ID_INIT_END));
  }
  for (const Entry& entry : m_Participants) {
    entry.Value->EndFrame();
  }
  ++m_CurrentFrame;
}

void SubcaptureSession::AddExport(uint64_t exportKey, uint64_t ownerKey) {
  m_Exports[exportKey] = ownerKey;
}

void SubcaptureSession::RemoveExports(uint64_t ownerKey) {
  std::erase_if(m_Exports, [ownerKey](const auto& entry) { return entry.second == ownerKey; });
  std::erase_if(m_HandBackLayouts,
                [ownerKey](const auto& entry) { return entry.first.first == ownerKey; });
}

bool SubcaptureSession::IsExportRestored(uint64_t exportKey) const {
  return m_Exports.contains(exportKey);
}

void SubcaptureSession::SetContentsNotRestored(uint64_t ownerKey, std::string reason) {
  m_ContentsNotRestored[ownerKey] = std::move(reason);
}

const std::string* SubcaptureSession::ContentsNotRestored(uint64_t exportKey) const {
  const auto owner = m_Exports.find(exportKey);
  if (owner == m_Exports.end()) {
    return nullptr;
  }
  const auto reason = m_ContentsNotRestored.find(owner->second);
  return reason != m_ContentsNotRestored.end() ? &reason->second : nullptr;
}

void SubcaptureSession::SetRestoredContents(uint64_t ownerKey, RestoredContents contents) {
  m_RestoredContents[ownerKey] = std::move(contents);
}

const RestoredContents* SubcaptureSession::GetRestoredContents(uint64_t exportKey) const {
  const auto owner = m_Exports.find(exportKey);
  if (owner == m_Exports.end()) {
    return nullptr;
  }
  const auto contents = m_RestoredContents.find(owner->second);
  return contents != m_RestoredContents.end() ? &contents->second : nullptr;
}

void SubcaptureSession::RequireContents(uint64_t exportKey, ContentRange range) {
  const auto owner = m_Exports.find(exportKey);
  if (owner == m_Exports.end()) {
    return;
  }
  if (const auto contents = m_RestoredContents.find(owner->second);
      contents != m_RestoredContents.end()) {
    contents->second.Required.push_back(range);
  }
}

bool SubcaptureSession::IsContentRequired(uint64_t ownerKey, ContentRange range) const {
  const auto contents = m_RestoredContents.find(ownerKey);
  return contents != m_RestoredContents.end() && Overlaps(contents->second.Required, range);
}

void SubcaptureSession::SetHandBackLayout(uint64_t exportKey, uint64_t offset, uint32_t layout) {
  const auto owner = m_Exports.find(exportKey);
  if (owner != m_Exports.end()) {
    m_HandBackLayouts[{owner->second, offset}] = layout;
  }
}

void SubcaptureSession::ClearHandBackLayout(uint64_t ownerKey, uint64_t offset) {
  m_HandBackLayouts.erase({ownerKey, offset});
}

std::optional<uint32_t> SubcaptureSession::GetHandBackLayout(uint64_t ownerKey,
                                                             uint64_t offset) const {
  const auto layout = m_HandBackLayouts.find({ownerKey, offset});
  if (layout == m_HandBackLayouts.end()) {
    return std::nullopt;
  }
  return layout->second;
}

void SubcaptureSession::AddSemaphoreExport(uint64_t exportKey,
                                           uint64_t semaphoreKey,
                                           bool signaled) {
  m_SemaphoreExports[exportKey] = semaphoreKey;
  m_SharedSemaphores.try_emplace(semaphoreKey, SharedSemaphore{signaled, false});
}

void SubcaptureSession::ImportSemaphore(uint64_t exportKey) {
  auto it = m_SemaphoreExports.find(exportKey);
  if (it != m_SemaphoreExports.end()) {
    m_SharedSemaphores[it->second].Imported = true;
  }
}

void SubcaptureSession::SetSemaphoreSignaled(uint64_t semaphoreKey, bool signaled) {
  auto it = m_SharedSemaphores.find(semaphoreKey);
  if (it != m_SharedSemaphores.end()) {
    it->second.Signaled = signaled;
  }
}

void SubcaptureSession::SetImportedSemaphoreSignaled(uint64_t exportKey, bool signaled) {
  auto it = m_SemaphoreExports.find(exportKey);
  if (it != m_SemaphoreExports.end()) {
    SetSemaphoreSignaled(it->second, signaled);
  }
}

std::optional<bool> SubcaptureSession::GetImportedSemaphoreSignaled(uint64_t semaphoreKey) const {
  auto it = m_SharedSemaphores.find(semaphoreKey);
  if (it == m_SharedSemaphores.end() || !it->second.Imported) {
    return std::nullopt;
  }
  return it->second.Signaled;
}

} // namespace stream
} // namespace gits
