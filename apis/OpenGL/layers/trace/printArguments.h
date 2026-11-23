// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "fastOStream.h"
#include "glEnumsAuto.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace gits::opengl {

inline constexpr size_t kMaxPrintedArrayElements = 64;
inline constexpr size_t kMaxPrintedTextBytes = 256;

inline void PrintGLBoolean(FastOStream& stream, GLboolean value) {
  stream << (value ? "GL_TRUE" : "GL_FALSE");
}

inline void PrintOpaque(FastOStream& stream, uintptr_t value) {
  stream << "0x";
  PrintHex(stream, value);
}

inline void PrintText(FastOStream& stream, std::string_view value, size_t length) {
  constexpr char digits[] = "0123456789ABCDEF";
  stream << '"';
  for (size_t index = 0; index < length; ++index) {
    const unsigned char byte = static_cast<unsigned char>(value[index]);
    if (byte == '"' || byte == '\\') {
      stream << '\\' << static_cast<char>(byte);
    } else if (byte == '\n') {
      stream << "\\n";
    } else if (byte == '\r') {
      stream << "\\r";
    } else if (byte == '\t') {
      stream << "\\t";
    } else if (byte >= 32 && byte < 127) {
      stream << static_cast<char>(byte);
    } else {
      stream << "\\x" << digits[byte >> 4] << digits[byte & 15];
    }
  }
  stream << '"';
  if (length < value.size()) {
    stream << "...(+" << value.size() - length << " bytes)";
  }
}

template <typename T>
void PrintArgument(FastOStream& stream, const Argument<T>& argument) {
  if constexpr (std::is_same_v<T, bool>) {
    stream << (argument.Value ? "true" : "false");
  } else if constexpr (std::is_pointer_v<T>) {
    PrintOpaque(stream, reinterpret_cast<uintptr_t>(argument.Value));
  } else {
    stream << argument.Value;
  }
}

template <typename T>
void PrintArgument(FastOStream& stream, const HandleArgument<T>& argument) {
  stream << "K" << argument.Key;
  if (argument.Key == 0) {
    stream << "=null";
  }
}

template <typename T>
void PrintArgument(FastOStream& stream, const HandleArrayArgument<T>& argument) {
  if (!argument.HasValue) {
    stream << "nullptr";
    return;
  }
  stream << '[';
  const size_t count = std::min(argument.Key.size(), kMaxPrintedArrayElements);
  for (size_t index = 0; index < count; ++index) {
    if (index) {
      stream << ", ";
    }
    stream << "K" << argument.Key[index];
  }
  if (count < argument.Key.size()) {
    stream << "...(+" << argument.Key.size() - count << ")";
  }
  stream << ']';
}

template <typename T>
void PrintArgument(FastOStream& stream, const ArrayArgument<T>& argument) {
  if (!argument.HasValue) {
    stream << "nullptr";
    return;
  }
  if constexpr (std::is_same_v<T, uint8_t>) {
    stream << "data[count=" << argument.Data.size() << "]";
  } else if constexpr (std::is_same_v<T, GLchar>) {
    const auto* bytes = reinterpret_cast<const char*>(argument.Data.data());
    PrintText(stream, std::string_view(bytes ? bytes : "", argument.Data.size()),
              std::min(argument.Data.size(), kMaxPrintedTextBytes));
  } else {
    stream << '[';
    const size_t count = std::min(argument.Data.size(), kMaxPrintedArrayElements);
    for (size_t index = 0; index < count; ++index) {
      if (index) {
        stream << ", ";
      }
      if constexpr (std::is_same_v<T, uintptr_t>) {
        PrintOpaque(stream, argument.Data[index]);
      } else {
        stream << argument.Data[index];
      }
    }
    if (count < argument.Data.size()) {
      stream << "...(+" << argument.Data.size() - count << ")";
    }
    stream << ']';
  }
}

inline void PrintArgument(FastOStream& stream,
                          const ArrayArgument<PIXELFORMATDESCRIPTOR>& argument) {
  if (!argument.HasValue) {
    stream << "nullptr";
    return;
  }
  stream << '[';
  const size_t count = std::min(argument.Data.size(), kMaxPrintedArrayElements);
  for (size_t index = 0; index < count; ++index) {
    if (index) {
      stream << ", ";
    }
    const auto& value = argument.Data[index];
    stream << "{size=" << value.nSize << ", version=" << value.nVersion << ", flags=";
    PrintOpaque(stream, value.dwFlags);
    stream << ", pixelType=" << unsigned(value.iPixelType)
           << ", color=" << unsigned(value.cColorBits) << ", red=" << unsigned(value.cRedBits)
           << ':' << unsigned(value.cRedShift) << ", green=" << unsigned(value.cGreenBits) << ':'
           << unsigned(value.cGreenShift) << ", blue=" << unsigned(value.cBlueBits) << ':'
           << unsigned(value.cBlueShift) << ", alpha=" << unsigned(value.cAlphaBits) << ':'
           << unsigned(value.cAlphaShift) << ", accum=" << unsigned(value.cAccumBits) << ':'
           << unsigned(value.cAccumRedBits) << ':' << unsigned(value.cAccumGreenBits) << ':'
           << unsigned(value.cAccumBlueBits) << ':' << unsigned(value.cAccumAlphaBits)
           << ", depth=" << unsigned(value.cDepthBits)
           << ", stencil=" << unsigned(value.cStencilBits)
           << ", aux=" << unsigned(value.cAuxBuffers)
           << ", layerType=" << unsigned(value.iLayerType)
           << ", reserved=" << unsigned(value.bReserved) << ", layerMask=";
    PrintOpaque(stream, value.dwLayerMask);
    stream << ", visibleMask=";
    PrintOpaque(stream, value.dwVisibleMask);
    stream << ", damageMask=";
    PrintOpaque(stream, value.dwDamageMask);
    stream << '}';
  }
  if (count < argument.Data.size()) {
    stream << "...(+" << argument.Data.size() - count << ")";
  }
  stream << ']';
}

template <typename T>
void PrintArgument(FastOStream& stream, const ArrayOfArrays<T>& argument) {
  if (!argument.HasValue) {
    stream << "nullptr";
    return;
  }
  stream << '[';
  const size_t count = std::min(argument.Value.size(), kMaxPrintedArrayElements);
  size_t remaining = kMaxPrintedTextBytes;
  for (size_t index = 0; index < count; ++index) {
    if (index) {
      stream << ", ";
    }
    const auto& bytes = argument.Value[index];
    const bool isNullTerminated = argument.Lengths[index] < 0 && !bytes.empty();
    const size_t length = isNullTerminated ? bytes.size() - 1 : bytes.size();
    const std::string_view text(bytes.empty() ? "" : bytes.data(), length);
    auto preview = text.substr(0, remaining);
    while (!preview.empty() &&
           std::string_view(" \t\n\r\v\f").find(preview.back()) != std::string_view::npos) {
      preview.remove_suffix(1);
    }
    PrintText(stream, text, preview.size());
    remaining -= preview.size();
    stream << "(length=" << argument.Lengths[index] << ')';
  }
  if (count < argument.Value.size()) {
    stream << "...(+" << argument.Value.size() - count << ")";
  }
  stream << ']';
}

inline void PrintEnum(FastOStream& stream, GLenum value, GLenumApi api, GLenumGroup group) {
  stream << GetGLenumString(api, value, group);
}

template <typename T>
void PrintEnumArray(FastOStream& stream,
                    const ArrayArgument<T>& argument,
                    GLenumApi api,
                    GLenumGroup group) {
  if (!argument.HasValue) {
    stream << "nullptr";
    return;
  }
  stream << '[';
  const size_t count = std::min(argument.Data.size(), kMaxPrintedArrayElements);
  for (size_t index = 0; index < count; ++index) {
    if (index) {
      stream << ", ";
    }
    PrintEnum(stream, argument.Data[index], api, group);
  }
  if (count < argument.Data.size()) {
    stream << "...(+" << argument.Data.size() - count << ")";
  }
  stream << ']';
}

} // namespace gits::opengl
