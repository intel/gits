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
  PrintGitsObjectKey(stream, arg.Key);
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, HandleOutputArgument<T>& arg) {
  if (arg.Value) {
    PrintGitsObjectKey(stream, arg.Key);
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
      PrintGitsObjectKey(stream, arg.Keys[i]);
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
      PrintGitsObjectKey(stream, arg.Keys[i]);
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

inline FastOStream& operator<<(FastOStream& stream, OutputCStringArrayArgument& arg) {
  if (!arg.OwnedStrings.empty()) {
    stream << "[";
    for (size_t i = 0; i < arg.OwnedStrings.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      PrintString(stream, arg.OwnedStrings[i].c_str());
    }
    stream << "]";
    return stream;
  }

  const uint32_t count = arg.CountPtr ? *arg.CountPtr : arg.Size;
  if (!arg.Value || !*arg.Value) {
    if (count == 0) {
      return stream << "[]";
    }
    stream << "[count=" << count << "]";
    return stream;
  }

  stream << "[";
  for (uint32_t i = 0; i < count; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    PrintString(stream, (*arg.Value)[i]);
  }
  stream << "]";
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

// RestoreContentDataCommand repurposes MemoryRegions to carry one restored resource's
// bytes per region, with Region.Offset holding the manifest resource index (not a byte
// offset) and Region.Size the byte count - see RestoreContentDataCommand's declaration in
// commandsCustom.h. Printing it through the generic MemoryRegions{{offset, size}} format
// above reads exactly like a real byte range, which is misleading, so wrap it in this
// distinct view for trace output instead.
struct ContentDataRegionsPrintView {
  MemoryRegions& Regions;
};

inline FastOStream& operator<<(FastOStream& stream, ContentDataRegionsPrintView& arg) {
  stream << "[";
  for (uint32_t i = 0; i < arg.Regions.Regions.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{ResourceIndex=" << arg.Regions.Regions[i].Offset
           << ", Bytes=" << arg.Regions.Regions[i].Size << "}";
  }
  stream << "]";
  return stream;
}

} // namespace vulkan
} // namespace gits
