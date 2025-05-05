// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configurationAuto.h"

namespace gits {

template <>
void DeriveConfigData<Configuration>(Configuration& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenGL>(Configuration::OpenGL& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::Vulkan>(Configuration::Vulkan& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::DirectX>(Configuration::DirectX& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenCL>(Configuration::OpenCL& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::LevelZero>(Configuration::LevelZero& obj,
                                                Configuration& config);

template <>
void DeriveConfigData<Configuration::Common>(Configuration::Common& obj, Configuration& config);

template <>
void DeriveConfigData<Configuration::Common::Shared>(Configuration::Common::Shared& obj,
                                                     Configuration& config);

template <>
void DeriveConfigData<Configuration::Common::Player>(Configuration::Common::Player& obj,
                                                     Configuration& config);

template <>
void DeriveConfigData<Configuration::Common::Recorder>(Configuration::Common::Recorder& obj,
                                                       Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenGL::Shared>(Configuration::OpenGL::Shared& obj,
                                                     Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenGL::Player>(Configuration::OpenGL::Player& obj,
                                                     Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenGL::Recorder>(Configuration::OpenGL::Recorder& obj,
                                                       Configuration& config);

template <>
void DeriveConfigData<Configuration::Vulkan::Player>(Configuration::Vulkan::Player& obj,
                                                     Configuration& config);

template <>
void DeriveConfigData<Configuration::Vulkan::Recorder>(Configuration::Vulkan::Recorder& obj,
                                                       Configuration& config);

template <>
void DeriveConfigData<Configuration::OpenCL::Recorder>(Configuration::OpenCL::Recorder& obj,
                                                       Configuration& config);

template <>
void DeriveConfigData<Configuration::LevelZero::Player>(Configuration::LevelZero::Player& obj,
                                                        Configuration& config);

template <>
void DeriveConfigData<Configuration::LevelZero::Recorder>(Configuration::LevelZero::Recorder& obj,
                                                          Configuration& config);

} // namespace gits
