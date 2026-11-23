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

namespace gits::opengl {

namespace {

[[noreturn]] void PrintMissingFormatter(FastOStream& stream, std::mutex& mutex, Command& command) {
  {
    CommandPrinter printer(stream, mutex, command, "Command");
    printer.AddOpaque(static_cast<uintptr_t>(command.GetId()));
    printer.Finish(true, " [error: no trace formatter]");
  }
  LOG_ERROR << "Missing OpenGL trace formatter for command ID "
            << static_cast<uint32_t>(command.GetId());
  throw std::logic_error("Missing OpenGL trace formatter");
}

// TODO: Decide whether FrameEndCommand belongs in layers or needs trace-local counters.

void PrintMappedDataMeta(FastOStream& stream,
                         std::mutex& mutex,
                         bool flush,
                         MappedDataMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "MappedDataMeta");
  printer.Add(command.m_Buffer);
  printer.Add(command.m_Offset);
  printer.Add(command.m_Length);
  printer.AddEnum(command.m_Access.Value, GLenumApi::GL, GLenumGroup::MapBufferAccessMask);
  printer.Add(command.m_Spans);
  printer.Add(command.m_Data);
  printer.Finish(flush);
}

void PrintWindowGeometryMeta(FastOStream& stream,
                             std::mutex& mutex,
                             bool flush,
                             WindowGeometryMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "WindowGeometryMeta");
  printer.Add(command.m_CallSamples);
  printer.Add(command.m_Windows);
  printer.Add(command.m_X);
  printer.Add(command.m_Y);
  printer.Add(command.m_Width);
  printer.Add(command.m_Height);
  printer.Add(command.m_Visible);
  printer.Add(command.m_DestroyedWindows);
  printer.Finish(flush);
}

void PrintClientArraysMeta(FastOStream& stream,
                           std::mutex& mutex,
                           bool flush,
                           ClientArraysMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "ClientArraysMeta");
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
  printer.Finish(flush);
}

void PrintUniformArrayMeta(FastOStream& stream,
                           std::mutex& mutex,
                           bool flush,
                           UniformArrayMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "UniformArrayMeta");
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
  printer.Finish(flush);
}

void PrintMappedTextureDataMeta(FastOStream& stream,
                                std::mutex& mutex,
                                bool flush,
                                MappedTextureDataMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "MappedTextureDataMeta");
  printer.Add(command.m_Textures);
  printer.Add(command.m_Levels);
  printer.Add(command.m_Strides);
  printer.Add(command.m_Rows);
  printer.Add(command.m_Data);
  printer.Finish(flush);
}

void PrintPixelFormatMeta(FastOStream& stream,
                          std::mutex& mutex,
                          bool flush,
                          PixelFormatMetaCommand& command) {
  CommandPrinter printer(stream, mutex, command, "PixelFormatMeta");
  printer.Add(command.m_Attributes);
  printer.Finish(flush);
}

void PrintUnsupportedCommand(FastOStream& stream,
                             std::mutex& mutex,
                             bool flush,
                             UnsupportedCommand& command) {
  CommandPrinter printer(stream, mutex, command, "UnsupportedCommand");
  printer.AddText(command.GetName());
  printer.Finish(flush);
}

} // namespace

void TraceLayer::Pre(Command& command) {
  PrintMissingFormatter(m_PreStream, m_Mutex, command);
}

void TraceLayer::Post(Command& command) {
  PrintMissingFormatter(m_PostStream, m_Mutex, command);
}

void TraceLayer::Pre(MappedDataMetaCommand& command) {
  PrintMappedDataMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(MappedDataMetaCommand& command) {
  PrintMappedDataMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(WindowGeometryMetaCommand& command) {
  PrintWindowGeometryMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(WindowGeometryMetaCommand& command) {
  PrintWindowGeometryMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(ClientArraysMetaCommand& command) {
  PrintClientArraysMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(ClientArraysMetaCommand& command) {
  PrintClientArraysMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(UniformArrayMetaCommand& command) {
  PrintUniformArrayMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(UniformArrayMetaCommand& command) {
  PrintUniformArrayMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(MappedTextureDataMetaCommand& command) {
  PrintMappedTextureDataMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(MappedTextureDataMetaCommand& command) {
  PrintMappedTextureDataMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(PixelFormatMetaCommand& command) {
  PrintPixelFormatMeta(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(PixelFormatMetaCommand& command) {
  PrintPixelFormatMeta(m_PostStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Pre(UnsupportedCommand& command) {
  PrintUnsupportedCommand(m_PreStream, m_Mutex, m_Flush, command);
}

void TraceLayer::Post(UnsupportedCommand& command) {
  PrintUnsupportedCommand(m_PostStream, m_Mutex, m_Flush, command);
}

} // namespace gits::opengl
