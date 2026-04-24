// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "commandsCustom.h"
#include "commandDecodersCustom.h"

#include <vector>

namespace gits {
namespace DirectX {

#pragma region Common

class StateRestoreBeginRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  StateRestoreBeginCommand command;
};

class StateRestoreEndRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  StateRestoreEndCommand command;
};

class FrameEndRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  FrameEndCommand command;
};

class MarkerUInt64Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  MarkerUInt64Command command;
};

#pragma endregion

#pragma region IUnknown

class IUnknownQueryInterfaceRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  IUnknownQueryInterfaceCommand command;
};

class IUnknownAddRefRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  IUnknownAddRefCommand command;
};

class IUnknownReleaseRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  IUnknownReleaseCommand command;
};

#pragma endregion

#pragma region MetaCommands

class CreateWindowMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  CreateWindowMetaCommand command;
};

class MappedDataMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  MappedDataMetaCommand command;
};

class CreateHeapAllocationMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  CreateHeapAllocationMetaCommand command;
};

class WaitForFenceSignaledRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  WaitForFenceSignaledCommand command;
};

class DllContainerMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  DllContainerMetaCommand command;
};

#pragma endregion

#pragma region INTCExtension

class INTC_D3D12_GetSupportedVersionsRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_GetSupportedVersionsCommand command;
};

class INTC_D3D12_CreateDeviceExtensionContextRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateDeviceExtensionContextCommand command;
};

class INTC_D3D12_CreateDeviceExtensionContext1Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateDeviceExtensionContext1Command command;
};

class INTC_D3D12_SetApplicationInfoRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_SetApplicationInfoCommand command;
};

class INTC_DestroyDeviceExtensionContextRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_DestroyDeviceExtensionContextCommand command;
};

class INTC_D3D12_CheckFeatureSupportRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CheckFeatureSupportCommand command;
};

class INTC_D3D12_CreateCommandQueueRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateCommandQueueCommand command;
};

class INTC_D3D12_CreateReservedResourceRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateReservedResourceCommand command;
};

class INTC_D3D12_SetFeatureSupportRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_SetFeatureSupportCommand command;
};

class INTC_D3D12_GetResourceAllocationInfoRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_GetResourceAllocationInfoCommand command;
};

class INTC_D3D12_CreateComputePipelineStateRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateComputePipelineStateCommand command;
};

class INTC_D3D12_CreatePlacedResourceRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreatePlacedResourceCommand command;
};

class INTC_D3D12_CreateCommittedResourceRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateCommittedResourceCommand command;
};

class INTC_D3D12_CreateHeapRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  INTC_D3D12_CreateHeapCommand command;
};

class NvAPI_InitializeRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_InitializeCommand command;
};

class NvAPI_UnloadRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_UnloadCommand command;
};

class NvAPI_D3D12_SetCreatePipelineStateOptionsRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_SetCreatePipelineStateOptionsCommand command;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand command;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand command;
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand command;
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand command;
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationRunner
    : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand command;
};

#pragma endregion

} // namespace DirectX
} // namespace gits
