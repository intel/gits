// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureDumpLayer.h"
#include "rootSignatureDump.h"
#include "gits.h"
#include "to_string/toStr.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

RootSignatureDumpLayer::RootSignatureDumpLayer()
    : Layer("RootSignatureDump"),
      rootSignatureKeys_(Configurator::Get().directx.features.rootSignatureDump.rootSignatureKeys) {
  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "rootSignatures"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  dumpPath_ = dumpPath;
}

void RootSignatureDumpLayer::post(ID3D12DeviceCreateRootSignatureCommand& command) {
  if (rootSignatureKeys_.empty() || rootSignatureKeys_.contains(command.ppvRootSignature_.key)) {
    std::wstring dumpName =
        dumpPath_ + L"/root_signature_O" + keyToWStr(command.ppvRootSignature_.key) + L".txt";
    rootSignatureDump_.DeserializeRootSignature(command.pBlobWithRootSignature_.value,
                                                command.blobLengthInBytes_.value, dumpName);
  }
}

} // namespace DirectX
} // namespace gits
