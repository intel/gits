// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrMessage.h"

#include <optional>

// cereal is confined to the implementation file, so consumers of the protocol
// never see it.

namespace gits::rtsr {

template <typename T>
Message Serialize(const T& obj, RTSR_MESSAGE_TYPE type);

// Returns nullopt for a truncated, malformed or unexpected payload. RTSR lets
// cereal's exception escape into the receive thread, which terminates the
// process.
template <typename T>
std::optional<T> Deserialize(const Message& message);

} // namespace gits::rtsr
