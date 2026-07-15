// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "commandsCustom.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

class StateRestoreBeginSerializer : public stream::CommandSerializer {
public:
  StateRestoreBeginSerializer(const StateRestoreBeginCommand& command) {}
  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_INIT_START);
  }
};

class StateRestoreEndSerializer : public stream::CommandSerializer {
public:
  StateRestoreEndSerializer(const StateRestoreEndCommand& command) {}

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_INIT_END);
  }
};

class FrameEndSerializer : public stream::CommandSerializer {
public:
  FrameEndSerializer(const FrameEndCommand& command) {}
  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_FRAME_END);
  }
};

class MarkerUInt64Serializer : public stream::CommandSerializer {
public:
  MarkerUInt64Serializer(const MarkerUInt64Command& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }
  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_MARKER_UINT64);
  }
};

class CreateWindowMetaSerializer : public stream::CommandSerializer {
public:
  CreateWindowMetaSerializer(CreateWindowMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_CREATE_WINDOW);
  }
};

class UpdateWindowMetaSerializer : public stream::CommandSerializer {
public:
  UpdateWindowMetaSerializer(UpdateWindowMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_UPDATE_WINDOW);
  }
};

class MappedDataMetaSerializer : public stream::CommandSerializer {
public:
  MappedDataMetaSerializer(MappedDataMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_MAPPED_DATA);
  }
};

class RestoreContentManifestSerializer : public stream::CommandSerializer {
public:
  RestoreContentManifestSerializer(RestoreContentManifestCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_RESTORE_CONTENT_MANIFEST);
  }
};

class RestoreContentDataSerializer : public stream::CommandSerializer {
public:
  RestoreContentDataSerializer(RestoreContentDataCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_RESTORE_CONTENT_DATA);
  }
};

} // namespace vulkan
} // namespace gits
