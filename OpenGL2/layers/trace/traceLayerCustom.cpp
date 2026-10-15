// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerAuto.h"

#include "log.h"

#include <stdexcept>

namespace gits::opengl2 {

void TraceLayer::Pre(Command& command) {
  {
    CommandPrinter printer(m_Stream, m_Mutex, command, "Command");
    printer.AddOpaque(static_cast<uintptr_t>(command.GetId()));
    printer.Finish(true, " [error: no trace formatter]");
  }
  LOG_ERROR << "Missing OpenGL trace formatter for command ID "
            << static_cast<uint32_t>(command.GetId());
  throw std::logic_error("Missing OpenGL trace formatter");
}

// TODO: Decide whether FrameEndCommand belongs in layers or needs trace-local counters.

void TraceLayer::Pre(MappedDataMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "MappedDataMeta");
  printer.Add(command.m_Buffer);
  printer.Add(command.m_Offset);
  printer.Add(command.m_Length);
  printer.Add(command.m_Data);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(WindowGeometryMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "WindowGeometryMeta");
  printer.AddOpaque(static_cast<uintptr_t>(command.m_Window.Value));
  printer.Add(command.m_X);
  printer.Add(command.m_Y);
  printer.Add(command.m_Width);
  printer.Add(command.m_Height);
  printer.Add(command.m_Visible);
  printer.Add(command.m_ClosedWindows);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(ClientArraysMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "ClientArraysMeta");
  printer.Add(command.m_ReleasedBlocks);
  printer.Add(command.m_BlockIds);
  printer.Add(command.m_BlockSizes);
  printer.Add(command.m_Data);
  printer.Add(command.m_Kinds);
  printer.Add(command.m_Slots);
  printer.Add(command.m_Sizes);
  printer.AddEnumArray(command.m_Types, GLenumApi::GL, GLenumGroup::Unknown);
  printer.Add(command.m_Strides);
  printer.Add(command.m_Flags);
  printer.Add(command.m_ArrayBlocks);
  printer.Add(command.m_ArrayOffsets);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(UniformArrayMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "UniformArrayMeta");
  printer.Add(command.m_Program);
  if (command.m_Name.HasValue && !command.m_Name.Data.empty()) {
    const auto& text = command.m_Name.Data;
    printer.AddText(std::string_view(text.data(), text.size() - 1));
  } else {
    printer.Add(command.m_Name);
  }
  printer.Add(command.m_Location);
  printer.Add(command.m_ArraySize);
  printer.Add(command.m_ArrayIndex);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(MappedTextureDataMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "MappedTextureDataMeta");
  printer.Add(command.m_Textures);
  printer.Add(command.m_Levels);
  printer.Add(command.m_Strides);
  printer.Add(command.m_Rows);
  printer.Add(command.m_Data);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(PbufferPixelFormatMetaCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "PbufferPixelFormatMeta");
  printer.Add(command.m_Attributes);
  printer.Finish(m_Flush);
}

void TraceLayer::Pre(UnsupportedCommand& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "UnsupportedCommand");
  printer.AddText(command.GetName());
  printer.Finish(m_Flush);
}

} // namespace gits::opengl2
