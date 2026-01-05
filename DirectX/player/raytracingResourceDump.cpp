// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingResourceDump.h"
#include "gits.h"

#include <fstream>

namespace gits {
namespace DirectX {

RaytracingResourceDump::~RaytracingResourceDump() {
  waitUntilDumped();
}

void RaytracingResourceDump::dumpResource(ID3D12GraphicsCommandList* commandList,
                                          ID3D12Resource* resource,
                                          unsigned offset,
                                          unsigned size,
                                          unsigned stride,
                                          D3D12_RESOURCE_STATES resourceState,
                                          const std::wstring& dumpName,
                                          ResourceType resourceType,
                                          bool fromCapture) {

  RaytracingDumpInfo* dumpInfo = new RaytracingDumpInfo();
  dumpInfo->resourceType = resourceType;
  dumpInfo->offset = offset;
  dumpInfo->size = size;
  dumpInfo->stride = stride;
  dumpInfo->dumpName = dumpName;
  dumpInfo->fromCapture = fromCapture;

  stageResource(commandList, resource, resourceState, *dumpInfo);
}

void RaytracingResourceDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  RaytracingDumpInfo& info = static_cast<RaytracingDumpInfo&>(dumpInfo);
  if (info.resourceType == Instances) {
    dumpInstancesBuffer(info, data);
  } else if (info.resourceType == InstancesArrayOfPointers) {
    dumpInstancesArrayOfPointersBuffer(info, data);
  } else if (info.resourceType == BindingTable) {
    dumpBindingTableBuffer(info, data);
  }
}

void RaytracingResourceDump::dumpInstancesBuffer(RaytracingDumpInfo& dumpInfo, void* data) {
  std::ofstream stream(dumpInfo.dumpName);
  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  unsigned count = dumpInfo.size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

  for (unsigned i = 0; i < count; ++i) {
    stream << "INSTANCE " << i + 1 << "\n";

    stream << "  Transform {";
    for (unsigned j = 0; j < 3; ++j) {
      if (j > 0) {
        stream << ", ";
      }
      stream << "{";
      for (unsigned k = 0; k < 3; ++k) {
        if (k > 0) {
          stream << ", ";
        }
        stream << instances[i].Transform[j][k];
      }
      stream << "}";
    }
    stream << "}\n";

    stream << "  InstanceID " << instances[i].InstanceID << "\n";
    stream << "  InstanceMask " << instances[i].InstanceMask << "\n";
    stream << "  InstanceContributionToHitGroupIndex "
           << instances[i].InstanceContributionToHitGroupIndex << "\n";
    stream << "  Flags " << instances[i].Flags << "\n";

    stream << "  AccelerationStructure " << std::hex << std::setw(16) << std::setfill('0')
           << instances[i].AccelerationStructure << std::dec;
    CapturePlayerGpuAddressService::ResourceInfo* info{};
    unsigned offset{};
    if (dumpInfo.fromCapture) {
      info = addressService_.getResourceInfoByCaptureAddress(instances[i].AccelerationStructure);
    } else {
      info = addressService_.getResourceInfoByPlayerAddress(instances[i].AccelerationStructure);
    }
    if (!info) {
      stream << " NOT FOUND\n";
    } else {
      if (dumpInfo.fromCapture) {
        offset = instances[i].AccelerationStructure - info->captureStart;
      } else {
        offset = instances[i].AccelerationStructure - info->playerStart;
      }
      stream << " blas O" << info->key << " offset " << offset << "\n";
    }
  }
}

void RaytracingResourceDump::dumpInstancesArrayOfPointersBuffer(RaytracingDumpInfo& dumpInfo,
                                                                void* data) {
  std::ofstream stream(dumpInfo.dumpName);
  D3D12_GPU_VIRTUAL_ADDRESS* addresses = static_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(data);
  unsigned count = dumpInfo.size / sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
  for (unsigned i = 0; i < count; ++i) {
    printGpuAddress(stream, addresses[i], dumpInfo.fromCapture);
    stream << "\n";
  }
}

void RaytracingResourceDump::dumpBindingTableBuffer(RaytracingDumpInfo& dumpInfo, void* data) {
  std::ofstream stream(dumpInfo.dumpName);
  unsigned recordCount = dumpInfo.size / dumpInfo.stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    stream << "BINDING TABLE " << recordIndex + 1 << " size " << dumpInfo.stride << "\n";

    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * dumpInfo.stride;
    CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
    memcpy(shaderIdentifier.data(), p, shaderIdentifier.size());
    stream << std::hex;
    for (uint8_t byte : shaderIdentifier) {
      stream << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    stream << " ";
    std::wstring wstr;
    if (dumpInfo.fromCapture) {
      wstr = shaderIdentifierService_.getExportNameByCaptureIdentifier(shaderIdentifier);
    } else {
      wstr = shaderIdentifierService_.getExportNameByPlayerIdentifier(shaderIdentifier);
    }
    std::string str(wstr.begin(), wstr.end());
    stream << std::dec << str << "\n";
    p += shaderIdentifier.size();

    unsigned count = (dumpInfo.stride - shaderIdentifier.size()) / sizeof(UINT64);
    for (unsigned i = 0; i < count; ++i) {
      UINT64* address = reinterpret_cast<UINT64*>(p + sizeof(UINT64) * i);
      stream << std::hex << std::setw(16) << std::setfill('0') << *address << std::dec;

      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo{};
      if (dumpInfo.fromCapture) {
        resourceInfo = addressService_.getResourceInfoByCaptureAddress(*address);
      } else {
        resourceInfo = addressService_.getResourceInfoByPlayerAddress(*address);
      }
      if (resourceInfo) {
        unsigned offset{};
        if (dumpInfo.fromCapture) {
          offset = *address - resourceInfo->captureStart;
        } else {
          offset = *address - resourceInfo->playerStart;
        }
        stream << " resource O" << resourceInfo->key << " offset " << offset;
      }

      CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo{};
      unsigned stride{};
      if (dumpInfo.fromCapture) {
        heapInfo = descriptorHandleService_.getViewDescriptorHeapInfoByCaptureHandle(*address);
        stride = descriptorHandleService_.viewHeapIncrement();
        if (!heapInfo) {
          heapInfo = descriptorHandleService_.getSamplerDescriptorHeapInfoByCaptureHandle(*address);
          stride = descriptorHandleService_.samplerHeapIncrement();
        }
      } else {
        heapInfo = descriptorHandleService_.getViewDescriptorHeapInfoByPlayerHandle(*address);
        stride = descriptorHandleService_.viewHeapIncrement();
        if (!heapInfo) {
          heapInfo = descriptorHandleService_.getSamplerDescriptorHeapInfoByPlayerHandle(*address);
          stride = descriptorHandleService_.samplerHeapIncrement();
        }
      }
      if (heapInfo) {
        unsigned index{};
        if (dumpInfo.fromCapture) {
          index = (*address - heapInfo->captureStart) / stride;
        } else {
          index = (*address - heapInfo->playerStart) / stride;
        }
        stream << " descriptor heap O" << heapInfo->key << " index " << index;
      }

      stream << "\n";
    }
  }
}

void RaytracingResourceDump::printGpuAddress(std::ostream& stream,
                                             D3D12_GPU_VIRTUAL_ADDRESS address,
                                             bool fromCapture) {
  std::ios state(nullptr);
  state.copyfmt(stream);

  stream << "{0x" << std::hex << std::setw(16) << std::setfill('0') << address << std::dec;
  if (address) {
    CapturePlayerGpuAddressService::ResourceInfo* info{};
    unsigned offset{};
    if (fromCapture) {
      info = addressService_.getResourceInfoByCaptureAddress(address);
    } else {
      info = addressService_.getResourceInfoByPlayerAddress(address);
    }
    if (!info) {
      stream << ", NOT FOUND";
    } else {
      if (fromCapture) {
        offset = address - info->captureStart;
      } else {
        offset = address - info->playerStart;
      }
      stream << ", key O" << info->key << ", offset " << offset;
    }
  }
  stream << "}";

  stream.copyfmt(state);
}

} // namespace DirectX
} // namespace gits
