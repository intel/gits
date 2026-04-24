// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureDumpLayer.h"
#include "rootSignatureDump.h"
#include "to_string/toStr.h"

#include "configurationLib.h"

namespace gits {
namespace DirectX {

RootSignatureDumpLayer::RootSignatureDumpLayer()
    : Layer("RootSignatureDump"),
      m_RootSignatureKeys(
          Configurator::Get().directx.features.rootSignatureDump.rootSignatureKeys) {
  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "rootSignatures"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  m_DumpPath = dumpPath;
}

void RootSignatureDumpLayer::Post(ID3D12DeviceCreateRootSignatureCommand& command) {
  if (m_RootSignatureKeys.Empty() || m_RootSignatureKeys.Contains(command.m_ppvRootSignature.Key)) {
    std::wstring dumpName =
        m_DumpPath + L"/root_signature_O" + keyToWStr(command.m_ppvRootSignature.Key) + L".txt";
    m_RootSignatureDump.DeserializeRootSignature(command.m_pBlobWithRootSignature.Value,
                                                 command.m_blobLengthInBytes.Value, dumpName);
  }
}

} // namespace DirectX
} // namespace gits
