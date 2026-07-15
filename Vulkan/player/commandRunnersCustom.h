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
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

class StateRestoreBeginRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  StateRestoreBeginCommand m_Command;
};

class StateRestoreEndRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  StateRestoreEndCommand m_Command;
};

class FrameEndRunner : public stream::CommandRunner {
public:
  void Run() override;

private:
  FrameEndCommand m_Command;
};

class MarkerUInt64Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  MarkerUInt64Command m_Command;
};

class CreateWindowMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  CreateWindowMetaCommand m_Command;
};

class UpdateWindowMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  UpdateWindowMetaCommand m_Command;
};

class MappedDataMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  MappedDataMetaCommand m_Command;
};

class RestoreContentManifestRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  RestoreContentManifestCommand m_Command;
};

class RestoreContentDataRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  RestoreContentDataCommand m_Command;
};

} // namespace vulkan
} // namespace gits
