// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "apis_iface.h"

namespace gits {
namespace Vulkan {
class VulkanApi : public ApisIface::Api3d {
public:
  VulkanApi() : Api3d(ApisIface::Vulkan) {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().vulkan.recorder.mode == TVulkanRecorderMode::ALL;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return Config::Get().vulkan.recorder.mode == TVulkanRecorderMode::FRAMES;
  }
  virtual unsigned CfgRec_ExitFrame() const {
    return Config::Get().vulkan.recorder.all.exitFrame;
  }
  virtual int CfgRec_StartFrame() const {
    return Config::Get().vulkan.recorder.frames.startFrame;
  }
  virtual int CfgRec_StopFrame() const {
    return Config::Get().vulkan.recorder.frames.stopFrame;
  }
  virtual const std::vector<unsigned>& CfgRec_StartKeys() const {
    return Config::Get().vulkan.recorder.frames.startKeys;
  }
  virtual void Play_SwapAfterPrepare() const;
  virtual void Play_StateRestoreBegin() const;
  virtual void Play_StateRestoreEnd() const;
  virtual void Rec_StateRestoreFinished() const;
  virtual bool CfgRec_IsDrawsRangeMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_DRAW;
  }
  virtual bool CfgRec_IsBlitRangeMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_BLIT;
  }
  virtual bool CfgRec_IsDispatchRangeMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_DISPATCH;
  }
  virtual bool CfgRec_IsRenderPassMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_RENDER_PASS;
  }
  virtual bool CfgRec_IsCmdBufferMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_COMMAND_BUFFER;
  }
  virtual bool CfgRec_IsQueueSubmitMode() const {
    return Config::Get().vulkan.recorder.objRange.rangeSpecial.objMode ==
           VulkanObjectMode::MODE_VK_QUEUE_SUBMIT;
  }
  virtual bool CfgRec_IsObjectToRecord() const;
  virtual bool CfgRec_IsSubcapture() const {
    return !CfgRec_IsAllMode() && Config::IsRecorder();
  }
  virtual bool CfgRec_IsSubFrameMode() const {
    return (CfgRec_IsCmdBufferMode() || CfgRec_IsQueueSubmitMode() || CfgRec_IsRenderPassMode() ||
            CfgRec_IsDrawsRangeMode() || CfgRec_IsBlitRangeMode() || CfgRec_IsDispatchRangeMode());
  }
  virtual ~VulkanApi() = default;
};
} // namespace Vulkan
} // namespace gits
