// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"

#include "clientArraysLayerAuto.h"
#include "encoderLayerAuto.h"
#include "mappedTextureLayer.h"
#include "pixelFormatCaptureLayer.h"
#include "uniformArrayCaptureLayer.h"
#include "unsupportedCommandTypes.h"
#include "windowGeometryTrackingLayer.h"

namespace gits::opengl2 {

// See SetPendingPreHookKey()/TakePendingPreHookKey()'s own header comment (captureManager.h).
thread_local uint64_t CaptureManager::s_PendingPreHookKey = 0;
thread_local uint64_t CaptureManager::s_PendingPostHookKey = 0;
thread_local uint64_t CaptureManager::s_PendingMappedTextureKey = 0;

CaptureManager& CaptureManager::Get() {
  // Thread-safe function-local static initialization (C++11 "magic statics") replaces a
  // hand-rolled, unsynchronized check-then-write of a raw pointer, which could race two
  // concurrent first-callers (GL wrapper threads) into constructing two different
  // CaptureManager instances - splitting command keys/recording across them. The instance is
  // still heap-allocated and intentionally never destroyed (see this method's declaration
  // comment in captureManager.h) - only the *construction* was unsynchronized before; the
  // "never destroyed" property is unaffected by using a static pointer instead of a static
  // value here.
  static CaptureManager* instance = new CaptureManager();
  return *instance;
}

stream::OrderingRecorder& CaptureManager::GetOrderingRecorder() {
  std::call_once(m_RecorderInitFlag,
                 [this] { m_Recorder = std::make_unique<stream::OrderingRecorder>(); });
  return *m_Recorder;
}

void CaptureManager::RecordCommand(Command& command, stream::CommandSerializer* serializer) {
  // See the header comment: command.m_Key must already be set by the caller - `serializer`
  // was built from `command` before this call, so setting it here would not affect the
  // already-encoded bytes.
  GetOrderingRecorder().Record(command.m_Key, serializer);
}

void CaptureManager::RecordUnsupportedCommand(const char* name, uint64_t key, uint32_t threadId) {
  UnsupportedCommand command(threadId, name);
  command.m_Key = key;
  RecordCommand(command, new UnsupportedCommandSerializer(command));
}

void CaptureManager::SkipCommandKey(uint64_t key) {
  GetOrderingRecorder().Skip(key);
}

void CaptureManager::LoadLayers() {
  // See the header comment: heap-allocated and intentionally never deleted, matching this
  // singleton's own never-destroyed pattern (Get()'s comment above) - EncoderLayer/
  // WindowGeometryTrackingLayer/PixelFormatCaptureLayer hold no resources that need releasing
  // (WindowGeometryTrackingLayer's actual state lives in the separately-singleton
  // WindowGeometryTrackingService; PixelFormatCaptureLayer is fully stateless), so this
  // introduces no static-destruction-order risk.
  //
  // WindowGeometryTrackingLayer is this backend's first real Pre layer (m_PreLayers was empty
  // before it - see this class's own header comment) - see that layer's own header comment
  // for why running in Pre(), ahead of EncoderLayer's Post-side serialization of the WGL call
  // a given hook is attached to, matters for window-creation ordering on replay.
  m_PreLayers.push_back(new WindowGeometryTrackingLayer());
  // Records client-memory vertex arrays ahead of the draws that read them - see
  // clientArraysService.h.
  m_PreLayers.push_back(new ClientArraysLayer());
  // Captures still-mapped GL_INTEL_map_texture textures at swaps - see mappedTextureLayer.h.
  m_PreLayers.push_back(new MappedTextureLayer());
  // One stateless instance in both lists: Pre() for wglCreatePbufferARB/EXT, Post() for
  // wglSetPixelFormat.
  auto* pixelFormatCaptureLayer = new PixelFormatCaptureLayer();
  m_PreLayers.push_back(pixelFormatCaptureLayer);
  // PixelFormatCaptureLayer must run BEFORE EncoderLayer (see that layer's own header comment
  // for why) - pushed first, since m_PostLayers runs in push_back order and EncoderLayer must
  // stay the LAST entry (see this class's own header comment on that invariant).
  m_PostLayers.push_back(pixelFormatCaptureLayer);
  m_PostLayers.push_back(new UniformArrayCaptureLayer());
  m_PostLayers.push_back(new EncoderLayer());
}

} // namespace gits::opengl2
