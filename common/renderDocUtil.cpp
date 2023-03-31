// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "renderDocUtil.h"

#ifdef GITS_PLATFORM_WINDOWS

std::string gits::RenderDocUtil::dllpath = "";

gits::RenderDocUtil* gits::RenderDocUtil::instance;

gits::RenderDocUtil::RenderDocUtil()
    : rdoc(nullptr), renderDocLibrary(NULL), pRenderDocDevice(nullptr), isValid(false) {

  if (dllpath == "") {
    Log(ERR) << "RenderDoc dll path not defined!";
    return;
  }

  renderDocLibrary = LoadLibrary(dllpath.c_str());
  if (renderDocLibrary == NULL) {
    Log(ERR) << "Couldn't load RenderDoc library!";
    return;
  }

  pRENDERDOC_GetAPI getApi = nullptr;
  getApi = (pRENDERDOC_GetAPI)GetProcAddress(renderDocLibrary, "RENDERDOC_GetAPI");

  if (getApi == nullptr) {
    Log(ERR) << "Couldn't get RENDERDOC_GetAPI address!";
    return;
  }

  int ret = getApi(eRENDERDOC_API_Version_1_0_0, (void**)&rdoc);
  if (ret != 1) {
    Log(ERR) << "Couldn't get RenderDoc API!";
    return;
  }
  rdoc->SetCaptureKeys(NULL, 0);
  rdoc->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, 0);
  isValid = true;
}

gits::RenderDocUtil& gits::RenderDocUtil::GetInstance() {
  if (!instance) {
    instance = new RenderDocUtil();
  }
  return *instance;
}

gits::RenderDocUtil::~RenderDocUtil() {
  if (renderDocLibrary) {
    FreeLibrary(renderDocLibrary);
  }
  delete instance;
  instance = NULL;
}

void gits::RenderDocUtil::StartRecording() {
  if (!isValid) {
    Log(ERR)
        << "RenderDoc API was not properly loaded to GITS. RenderDoc capture will not be dumped.";
    return;
  }
  if (rdoc->IsFrameCapturing()) {
    return;
  }

  SetOutputName();

  rdoc->StartFrameCapture(pRenderDocDevice, nullptr);
}

void gits::RenderDocUtil::StopRecording() {
  if (!isValid) {
    Log(ERR)
        << "RenderDoc API was not properly loaded to GITS. RenderDoc capture will not be dumped.";
    return;
  }
  if (!rdoc->IsFrameCapturing()) {
    return;
  }
  rdoc->EndFrameCapture(pRenderDocDevice, nullptr);
}

void gits::RenderDocUtil::SetOutputName() {
  if (!isValid) {
    Log(ERR)
        << "RenderDoc API was not properly loaded to GITS. RenderDoc capture will not be dumped.";
    return;
  }

  auto& cfg = Config::Get().player;

  if (cfg.renderDoc.enableUI) {
    return;
  }

  std::string baseName;
  std::string fileName;

  if (cfg.renderDoc.frameRecEnabled) {
    baseName = "RenderDoc/frame_";
    if (cfg.renderDoc.continuousCapture) {
      fileName = baseName + cfg.renderDoc.captureRange.StrValue();
    } else {
      fileName = baseName + std::to_string(CGits::Instance().CurrentFrame());
    }
  } else if (cfg.renderDoc.queuesubmitRecEnabled) {
    baseName = "RenderDoc/queuesubmit_";
    if (cfg.renderDoc.continuousCapture) {
      fileName = baseName + cfg.renderDoc.captureRange.StrValue();
    } else {
      fileName = baseName + std::to_string(CGits::Instance().vkCounters.CurrentQueueSubmitCount());
    }
  }

  auto outputName = cfg.streamPath.parent_path() / fileName;

  rdoc->SetCaptureFilePathTemplate(outputName.string().c_str());
}

void gits::RenderDocUtil::LaunchRenderDocUI() {
  if (!isValid) {
    Log(ERR)
        << "RenderDoc API was not properly loaded to GITS. RenderDoc capture will not be dumped.";
    return;
  }
  rdoc->LaunchReplayUI(1, nullptr);
  while (rdoc->IsTargetControlConnected() != 1) {
    sleep_millisec(1000);
  }
}

void gits::RenderDocUtil::SetRenderDocDevice(VkInstance vkInstance) {
  pRenderDocDevice = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(vkInstance);
}

#endif // GITS_PLATFORM_WINDOWS
