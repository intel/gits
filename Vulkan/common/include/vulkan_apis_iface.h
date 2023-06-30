// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "apis_iface.h"
#include "config.h"
#include "gits.h"

namespace gits {
namespace Vulkan {
class VulkanApi : public ApisIface::Api3d {
public:
  VulkanApi() : Api3d(ApisIface::Vulkan) {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().recorder.vulkan.capture.mode.find("All") != std::string::npos;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return Config::Get().recorder.vulkan.capture.mode.find("Frames") != std::string::npos;
  }
  virtual bool CfgRec_IsBenchmark() const {
    return Config::Get().recorder.vulkan.performance.benchmark;
  }
  virtual unsigned CfgRec_ExitFrame() const {
    return Config::Get().recorder.vulkan.capture.all.exitFrame;
  }
  virtual int CfgRec_StartFrame() const {
    return Config::Get().recorder.vulkan.capture.frames.startFrame;
  }
  virtual int CfgRec_StopFrame() const {
    return Config::Get().recorder.vulkan.capture.frames.stopFrame;
  }
  virtual const std::vector<unsigned>& CfgRec_StartKeys() const {
    return Config::Get().recorder.vulkan.capture.frames.startKeys;
  }
  virtual void Play_SwapAfterPrepare() const;
  virtual void Play_StateRestoreBegin() const;
  virtual void Play_StateRestoreEnd() const;
  virtual void Rec_StateRestoreFinished() const;
  virtual bool CfgRec_IsRenderPassMode() const {
    return Config::Get().recorder.vulkan.capture.objRange.rangeSpecial.objMode ==
           Config::MODE_VKRENDERPASS;
  }
  virtual bool CfgRec_IsCmdBufferMode() const {
    return Config::Get().recorder.vulkan.capture.objRange.rangeSpecial.objMode ==
           Config::MODE_VKCOMMANDBUFFER;
  }
  virtual bool CfgRec_IsQueueSubmitMode() const {
    return Config::Get().recorder.vulkan.capture.objRange.rangeSpecial.objMode ==
           Config::MODE_VKQUEUESUBMIT;
  }
  virtual bool CfgRec_IsObjectToRecord() const {
    return Config::Get()
        .recorder.vulkan.capture.objRange
        .rangeSpecial[CGits::Instance().vkCounters.CurrentQueueSubmitCount()];
  }
  virtual bool CfgRec_IsSubcapture() const {
    return (CfgRec_IsFramesMode() || CfgRec_IsCmdBufferMode() || CfgRec_IsQueueSubmitMode() ||
            CfgRec_IsRenderPassMode());
  }
  virtual ~VulkanApi() = default;
};
} // namespace Vulkan
} // namespace gits
