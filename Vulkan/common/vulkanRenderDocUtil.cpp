// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanRenderDocUtil.h"
#include "vulkanStateDynamic.h"
#include "log.h"

#ifdef GITS_PLATFORM_WINDOWS

namespace gits::Vulkan {

//
// RenderDocCapturer
//

uint32_t RenderDocUtil::RenderDocCapturer::ID = 1;

RenderDocUtil::RenderDocCapturer::RenderDocCapturer(VkInstance instance)
    : vkInstance(instance),
      capturerID(ID++),
      device(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance)),
      isRecording(false) {}

void RenderDocUtil::RenderDocCapturer::SetCapturePath() {
  const auto& cfg = Configurator::Get();
  const auto& cfgRenderDoc = cfg.vulkan.player.renderDoc;

  std::string baseName;
  std::string fileName;

  if (cfgRenderDoc.mode == TVkRenderDocCaptureMode::FRAMES) {
    baseName = "frame_";
    if (cfgRenderDoc.continuousCapture) {
      fileName = baseName + cfgRenderDoc.captureRange.StrValue();
    } else {
      fileName = baseName + std::to_string(CGits::Instance().CurrentFrame());
    }
  } else if (cfgRenderDoc.mode == TVkRenderDocCaptureMode::QUEUE_SUBMIT) {
    baseName = "queuesubmit_";
    if (cfgRenderDoc.continuousCapture) {
      fileName = baseName + cfgRenderDoc.captureRange.StrValue();
    } else {
      fileName = baseName + std::to_string(CGits::Instance().vkCounters.CurrentQueueSubmitCount());
    }
  }

  std::stringstream capturePath;
  capturePath << cfg.common.player.streamDir.string() << "/RenderDoc/" << fileName << "_inst_"
              << capturerID;
  std::string capturePathStr = capturePath.str();
  rdoc->SetCaptureFilePathTemplate(capturePathStr.c_str());
  LOG_INFO << "Set RenderDoc capture path to: " << capturePathStr;
}

void RenderDocUtil::RenderDocCapturer::Start() {
  if (isRecording) {
    return;
  }
  rdoc->StartFrameCapture(device, nullptr);
  isRecording = true;
  LOG_INFO << "Renderdoc capture started";
}

void RenderDocUtil::RenderDocCapturer::Stop() {
  if (!isRecording) {
    return;
  }
  SetCapturePath();
  rdoc->EndFrameCapture(device, nullptr);
  isRecording = false;
  LOG_INFO << "Renderdoc capture ended";
}

void RenderDocUtil::RenderDocCapturer::LaunchUI() {
  rdoc->LaunchReplayUI(1, nullptr);
  while (rdoc->IsTargetControlConnected() != 1) {
    sleep_millisec(1000);
  }
}

//
// RenderDocUtil
//

std::string RenderDocUtil::dllpath = "";
RENDERDOC_API_1_0_0* RenderDocUtil::rdoc = nullptr;

RenderDocUtil::RenderDocUtil() : renderDocLibrary(NULL) {
  if (dllpath == "") {
    throw std::runtime_error("RenderDoc dll path not defined!");
  }

  renderDocLibrary = LoadLibrary(dllpath.c_str());
  if (renderDocLibrary == NULL) {
    throw std::runtime_error("Couldn't load RenderDoc library!");
  }

  pRENDERDOC_GetAPI getApi = nullptr;
  getApi = (pRENDERDOC_GetAPI)GetProcAddress(renderDocLibrary, "RENDERDOC_GetAPI");

  if (getApi == nullptr) {
    throw std::runtime_error("Couldn't get RENDERDOC_GetAPI address!");
  }

  int ret = getApi(eRENDERDOC_API_Version_1_0_0, (void**)&rdoc);
  if (ret != 1) {
    throw std::runtime_error("Couldn't get RenderDoc API!");
  }
  rdoc->SetCaptureKeys(NULL, 0);
  rdoc->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, 0);
}

RenderDocUtil& RenderDocUtil::GetInstance() {
  static RenderDocUtil instance;
  return instance;
}

RenderDocUtil::~RenderDocUtil() {
  if (renderDocLibrary) {
    FreeLibrary(renderDocLibrary);
  }
}

RenderDocUtil::RenderDocCapturer& RenderDocUtil::GetCapturer(VkInstance instance) {
  auto it = rdocCapturers.find(instance);
  if (it == rdocCapturers.end()) {
    auto result = rdocCapturers.insert({instance, RenderDocCapturer(instance)});
    it = result.first;
  }
  return it->second;
}

void RenderDocUtil::AddCapturer(VkInstance instance) {
  if (rdocCapturers.find(instance) == rdocCapturers.end()) {
    rdocCapturers.insert({instance, RenderDocCapturer(instance)});
  }
}

void RenderDocUtil::DeleteCapturer(VkInstance instance) {
  auto it = rdocCapturers.find(instance);
  if (it != rdocCapturers.end()) {
    (*it).second.Stop();
    rdocCapturers.erase(instance);
  }
}

void RenderDocUtil::StartRecording() {
  for (const auto& deviceState : SD()._devicestates) {
    auto instanceHandle =
        deviceState.second->physicalDeviceStateStore->instanceStateStore->instanceHandle;
    GetCapturer(instanceHandle).Start();
  }
}

void RenderDocUtil::StopRecording() {
  for (const auto& deviceState : SD()._devicestates) {
    auto instanceHandle =
        deviceState.second->physicalDeviceStateStore->instanceStateStore->instanceHandle;
    GetCapturer(instanceHandle).Stop();
  }
}

void RenderDocUtil::LaunchRenderDocUI() {
  for (const auto& deviceState : SD()._devicestates) {
    auto instanceHandle =
        deviceState.second->physicalDeviceStateStore->instanceStateStore->instanceHandle;
    GetCapturer(instanceHandle).LaunchUI();
  }
}

} // namespace gits::Vulkan
#endif // GITS_PLATFORM_WINDOWS
