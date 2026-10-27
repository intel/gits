// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingLayerAuto.h"
#include "clientArraysMetaRunner.h"
#include "clientArraysMetaTypes.h"
#include "mappedDataMetaTypes.h"
#include "mappedTextureDataMetaTypes.h"
#include "pixelFormatMetaTypes.h"
#include "uniformArrayMetaTypes.h"
#include "unsupportedCommandTypes.h"
#include "windowGeometryMetaTypes.h"

#include <set>
#include <vector>

namespace gits::opengl {

namespace {

// A list's arrays may reference blocks sent before the list, which the player can release
// before the range. The copy carries them and releases nothing.
ClientArraysMetaCommand WithReferencedBlocks(const ClientArraysMetaCommand& command) {
  ClientArraysMetaCommand copy = command;
  copy.m_ReleasedBlocks.Data.clear();
  std::set<uint64_t> carried(copy.m_BlockIds.Data.begin(), copy.m_BlockIds.Data.end());
  for (uint64_t id : copy.m_ArrayBlocks.Data) {
    std::vector<uint8_t> bytes;
    if (carried.insert(id).second && CopyClientArrayBlock(id, bytes)) {
      copy.m_BlockIds.Data.push_back(id);
      copy.m_BlockSizes.Data.push_back(bytes.size());
      copy.m_Data.Data.insert(copy.m_Data.Data.end(), bytes.begin(), bytes.end());
    }
  }
  copy.m_BlockIds.HasValue = !copy.m_BlockIds.Data.empty();
  copy.m_BlockSizes.HasValue = !copy.m_BlockSizes.Data.empty();
  copy.m_Data.HasValue = !copy.m_Data.Data.empty();
  return copy;
}

} // namespace

void RecordingLayer::Pre(MappedDataMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(MappedDataMetaSerializer(command));
  }
}

void RecordingLayer::Pre(WindowGeometryMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(WindowGeometryMetaSerializer(command));
  }
}

void RecordingLayer::Pre(ClientArraysMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(ClientArraysMetaSerializer(command));
  } else if (CompilingList(command.m_ThreadId)) {
    m_StateTracking->AddListCommand(
        command.m_ThreadId, Store<ClientArraysMetaSerializer>(WithReferencedBlocks(command)));
  }
}

void RecordingLayer::Pre(UniformArrayMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(UniformArrayMetaSerializer(command));
  }
}

void RecordingLayer::Pre(MappedTextureDataMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(MappedTextureDataMetaSerializer(command));
  }
}

void RecordingLayer::Pre(PixelFormatMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(PixelFormatMetaSerializer(command));
  }
}

void RecordingLayer::Pre(UnsupportedCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(UnsupportedCommandSerializer(command));
  }
}

void RecordingLayer::EndFrame(uint32_t threadId) {
  if (!m_RecordedFrameEndingSwap) {
    return;
  }
  m_RecordedFrameEndingSwap = false;
  m_Recorder.Record(FrameEndSerializer(FrameEndCommand(threadId)));
  if (!m_Range.InRange()) {
    m_Recorder.FinishRecording();
  }
}

} // namespace gits::opengl
