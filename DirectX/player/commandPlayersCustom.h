// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandPlayer.h"
#include "commandsCustom.h"
#include "commandDecodersCustom.h"

#include <vector>

namespace gits {
namespace DirectX {

#pragma region IUnknown

class IUnknownQueryInterfacePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_QUERYINTERFACE);
  }
  const char* Name() const override {
    return "IUnknown::QueryInterface";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  IUnknownQueryInterfaceCommand command;
};

class IUnknownAddRefPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_ADDREF);
  }
  const char* Name() const override {
    return "IUnknown::AddRef";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  IUnknownAddRefCommand command;
};

class IUnknownReleasePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_RELEASE);
  }
  const char* Name() const override {
    return "IUnknown::Release";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  IUnknownReleaseCommand command;
};

#pragma endregion

#pragma region MetaCommands

class CreateWindowMetaPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_CREATE_WINDOW);
  }
  const char* Name() const override {
    return "CreateWindowMetaCommand";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  CreateWindowMetaCommand command;
};

class MappedDataMetaPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_MAPPED_DATA);
  }
  const char* Name() const override {
    return "MappedDataMetaCommand";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  MappedDataMetaCommand command;
};

class CreateHeapAllocationMetaPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_CREATE_HEAP_ALLOCATION);
  }
  const char* Name() const override {
    return "CreateHeapAllocationMetaCommand";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  CreateHeapAllocationMetaCommand command;
};

class WaitForFenceSignaledPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_WAIT_FOR_FENCE_SIGNALED);
  }
  const char* Name() const override {
    return "WaitForFenceSignaledCommand";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  WaitForFenceSignaledCommand command;
};

#pragma endregion

#pragma region INTCExtension

class INTC_D3D12_GetSupportedVersionsPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS);
  }
  const char* Name() const override {
    return "INTC_D3D12_GetSupportedVersions";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_GetSupportedVersionsCommand command;
};

class INTC_D3D12_CreateDeviceExtensionContextPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateDeviceExtensionContext";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateDeviceExtensionContextCommand command;
};

class INTC_D3D12_CreateDeviceExtensionContext1Player : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateDeviceExtensionContext1";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateDeviceExtensionContext1Command command;
};

class INTC_D3D12_SetApplicationInfoPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_SETAPPLICATIONINFO);
  }
  const char* Name() const override {
    return "INTC_D3D12_SetApplicationInfo";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_SetApplicationInfoCommand command;
};

class INTC_DestroyDeviceExtensionContextPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT);
  }
  const char* Name() const override {
    return "INTC_DestroyDeviceExtensionContext";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_DestroyDeviceExtensionContextCommand command;
};

class INTC_D3D12_CheckFeatureSupportPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CHECKFEATURESUPPORT);
  }
  const char* Name() const override {
    return "INTC_D3D12_CheckFeatureSupport";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CheckFeatureSupportCommand command;
};

class INTC_D3D12_CreateCommandQueuePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMANDQUEUE);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateCommandQueue";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateCommandQueueCommand command;
};

class INTC_D3D12_CreateReservedResourcePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateReservedResource";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateReservedResourceCommand command;
};

class INTC_D3D12_SetFeatureSupportPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_SETFEATURESUPPORT);
  }
  const char* Name() const override {
    return "INTC_D3D12_SetFeatureSupport";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_SetFeatureSupportCommand command;
};

class INTC_D3D12_GetResourceAllocationInfoPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO);
  }
  const char* Name() const override {
    return "INTC_D3D12_GetResourceAllocationInfo";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_GetResourceAllocationInfoCommand command;
};

class INTC_D3D12_CreateComputePipelineStatePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateComputePipelineState";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateComputePipelineStateCommand command;
};

class INTC_D3D12_CreatePlacedResourcePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreatePlacedResource";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreatePlacedResourceCommand command;
};

class INTC_D3D12_CreateCommittedResourcePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateCommittedResource";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateCommittedResourceCommand command;
};

class INTC_D3D12_CreateHeapPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEHEAP);
  }
  const char* Name() const override {
    return "INTC_D3D12_CreateHeap";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  INTC_D3D12_CreateHeapCommand command;
};

class NvAPI_InitializePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_INITIALIZE);
  }
  const char* Name() const override {
    return "NvAPI_Initialize";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_InitializeCommand command;
};

class NvAPI_UnloadPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_UNLOAD);
  }
  const char* Name() const override {
    return "NvAPI_Unload";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_UnloadCommand command;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpacePlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE);
  }
  const char* Name() const override {
    return "NvAPI_D3D12_SetNvShaderExtnSlotSpace";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand command;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD);
  }
  const char* Name() const override {
    return "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand command;
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX);
  }
  const char* Name() const override {
    return "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand command;
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY);
  }
  const char* Name() const override {
    return "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand command;
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(
        CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION);
  }
  const char* Name() const override {
    return "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand command;
};

#pragma endregion

} // namespace DirectX
} // namespace gits
