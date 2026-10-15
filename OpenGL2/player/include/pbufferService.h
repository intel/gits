// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "glTypes.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace gits::opengl2 {

// WGL_ARB_pbuffer/WGL_EXT_pbuffer replay state. The pbuffer handle itself is a mapped handle
// (PbufferHandleMapService/PbufferEXTHandleMapService); this keeps what that mapping cannot:
//   - The capture-time pixel format attributes a PbufferPixelFormatMetaCommand delivers right
//     before its wglCreatePbufferARB/EXT, keyed by recorded thread.
//   - Each recorded pbuffer DC (wglGetPbufferDCARB/EXT) and the real one replay got for it, which
//     HdcVirtualizationService::Resolve() returns instead of the player window's DC. A driver may
//     return the same DC for repeated acquisitions, each matched by its own release, so every
//     acquisition is kept until its release.
class PbufferService {
public:
  static PbufferService& Get();

  void SetPendingPixelFormat(uint32_t threadId, std::vector<int32_t> attributes);
  // Empty when the thread has no pending attributes.
  std::vector<int32_t> TakePendingPixelFormat(uint32_t threadId);

  // `realDc` is nullptr when replay failed to get the DC capture got.
  void AcquireDc(HDC recordedDc, GITSKey pbufferKey, HDC realDc);
  // std::nullopt when `recordedDc` is not a live pbuffer DC; nullptr when it is, but replay
  // failed to get it, so calls on it must fail rather than reach the player window's DC.
  std::optional<HDC> FindDc(HDC recordedDc);
  // Forgets the latest acquisition of `recordedDc`.
  void ReleaseDc(HDC recordedDc);
  // Forgets every DC of a destroyed pbuffer that was not released first.
  void ErasePbuffer(GITSKey pbufferKey);

private:
  PbufferService() = default;

  struct PbufferDc {
    GITSKey Pbuffer{};
    HDC Real{};
  };

  std::mutex m_Mutex;
  std::unordered_map<uint32_t, std::vector<int32_t>> m_PendingPixelFormats;
  // Acquisitions not yet released, latest last.
  std::unordered_map<HDC, std::vector<PbufferDc>> m_Dcs;
};

} // namespace gits::opengl2
