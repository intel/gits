// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandWriter.h"
#include "commandsCustom.h"
#include "commandIdsAuto.h"
#include "commandEncodersCustom.h"

namespace gits {
namespace DirectX {

class IUnknownQueryInterfaceWriter : public CommandWriter {
public:
  IUnknownQueryInterfaceWriter(IUnknownQueryInterfaceCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_QUERYINTERFACE);
  }
};

class IUnknownAddRefWriter : public CommandWriter {
public:
  IUnknownAddRefWriter(IUnknownAddRefCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_ADDREF);
  }
};

class IUnknownReleaseWriter : public CommandWriter {
public:
  IUnknownReleaseWriter(IUnknownReleaseCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_RELEASE);
  }
};

class CreateWindowMetaWriter : public CommandWriter {
public:
  CreateWindowMetaWriter(CreateWindowMetaCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_CREATE_WINDOW);
  }
};

class MappedDataMetaWriter : public CommandWriter {
public:
  MappedDataMetaWriter(MappedDataMetaCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_MAPPED_DATA);
  }
};

class CreateHeapAllocationMetaWriter : public CommandWriter {
public:
  CreateHeapAllocationMetaWriter(CreateHeapAllocationMetaCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_CREATE_HEAP_ALLOCATION);
  }
};

class WaitForFenceSignaledWriter : public CommandWriter {
public:
  WaitForFenceSignaledWriter(WaitForFenceSignaledCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_WAIT_FOR_FENCE_SIGNALED);
  }
};

class INTC_D3D12_GetSupportedVersionsWriter : public CommandWriter {
public:
  INTC_D3D12_GetSupportedVersionsWriter(INTC_D3D12_GetSupportedVersionsCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS);
  }
};

class INTC_D3D12_CreateDeviceExtensionContextWriter : public CommandWriter {
public:
  INTC_D3D12_CreateDeviceExtensionContextWriter(
      INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT);
  }
};

class INTC_D3D12_CreateDeviceExtensionContext1Writer : public CommandWriter {
public:
  INTC_D3D12_CreateDeviceExtensionContext1Writer(
      INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1);
  }
};

class INTC_DestroyDeviceExtensionContextWriter : public CommandWriter {
public:
  INTC_DestroyDeviceExtensionContextWriter(INTC_DestroyDeviceExtensionContextCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT);
  }
};

class INTC_D3D12_CheckFeatureSupportWriter : public CommandWriter {
public:
  INTC_D3D12_CheckFeatureSupportWriter(INTC_D3D12_CheckFeatureSupportCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CHECKFEATURESUPPORT);
  }
};

class INTC_D3D12_CreateCommandQueueWriter : public CommandWriter {
public:
  INTC_D3D12_CreateCommandQueueWriter(INTC_D3D12_CreateCommandQueueCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMANDQUEUE);
  }
};

class INTC_D3D12_CreateReservedResourceWriter : public CommandWriter {
public:
  INTC_D3D12_CreateReservedResourceWriter(INTC_D3D12_CreateReservedResourceCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE);
  }
};

class INTC_D3D12_SetFeatureSupportWriter : public CommandWriter {
public:
  INTC_D3D12_SetFeatureSupportWriter(INTC_D3D12_SetFeatureSupportCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_SETFEATURESUPPORT);
  }
};

class INTC_D3D12_GetResourceAllocationInfoWriter : public CommandWriter {
public:
  INTC_D3D12_GetResourceAllocationInfoWriter(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO);
  }
};

class INTC_D3D12_CreateComputePipelineStateWriter : public CommandWriter {
public:
  INTC_D3D12_CreateComputePipelineStateWriter(
      INTC_D3D12_CreateComputePipelineStateCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE);
  }
};

class INTC_D3D12_CreatePlacedResourceWriter : public CommandWriter {
public:
  INTC_D3D12_CreatePlacedResourceWriter(INTC_D3D12_CreatePlacedResourceCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE);
  }
};

class INTC_D3D12_CreateCommittedResourceWriter : public CommandWriter {
public:
  INTC_D3D12_CreateCommittedResourceWriter(INTC_D3D12_CreateCommittedResourceCommand& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE);
  }
};

} // namespace DirectX
} // namespace gits
