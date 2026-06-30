// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "printCustom.h"
#include "printEnumsAuto.h"
#include "printStructuresAuto.h"

#include <iostream>

namespace gits {
namespace vulkan {

template <typename T>
FastOStream& operator<<(FastOStream& stream, PointerArgument<T>& arg) {
  if (arg.Value) {
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
      return stream << "{" << *arg.Value << "}";
    } else {
      return stream << const_cast<const T*>(arg.Value);
    }
  }
  return stream << "nullptr";
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, HandleArgument<T>& arg) {
  stream << "O" << arg.Key;
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, HandleOutputArgument<T>& arg) {
  if (arg.Value) {
    stream << "O" << arg.Key;
  } else {
    stream << "nullptr";
  }
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, HandleArrayArgument<T>& arg) {
  if (arg.Value) {
    stream << "{";
    for (uint32_t i = 0; i < arg.Keys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "O" << arg.Keys[i];
    }
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, HandleArrayOutputArgument<T>& arg) {
  if (arg.Value) {
    stream << "{";
    for (uint32_t i = 0; i < arg.Keys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "O" << arg.Keys[i];
    }
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, ArrayOfArrays<T>& arg) {
  if (arg.Value) {
    stream << "ArrayOfArrays[" << arg.Size << "]";
  } else {
    stream << "nullptr";
  }
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, OpaquePointerArgument<T>& arg) {
  if (arg.Value) {
    stream << static_cast<const void*>(arg.Value);
  } else {
    stream << "nullptr";
  }
  return stream;
}

template <template <typename> typename Arg, typename T>
FastOStream& operator<<(FastOStream& stream, Arg<T>& arg) {
  stream << arg.Value;
  return stream;
}

template <typename T, uint32_t N>
FastOStream& operator<<(FastOStream& stream, StaticArrayArgument<T, N>& arg) {
  stream << "[";
  for (uint32_t i = 0; i < N; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << arg.Value[i];
  }
  stream << "]";
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, ArrayArgument<T>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (uint32_t i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << arg.Value[i];
  }
  stream << "]";
  return stream;
}

inline FastOStream& operator<<(FastOStream& stream, BufferArgument& arg) {
  if (arg.Value) {
    stream << arg.Value << "[" << arg.Size << "]";
  } else {
    stream << "nullptr";
  }
  return stream;
}

inline FastOStream& operator<<(FastOStream& stream, OpaqueBufferArgument& arg) {
  if (arg.Value) {
    stream << arg.Value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

inline FastOStream& operator<<(FastOStream& stream, BufferOutputArgument& arg) {
  if (arg.Value) {
    stream << static_cast<const void*>(arg.Value);
  } else {
    stream << "nullptr";
  }
  return stream;
}

inline FastOStream& operator<<(FastOStream& stream, DescriptorTemplateDataArgument& arg) {
  if (arg.Value) {
    stream << arg.Value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

inline FastOStream& operator<<(FastOStream& stream, MemoryRegions& arg) {
  stream << "MemoryRegions{";
  for (uint32_t i = 0; i < arg.Regions.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{" << arg.Regions[i].Offset << ", " << arg.Regions[i].Size << "}";
  }
  stream << "}";
  return stream;
}

} // namespace vulkan
} // namespace gits
