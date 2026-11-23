// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedTextureLayer.h"

#include "captureManager.h"
#include "mappedTextureService.h"

namespace gits::opengl {

void MappedTextureLayer::Pre(wglSwapBuffersCommand& command) {
  MappedTextureService::Get().CaptureOnSwap(CaptureManager::Get().TakePendingMappedTextureKey());
}

void MappedTextureLayer::Pre(wglSwapLayerBuffersCommand& command) {
  MappedTextureService::Get().CaptureOnSwap(CaptureManager::Get().TakePendingMappedTextureKey());
}

void MappedTextureLayer::Pre(glXSwapBuffersCommand& command) {
  MappedTextureService::Get().CaptureOnSwap(CaptureManager::Get().TakePendingMappedTextureKey());
}

void MappedTextureLayer::Pre(glDeleteTexturesCommand& command) {
  MappedTextureService::Get().OnDelete(static_cast<GLsizei>(command.m_textures.Value.size()),
                                       command.m_textures.Value.data());
}

void MappedTextureLayer::Pre(glDeleteTexturesEXTCommand& command) {
  MappedTextureService::Get().OnDelete(static_cast<GLsizei>(command.m_textures.Value.size()),
                                       command.m_textures.Value.data());
}

} // namespace gits::opengl
