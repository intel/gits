// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "clientArraysService.h"

#include "bufferShadowService.h"
#include "captureManager.h"
#include "clientArraysMetaTypes.h"
#include "clientMemoryCapture.h"
#include "currentContext.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "log.h"
#include "messageBus.h"
#include "openglDriversAuto.h"
#include "pixelTransferSize.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits::opengl2 {

namespace {

// Which array-state queries are legal on a context. Every query below is gated on one of these,
// so capture never injects a GL error into the application's error state.
struct ContextCaps {
  bool ClientArrays{false};
  bool BufferObjects{false};  // GL 1.5 / ARB_vertex_buffer_object: per-array bindings.
  bool MultiTexture{false};   // GL 1.3 / ARB_multitexture: GL_CLIENT_ACTIVE_TEXTURE.
  bool SecondaryColor{false}; // GL 1.4 / EXT_secondary_color.
  bool FogCoord{false};       // GL 1.4 / EXT_fog_coord.
  // GL 2.0 / ARB_vertex_shader / ARB_vertex_program: generic attribute arrays. The ARB queries
  // take the core enums; only their entry points differ.
  bool GenericAttribs{false};
  bool GenericAttribsCore{false};
  bool IntegerAttribs{false}; // GL 3.0 / EXT_gpu_shader4: GL_VERTEX_ATTRIB_ARRAY_INTEGER.
  bool Divisor{false};        // GL 3.3 / ARB, EXT or NV_instanced_arrays.
  // GL 4.3: GL_VERTEX_ATTRIB_ARRAY_LONG. GL 4.1 and ARB/EXT_vertex_attrib_64bit add the long
  // setters but not this query, so there the setter flag says.
  bool LongAttribs{false};
  bool PrimitiveRestart{false};   // GL 3.1.
  bool PrimitiveRestartNV{false}; // NV_primitive_restart, with its own enums.
  bool FixedIndexRestart{false};  // GL 4.3 / ARB_ES3_compatibility.
  // GL 4.4 / ARB or EXT_buffer_storage, with GL 3.0 for the GL_BUFFER_ACCESS_FLAGS query.
  bool PersistentMaps{false};
  bool CopyBuffer{false};   // GL 3.1 / ARB_copy_buffer: BufferShadowService's staging copy.
  bool VertexArrays{false}; // GL 3.0 / ARB_ or APPLE_vertex_array_object.
  GLint MaxTexCoords{1};
  GLint MaxAttribs{0};
  // glClientActiveTexture, or glClientActiveTextureARB on a pre-1.3 context; null without
  // MultiTexture. The ARB enums have the core values, so only the entry point differs.
  OpenGLDispatchTable::glClientActiveTextureARBPtr ClientActiveTexture{nullptr};
};

bool AtLeast(std::pair<int, int> version, int major, int minor) {
  return GlVersionAtLeast(version, major, minor);
}

GLint GetInteger(GLenum pname) {
  GLint value = 0;
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(pname, &value);
  return value;
}

ContextCaps QueryContextCaps() {
  ContextCaps caps;
  const auto version = CurrentContextGLVersion();
  if (version.first == 0) {
    return caps;
  }
  // Client arrays were removed from core profiles (and from forward-compatible contexts, which
  // drop every deprecated feature); on those contexts every array is necessarily buffer-backed.
  bool core = false;
  if (AtLeast(version, 3, 0)) {
    core = (GetInteger(GL_CONTEXT_FLAGS) & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0;
  }
  if (AtLeast(version, 3, 2)) {
    core = core || (GetInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
  } else if (version.first == 3 && version.second == 1) {
    core = core || !CurrentContextHasExtension(version.first, "GL_ARB_compatibility");
  }
  if (core) {
    return caps;
  }
  caps.ClientArrays = true;
  caps.BufferObjects = AtLeast(version, 1, 5) ||
                       CurrentContextHasExtension(version.first, "GL_ARB_vertex_buffer_object");
  const bool multiTextureCore = AtLeast(version, 1, 3);
  caps.MultiTexture =
      multiTextureCore || CurrentContextHasExtension(version.first, "GL_ARB_multitexture");
  if (caps.MultiTexture) {
    caps.ClientActiveTexture =
        multiTextureCore ? g_DispatchTable.glClientActiveTexture.load(std::memory_order_acquire)
                         : g_DispatchTable.glClientActiveTextureARB.load(std::memory_order_acquire);
  }
  caps.SecondaryColor =
      AtLeast(version, 1, 4) || CurrentContextHasExtension(version.first, "GL_EXT_secondary_color");
  caps.FogCoord =
      AtLeast(version, 1, 4) || CurrentContextHasExtension(version.first, "GL_EXT_fog_coord");
  caps.GenericAttribsCore = AtLeast(version, 2, 0);
  caps.GenericAttribs = caps.GenericAttribsCore ||
                        CurrentContextHasExtension(version.first, "GL_ARB_vertex_shader") ||
                        CurrentContextHasExtension(version.first, "GL_ARB_vertex_program");
  caps.IntegerAttribs =
      AtLeast(version, 3, 0) || CurrentContextHasExtension(version.first, "GL_EXT_gpu_shader4");
  caps.Divisor = AtLeast(version, 3, 3) ||
                 CurrentContextHasExtension(version.first, "GL_ARB_instanced_arrays") ||
                 CurrentContextHasExtension(version.first, "GL_EXT_instanced_arrays") ||
                 CurrentContextHasExtension(version.first, "GL_NV_instanced_arrays");
  caps.LongAttribs = AtLeast(version, 4, 3);
  caps.PrimitiveRestart = AtLeast(version, 3, 1);
  caps.PrimitiveRestartNV = CurrentContextHasExtension(version.first, "GL_NV_primitive_restart");
  caps.FixedIndexRestart = AtLeast(version, 4, 3) ||
                           CurrentContextHasExtension(version.first, "GL_ARB_ES3_compatibility");
  caps.PersistentMaps = AtLeast(version, 4, 4) ||
                        (AtLeast(version, 3, 0) &&
                         (CurrentContextHasExtension(version.first, "GL_ARB_buffer_storage") ||
                          CurrentContextHasExtension(version.first, "GL_EXT_buffer_storage")));
  caps.CopyBuffer =
      AtLeast(version, 3, 1) || CurrentContextHasExtension(version.first, "GL_ARB_copy_buffer");
  caps.VertexArrays = AtLeast(version, 3, 0) ||
                      CurrentContextHasExtension(version.first, "GL_ARB_vertex_array_object") ||
                      CurrentContextHasExtension(version.first, "GL_APPLE_vertex_array_object");
  if (AtLeast(version, 2, 0)) {
    caps.MaxTexCoords = GetInteger(GL_MAX_TEXTURE_COORDS);
  } else if (caps.MultiTexture) {
    caps.MaxTexCoords = GetInteger(GL_MAX_TEXTURE_UNITS);
  }
  if (caps.GenericAttribs) {
    caps.MaxAttribs = GetInteger(GL_MAX_VERTEX_ATTRIBS);
  }
  return caps;
}

// Per-context array bookkeeping. Marks are the slots ever given a client-memory pointer on the
// context, in any VAO; a draw only queries marked slots. Marks are never cleared, so they stay a
// superset of what any VAO can reference. The first draw with each VAO bound (0 is the default
// one) also scans every slot once, for state set before the recorder saw the context.
struct ContextState {
  ContextCaps Caps;
  std::unordered_set<GLuint> ScannedVertexArrays;
  bool Scanning{false};   // The current draw scans every slot.
  uint32_t FixedMarks{0}; // Bit per fixed-function ClientArrayKind.
  std::vector<bool> TexCoordMarks;
  std::vector<bool> AttribMarks;
  // CLIENT_ARRAY_FLAG_INTEGER/LONG implied by the last client-memory setter of each attribute.
  // Per context rather than per VAO: only read where the state itself cannot be queried.
  std::vector<uint32_t> AttribSetterFlags;

  bool IsMarked(ClientArrayKind kind, uint32_t slot) const {
    switch (kind) {
    case ClientArrayKind::TexCoord:
      return slot < TexCoordMarks.size() && TexCoordMarks[slot];
    case ClientArrayKind::VertexAttrib:
      return slot < AttribMarks.size() && AttribMarks[slot];
    default:
      return (FixedMarks & (1u << static_cast<uint32_t>(kind))) != 0;
    }
  }

  void Mark(ClientArrayKind kind, uint32_t slot) {
    switch (kind) {
    case ClientArrayKind::TexCoord:
      if (slot < TexCoordMarks.size()) {
        TexCoordMarks[slot] = true;
      }
      break;
    case ClientArrayKind::VertexAttrib:
      if (slot < AttribMarks.size()) {
        AttribMarks[slot] = true;
      }
      break;
    default:
      FixedMarks |= 1u << static_cast<uint32_t>(kind);
      break;
    }
  }
};

ContextState MakeContextState() {
  ContextState state;
  state.Caps = QueryContextCaps();
  state.TexCoordMarks.assign(static_cast<size_t>((std::max)(state.Caps.MaxTexCoords, 0)), false);
  state.AttribMarks.assign(static_cast<size_t>((std::max)(state.Caps.MaxAttribs, 0)), false);
  state.AttribSetterFlags.assign(state.AttribMarks.size(), 0);
  return state;
}

// Keyed by context GITSKey. A context is current on at most one thread, so its entry is used
// without the lock once found; unordered_map never moves its elements. Contexts with the shared
// unregistered key 0 (see ContextSupportsGLFeature(), clientMemoryCapture.cpp) get fresh,
// unscanned state on every call, so each of their draws scans every slot.
std::mutex g_ContextsMutex;
std::unordered_map<GITSKey, ContextState> g_Contexts;

ContextState& CurrentContextState(ContextState& unregistered) {
  const GITSKey contextKey = CurrentContextKey();
  if (contextKey == 0) {
    unregistered = MakeContextState();
    return unregistered;
  }
  {
    std::lock_guard<std::mutex> lock(g_ContextsMutex);
    auto it = g_Contexts.find(contextKey);
    if (it != g_Contexts.end()) {
      return it->second;
    }
  }
  ContextState state = MakeContextState();
  std::lock_guard<std::mutex> lock(g_ContextsMutex);
  return g_Contexts.try_emplace(contextKey, std::move(state)).first->second;
}

// Work done for hooked draws, logged once at program exit to judge the recording cost on a
// title. Byte counts are before stream compression.
struct Statistics {
  std::atomic<uint64_t> Draws{0};
  std::atomic<uint64_t> CapturedDraws{0};
  std::atomic<uint64_t> SlotQueries{0};
  std::atomic<uint64_t> CopiedIndexReads{0};
  std::atomic<uint64_t> StagedIndexReads{0};
  std::atomic<uint64_t> FailedIndexReads{0};
  std::atomic<uint64_t> SentBlocks{0};
  std::atomic<uint64_t> SentBytes{0};
  std::atomic<uint64_t> ReusedBlocks{0};
  std::atomic<uint64_t> ReusedBytes{0};
};
Statistics g_Stats;

void Count(std::atomic<uint64_t>& counter, uint64_t value = 1) {
  counter.fetch_add(value, std::memory_order_relaxed);
}

void LogStatistics() {
  const auto get = [](const std::atomic<uint64_t>& counter) {
    return counter.load(std::memory_order_relaxed);
  };
  if (get(g_Stats.Draws) == 0) {
    return;
  }
  LOG_INFO << "ClientArraysService: " << get(g_Stats.Draws) << " hooked draws, "
           << get(g_Stats.CapturedDraws) << " read client-memory arrays; "
           << get(g_Stats.SlotQueries) << " array slots queried; element buffer indices read "
           << get(g_Stats.CopiedIndexReads) << " times from recorder copies, "
           << get(g_Stats.StagedIndexReads) << " through staging, " << get(g_Stats.FailedIndexReads)
           << " not readable; sent " << get(g_Stats.SentBlocks) << " blocks ("
           << get(g_Stats.SentBytes) << " bytes), referenced unchanged "
           << get(g_Stats.ReusedBlocks) << " blocks (" << get(g_Stats.ReusedBytes) << " bytes)";
}

std::mutex g_WarnMutex;
bool g_WarnedUnknownType = false;
bool g_WarnedMappedIndices = false;
bool g_WarnedUnreadableIndices = false;
bool g_WarnedMultiDrawClientIndices = false;

void WarnOnce(bool& warned, const char* message) {
  std::lock_guard<std::mutex> lock(g_WarnMutex);
  if (!warned) {
    warned = true;
    LOG_WARNING << "ClientArraysService: " << message;
  }
}

struct ClientArray {
  ClientArrayKind Kind{};
  uint32_t Slot{0};
  GLint Size{0};
  GLenum Type{0};
  GLsizei Stride{0};
  uint32_t Flags{0};
  GLuint Divisor{0};
  const uint8_t* Pointer{nullptr};
};

// Bytes of one vertex of an array, or 0 for a type this does not know.
uint64_t VertexElementSize(GLint size, GLenum type) {
  switch (type) {
  case GL_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_10F_11F_11F_REV:
    return 4;
  default:
    break;
  }
  uint64_t componentSize = 0;
  switch (type) {
  case GL_BYTE:
  case GL_UNSIGNED_BYTE:
    componentSize = 1;
    break;
  case GL_SHORT:
  case GL_UNSIGNED_SHORT:
  case GL_HALF_FLOAT:
    componentSize = 2;
    break;
  case GL_INT:
  case GL_UNSIGNED_INT:
  case GL_FLOAT:
  case GL_FIXED:
    componentSize = 4;
    break;
  case GL_DOUBLE:
    componentSize = 8;
    break;
  default:
    return 0;
  }
  const GLint components = size == GL_BGRA ? 4 : size;
  return components > 0 ? componentSize * static_cast<uint64_t>(components) : 0;
}

bool BufferBound(const ContextCaps& caps, GLenum bindingPname) {
  return caps.BufferObjects && GetInteger(bindingPname) != 0;
}

// Whether a slot needs querying on this draw: every slot on a VAO's first scan, afterwards only
// marked ones.
bool ShouldQuery(const ContextState& state, ClientArrayKind kind, uint32_t slot) {
  return state.Scanning || state.IsMarked(kind, slot);
}

void AddFixedArray(std::vector<ClientArray>& arrays,
                   ContextState& state,
                   ClientArrayKind kind,
                   uint32_t slot,
                   GLenum cap,
                   GLenum bindingPname,
                   GLenum pointerPname,
                   GLenum sizePname,
                   GLint fixedSize,
                   GLenum typePname,
                   GLenum fixedType,
                   GLenum stridePname) {
  if (!ShouldQuery(state, kind, slot)) {
    return;
  }
  Count(g_Stats.SlotQueries);
  const bool enabled = g_DispatchTable.glIsEnabled.load(std::memory_order_acquire)(cap) != 0;
  // The first scan marks client pointers of disabled arrays too: enabling one later needs no
  // pointer call.
  if ((!enabled && !state.Scanning) || BufferBound(state.Caps, bindingPname)) {
    return;
  }
  void* pointer = nullptr;
  g_DispatchTable.glGetPointerv.load(std::memory_order_acquire)(pointerPname, &pointer);
  if (pointer == nullptr) {
    return;
  }
  state.Mark(kind, slot);
  if (!enabled) {
    return;
  }
  ClientArray array;
  array.Kind = kind;
  array.Slot = slot;
  array.Size = sizePname != 0 ? GetInteger(sizePname) : fixedSize;
  array.Type = typePname != 0 ? static_cast<GLenum>(GetInteger(typePname)) : fixedType;
  array.Stride = GetInteger(stridePname);
  array.Pointer = static_cast<const uint8_t*>(pointer);
  arrays.push_back(array);
}

void AddVertexAttribArray(std::vector<ClientArray>& arrays, ContextState& state, GLuint index) {
  if (!ShouldQuery(state, ClientArrayKind::VertexAttrib, index)) {
    return;
  }
  Count(g_Stats.SlotQueries);
  const ContextCaps& caps = state.Caps;
  auto getVertexAttribiv =
      caps.GenericAttribsCore
          ? g_DispatchTable.glGetVertexAttribiv.load(std::memory_order_acquire)
          : g_DispatchTable.glGetVertexAttribivARB.load(std::memory_order_acquire);
  auto getVertexAttribPointerv =
      caps.GenericAttribsCore
          ? g_DispatchTable.glGetVertexAttribPointerv.load(std::memory_order_acquire)
          : g_DispatchTable.glGetVertexAttribPointervARB.load(std::memory_order_acquire);
  auto getAttrib = [&](GLenum pname) {
    GLint value = 0;
    getVertexAttribiv(index, pname, &value);
    return value;
  };
  const bool enabled = getAttrib(GL_VERTEX_ATTRIB_ARRAY_ENABLED) != 0;
  if ((!enabled && !state.Scanning) || getAttrib(GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING) != 0) {
    return;
  }
  void* pointer = nullptr;
  getVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
  if (pointer == nullptr) {
    return;
  }
  state.Mark(ClientArrayKind::VertexAttrib, index);
  if (!enabled) {
    return;
  }
  const uint32_t setterFlags =
      index < state.AttribSetterFlags.size() ? state.AttribSetterFlags[index] : 0;
  ClientArray array;
  array.Kind = ClientArrayKind::VertexAttrib;
  array.Slot = index;
  array.Size = getAttrib(GL_VERTEX_ATTRIB_ARRAY_SIZE);
  array.Type = static_cast<GLenum>(getAttrib(GL_VERTEX_ATTRIB_ARRAY_TYPE));
  array.Stride = getAttrib(GL_VERTEX_ATTRIB_ARRAY_STRIDE);
  if (getAttrib(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED) != 0) {
    array.Flags |= CLIENT_ARRAY_FLAG_NORMALIZED;
  }
  // The state is authoritative where it can be queried; otherwise the setter that set it says.
  if (caps.IntegerAttribs ? getAttrib(GL_VERTEX_ATTRIB_ARRAY_INTEGER) != 0
                          : (setterFlags & CLIENT_ARRAY_FLAG_INTEGER) != 0) {
    array.Flags |= CLIENT_ARRAY_FLAG_INTEGER;
  }
  if (caps.LongAttribs ? getAttrib(GL_VERTEX_ATTRIB_ARRAY_LONG) != 0
                       : (setterFlags & CLIENT_ARRAY_FLAG_LONG) != 0) {
    array.Flags |= CLIENT_ARRAY_FLAG_LONG;
  }
  if (caps.Divisor) {
    array.Divisor = static_cast<GLuint>(getAttrib(GL_VERTEX_ATTRIB_ARRAY_DIVISOR));
  }
  array.Pointer = static_cast<const uint8_t*>(pointer);
  arrays.push_back(array);
}

std::vector<ClientArray> QueryClientArrays(ContextState& state) {
  const ContextCaps& caps = state.Caps;
  const auto vertexArray =
      caps.VertexArrays ? static_cast<GLuint>(GetInteger(GL_VERTEX_ARRAY_BINDING)) : 0u;
  state.Scanning = state.ScannedVertexArrays.insert(vertexArray).second;
  std::vector<ClientArray> arrays;
  AddFixedArray(arrays, state, ClientArrayKind::Vertex, 0, GL_VERTEX_ARRAY,
                GL_VERTEX_ARRAY_BUFFER_BINDING, GL_VERTEX_ARRAY_POINTER, GL_VERTEX_ARRAY_SIZE, 0,
                GL_VERTEX_ARRAY_TYPE, 0, GL_VERTEX_ARRAY_STRIDE);
  AddFixedArray(arrays, state, ClientArrayKind::Normal, 0, GL_NORMAL_ARRAY,
                GL_NORMAL_ARRAY_BUFFER_BINDING, GL_NORMAL_ARRAY_POINTER, 0, 3, GL_NORMAL_ARRAY_TYPE,
                0, GL_NORMAL_ARRAY_STRIDE);
  AddFixedArray(arrays, state, ClientArrayKind::Color, 0, GL_COLOR_ARRAY,
                GL_COLOR_ARRAY_BUFFER_BINDING, GL_COLOR_ARRAY_POINTER, GL_COLOR_ARRAY_SIZE, 0,
                GL_COLOR_ARRAY_TYPE, 0, GL_COLOR_ARRAY_STRIDE);
  AddFixedArray(arrays, state, ClientArrayKind::Index, 0, GL_INDEX_ARRAY,
                GL_INDEX_ARRAY_BUFFER_BINDING, GL_INDEX_ARRAY_POINTER, 0, 1, GL_INDEX_ARRAY_TYPE, 0,
                GL_INDEX_ARRAY_STRIDE);
  AddFixedArray(arrays, state, ClientArrayKind::EdgeFlag, 0, GL_EDGE_FLAG_ARRAY,
                GL_EDGE_FLAG_ARRAY_BUFFER_BINDING, GL_EDGE_FLAG_ARRAY_POINTER, 0, 1, 0,
                GL_UNSIGNED_BYTE, GL_EDGE_FLAG_ARRAY_STRIDE);
  if (caps.SecondaryColor) {
    AddFixedArray(arrays, state, ClientArrayKind::SecondaryColor, 0, GL_SECONDARY_COLOR_ARRAY,
                  GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING, GL_SECONDARY_COLOR_ARRAY_POINTER,
                  GL_SECONDARY_COLOR_ARRAY_SIZE, 0, GL_SECONDARY_COLOR_ARRAY_TYPE, 0,
                  GL_SECONDARY_COLOR_ARRAY_STRIDE);
  }
  if (caps.FogCoord) {
    AddFixedArray(arrays, state, ClientArrayKind::FogCoord, 0, GL_FOG_COORD_ARRAY,
                  GL_FOG_COORD_ARRAY_BUFFER_BINDING, GL_FOG_COORD_ARRAY_POINTER, 0, 1,
                  GL_FOG_COORD_ARRAY_TYPE, 0, GL_FOG_COORD_ARRAY_STRIDE);
  }

  // Texture coordinate array state is selected by the client active texture unit, switched only
  // for units that are queried.
  auto clientActiveTexture = caps.ClientActiveTexture;
  std::optional<GLint> activeTexture;
  for (GLint unit = 0; unit < caps.MaxTexCoords; ++unit) {
    const auto slot = static_cast<uint32_t>(unit);
    if (!ShouldQuery(state, ClientArrayKind::TexCoord, slot)) {
      continue;
    }
    if (caps.MultiTexture) {
      if (!activeTexture) {
        activeTexture = GetInteger(GL_CLIENT_ACTIVE_TEXTURE);
      }
      clientActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    }
    AddFixedArray(arrays, state, ClientArrayKind::TexCoord, slot, GL_TEXTURE_COORD_ARRAY,
                  GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, GL_TEXTURE_COORD_ARRAY_POINTER,
                  GL_TEXTURE_COORD_ARRAY_SIZE, 0, GL_TEXTURE_COORD_ARRAY_TYPE, 0,
                  GL_TEXTURE_COORD_ARRAY_STRIDE);
  }
  if (activeTexture) {
    clientActiveTexture(static_cast<GLenum>(*activeTexture));
  }

  for (GLint i = 0; i < caps.MaxAttribs; ++i) {
    AddVertexAttribArray(arrays, state, static_cast<GLuint>(i));
  }
  state.Scanning = false;
  return arrays;
}

struct VertexRange {
  int64_t Min{0};
  int64_t Max{0};
};

// The index bytes an element draw reads: its own client-memory copy, or the range of the bound
// GL_ELEMENT_ARRAY_BUFFER it reads, from BufferShadowService (never read back from the
// application's buffer directly; see bufferShadowService.h).
bool ReadIndices(const ContextCaps& caps,
                 const ClientArraysDraw& draw,
                 uint64_t byteSize,
                 std::vector<uint8_t>& storage,
                 const uint8_t*& bytes) {
  if (draw.Indices != nullptr && draw.Indices->HasValue) {
    if (draw.Indices->Data.size() < byteSize) {
      return false;
    }
    bytes = draw.Indices->Data.data();
    return true;
  }
  if (!ElementArrayBufferIsBound()) {
    return false;
  }
  GLint mapped = 0;
  GetBoundBufferParameter(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_MAPPED, mapped);
  if (mapped != 0) {
    GLint accessFlags = 0;
    if (caps.PersistentMaps) {
      GetBoundBufferParameter(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_ACCESS_FLAGS, accessFlags);
    }
    if ((accessFlags & GL_MAP_PERSISTENT_BIT) == 0) {
      WarnOnce(g_WarnedMappedIndices,
               "a draw reads client-memory vertex arrays through indices in a mapped element "
               "array buffer; its vertex arrays are not captured and replay of that draw may "
               "read invalid memory");
      return false;
    }
  }
  switch (BufferShadowService::Get().ReadElementBuffer(caps.CopyBuffer, mapped != 0,
                                                       draw.IndicesOffset, byteSize, storage)) {
  case BufferShadowService::ReadSource::Copy:
    Count(g_Stats.CopiedIndexReads);
    break;
  case BufferShadowService::ReadSource::Staging:
    Count(g_Stats.StagedIndexReads);
    break;
  case BufferShadowService::ReadSource::Failed:
    Count(g_Stats.FailedIndexReads);
    WarnOnce(g_WarnedUnreadableIndices,
             "a draw reads client-memory vertex arrays through indices in an element array "
             "buffer whose contents the recorder does not know and cannot copy (needs GL 3.1 or "
             "GL_ARB_copy_buffer, and the read inside the buffer); its vertex arrays are not "
             "captured and replay of that draw may read invalid memory");
    return false;
  }
  bytes = storage.data();
  return true;
}

template <typename T>
bool ScanIndices(const uint8_t* bytes,
                 GLsizei count,
                 std::optional<uint32_t> restartIndex,
                 VertexRange& range) {
  bool found = false;
  uint32_t lowest = std::numeric_limits<uint32_t>::max();
  uint32_t highest = 0;
  for (GLsizei i = 0; i < count; ++i) {
    T value{};
    std::memcpy(&value, bytes + static_cast<size_t>(i) * sizeof(T), sizeof(T));
    const uint32_t index = value;
    if (restartIndex && index == *restartIndex) {
      continue;
    }
    lowest = (std::min)(lowest, index);
    highest = (std::max)(highest, index);
    found = true;
  }
  range.Min = lowest;
  range.Max = highest;
  return found;
}

std::optional<uint32_t> RestartIndex(const ContextCaps& caps, GLenum indexType) {
  auto isEnabled = g_DispatchTable.glIsEnabled.load(std::memory_order_acquire);
  if (caps.FixedIndexRestart && isEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX)) {
    const uint32_t bits = ElementIndexTypeSize(indexType) * 8;
    return bits >= 32 ? std::numeric_limits<uint32_t>::max() : (1u << bits) - 1;
  }
  if (caps.PrimitiveRestart && isEnabled(GL_PRIMITIVE_RESTART)) {
    return static_cast<uint32_t>(GetInteger(GL_PRIMITIVE_RESTART_INDEX));
  }
  if (caps.PrimitiveRestartNV && isEnabled(GL_PRIMITIVE_RESTART_NV)) {
    return static_cast<uint32_t>(GetInteger(GL_PRIMITIVE_RESTART_INDEX_NV));
  }
  return std::nullopt;
}

// The range of per-vertex elements a single draw reads, before its base vertex is applied for
// array draws (`first` already is the base).
bool SingleDrawVertexRange(const ContextCaps& caps,
                           const ClientArraysDraw& draw,
                           VertexRange& range) {
  if (draw.Count <= 0) {
    return false;
  }
  if (!draw.IsElements) {
    range.Min = draw.First;
    range.Max = static_cast<int64_t>(draw.First) + draw.Count - 1;
    return range.Min >= 0;
  }
  const uint64_t byteSize = ComputeElementIndicesByteSize(draw.Count, draw.IndexType);
  if (byteSize == 0) {
    return false;
  }
  std::vector<uint8_t> storage;
  const uint8_t* bytes = nullptr;
  if (!ReadIndices(caps, draw, byteSize, storage, bytes)) {
    return false;
  }
  const auto restart = RestartIndex(caps, draw.IndexType);
  bool found = false;
  switch (draw.IndexType) {
  case GL_UNSIGNED_BYTE:
    found = ScanIndices<uint8_t>(bytes, draw.Count, restart, range);
    break;
  case GL_UNSIGNED_SHORT:
    found = ScanIndices<uint16_t>(bytes, draw.Count, restart, range);
    break;
  default:
    found = ScanIndices<uint32_t>(bytes, draw.Count, restart, range);
    break;
  }
  if (!found) {
    return false;
  }
  range.Min += draw.BaseVertex;
  range.Max += draw.BaseVertex;
  range.Min = (std::max)(range.Min, int64_t{0});
  return range.Max >= range.Min;
}

// The union of the ranges every sub-draw of a multi-draw reads.
bool DrawVertexRange(const ContextCaps& caps, const ClientArraysDraw& draw, VertexRange& range) {
  if (draw.Counts == nullptr) {
    return SingleDrawVertexRange(caps, draw, range);
  }
  if (draw.IsElements ? draw.IndicesOffsets == nullptr : draw.Firsts == nullptr) {
    return false;
  }
  if (draw.IsElements && !ElementArrayBufferIsBound()) {
    WarnOnce(g_WarnedMultiDrawClientIndices,
             "a multi-draw reads client-memory vertex arrays through client-memory indices, "
             "which are not captured; its vertex arrays are not captured and replay of that "
             "draw may read invalid memory");
    return false;
  }
  bool found = false;
  for (GLsizei i = 0; i < draw.DrawCount; ++i) {
    ClientArraysDraw single = draw;
    single.Counts = nullptr;
    single.Count = draw.Counts[i];
    if (draw.IsElements) {
      single.Indices = nullptr;
      single.IndicesOffset = draw.IndicesOffsets[i];
    } else {
      single.First = draw.Firsts[i];
    }
    single.BaseVertex = draw.BaseVertices != nullptr ? draw.BaseVertices[i] : 0;
    VertexRange sub;
    if (!SingleDrawVertexRange(caps, single, sub)) {
      continue;
    }
    range.Min = found ? (std::min)(range.Min, sub.Min) : sub.Min;
    range.Max = found ? (std::max)(range.Max, sub.Max) : sub.Max;
    found = true;
  }
  return found;
}

// Client memory already sent to the player, per recording thread (each thread's commands replay
// in order, so the player holds exactly these blocks when this thread's next command runs).
// A block whose bytes still match is referenced again instead of re-sent. Bounded, least
// recently used first; an evicted block is released by the thread's next command, after the
// draw that may still read it.
constexpr uint64_t BLOCK_CACHE_LIMIT = 512ull << 20;

struct CachedBlock {
  uint64_t Id{0};
  std::vector<uint8_t> Bytes;
  uint64_t LastUse{0};
};

struct ThreadBlocks {
  std::map<uintptr_t, CachedBlock> ByStart;
  uint64_t TotalBytes{0};
  uint64_t Clock{0};
  std::vector<uint64_t> PendingRelease;

  void Evict(std::map<uintptr_t, CachedBlock>::iterator it) {
    TotalBytes -= it->second.Bytes.size();
    PendingRelease.push_back(it->second.Id);
    ByStart.erase(it);
  }
};

thread_local ThreadBlocks t_Blocks;
std::atomic<uint64_t> g_NextBlockId{1};

struct Region {
  uintptr_t Begin{0};
  uintptr_t End{0};
};

// The cached block holding [begin, end) with unchanged bytes, or ByStart.end().
std::map<uintptr_t, CachedBlock>::iterator FindUnchanged(uintptr_t begin, uintptr_t end) {
  auto& blocks = t_Blocks.ByStart;
  auto it = blocks.upper_bound(begin);
  if (it == blocks.begin()) {
    return blocks.end();
  }
  --it;
  const uintptr_t start = it->first;
  if (end - start > it->second.Bytes.size() ||
      std::memcmp(it->second.Bytes.data() + (begin - start), reinterpret_cast<const void*>(begin),
                  end - begin) != 0) {
    return blocks.end();
  }
  return it;
}

// Sends [begin, end) as a new block, evicting cached blocks it overlaps. Returns the block.
std::map<uintptr_t, CachedBlock>::iterator SendBlock(ClientArraysMetaCommand& command,
                                                     uintptr_t begin,
                                                     uintptr_t end) {
  auto& blocks = t_Blocks.ByStart;
  auto it = blocks.upper_bound(begin);
  if (it != blocks.begin() && std::prev(it)->first + std::prev(it)->second.Bytes.size() > begin) {
    --it;
  }
  while (it != blocks.end() && it->first < end) {
    auto next = std::next(it);
    t_Blocks.Evict(it);
    it = next;
  }
  const auto* source = reinterpret_cast<const uint8_t*>(begin);
  CachedBlock block;
  block.Id = g_NextBlockId.fetch_add(1, std::memory_order_relaxed);
  block.Bytes.assign(source, source + (end - begin));
  command.m_BlockIds.Data.push_back(block.Id);
  command.m_BlockSizes.Data.push_back(end - begin);
  command.m_Data.Data.insert(command.m_Data.Data.end(), source, source + (end - begin));
  t_Blocks.TotalBytes += end - begin;
  return blocks.emplace(begin, std::move(block)).first;
}

void EnforceBlockCacheLimit() {
  while (t_Blocks.TotalBytes > BLOCK_CACHE_LIMIT) {
    auto oldest = t_Blocks.ByStart.end();
    for (auto it = t_Blocks.ByStart.begin(); it != t_Blocks.ByStart.end(); ++it) {
      if (it->second.LastUse != t_Blocks.Clock &&
          (oldest == t_Blocks.ByStart.end() || it->second.LastUse < oldest->second.LastUse)) {
        oldest = it;
      }
    }
    if (oldest == t_Blocks.ByStart.end()) {
      return; // Everything left is read by the current draw.
    }
    t_Blocks.Evict(oldest);
  }
}

} // namespace

ClientArraysService& ClientArraysService::Get() {
  static ClientArraysService instance;
  return instance;
}

ClientArraysService::ClientArraysService() {
  MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                              [](Topic, const MessagePtr&) { LogStatistics(); });
}

void ClientArraysService::NoteArrayPointer(ClientArrayKind kind,
                                           int64_t slot,
                                           const void* pointer,
                                           uint32_t flags) {
  ContextState unregistered;
  ContextState& state = CurrentContextState(unregistered);
  if (!state.Caps.ClientArrays || pointer == nullptr ||
      BufferBound(state.Caps, GL_ARRAY_BUFFER_BINDING)) {
    return;
  }
  if (kind == ClientArrayKind::TexCoord && slot < 0) {
    slot = state.Caps.MultiTexture ? GetInteger(GL_CLIENT_ACTIVE_TEXTURE) - GL_TEXTURE0 : 0;
  }
  if (slot < 0) {
    return;
  }
  state.Mark(kind, static_cast<uint32_t>(slot));
  if (kind == ClientArrayKind::VertexAttrib &&
      static_cast<uint64_t>(slot) < state.AttribSetterFlags.size()) {
    state.AttribSetterFlags[static_cast<size_t>(slot)] = flags;
  }
}

void ClientArraysService::NoteInterleavedArrays(const void* pointer) {
  for (auto kind : {ClientArrayKind::Vertex, ClientArrayKind::Normal, ClientArrayKind::Color}) {
    NoteArrayPointer(kind, 0, pointer);
  }
  // -1: the client active texture unit, the one glInterleavedArrays sets.
  NoteArrayPointer(ClientArrayKind::TexCoord, -1, pointer);
}

void ClientArraysService::Capture(uint32_t threadId, const ClientArraysDraw& draw, uint64_t key) {
  Count(g_Stats.Draws);
  ContextState unregistered;
  ContextState& state = CurrentContextState(unregistered);
  std::vector<ClientArray> arrays;
  if (state.Caps.ClientArrays && draw.InstanceCount > 0) {
    arrays = QueryClientArrays(state);
  }
  VertexRange vertices;
  if (arrays.empty() || !DrawVertexRange(state.Caps, draw, vertices)) {
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }

  // Byte range each array reads.
  std::vector<Region> ranges;
  std::vector<const ClientArray*> captured;
  for (const ClientArray& array : arrays) {
    const uint64_t elementSize = VertexElementSize(array.Size, array.Type);
    if (elementSize == 0) {
      WarnOnce(g_WarnedUnknownType,
               "a client-memory vertex array has a size/type this cannot size; it is not "
               "captured and replay of draws reading it may read invalid memory");
      continue;
    }
    VertexRange range = vertices;
    if (array.Divisor != 0) {
      const auto base = static_cast<int64_t>(draw.BaseInstance);
      range.Min = base;
      range.Max = base + (static_cast<int64_t>(draw.InstanceCount) - 1) / array.Divisor;
    }
    const uint64_t step = array.Stride != 0 ? static_cast<uint64_t>(array.Stride) : elementSize;
    const uintptr_t begin =
        reinterpret_cast<uintptr_t>(array.Pointer) + static_cast<uint64_t>(range.Min) * step;
    const uintptr_t end = begin + static_cast<uint64_t>(range.Max - range.Min) * step + elementSize;
    ranges.push_back({begin, end});
    captured.push_back(&array);
  }
  if (captured.empty()) {
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }

  // Overlapping ranges (interleaved arrays) share one region.
  std::vector<Region> regions = ranges;
  std::sort(regions.begin(), regions.end(),
            [](const Region& a, const Region& b) { return a.Begin < b.Begin; });
  size_t merged = 0;
  for (size_t i = 1; i < regions.size(); ++i) {
    if (regions[i].Begin <= regions[merged].End) {
      regions[merged].End = (std::max)(regions[merged].End, regions[i].End);
    } else {
      regions[++merged] = regions[i];
    }
  }
  regions.resize(merged + 1);

  ClientArraysMetaCommand command(threadId);
  command.m_Key = key;
  command.m_ReleasedBlocks.Data = std::move(t_Blocks.PendingRelease);
  t_Blocks.PendingRelease.clear();
  ++t_Blocks.Clock;
  std::vector<std::pair<uintptr_t, uint64_t>> regionBlocks; // (block start, block id)
  for (const Region& region : regions) {
    auto block = FindUnchanged(region.Begin, region.End);
    if (block == t_Blocks.ByStart.end()) {
      block = SendBlock(command, region.Begin, region.End);
      Count(g_Stats.SentBlocks);
      Count(g_Stats.SentBytes, region.End - region.Begin);
    } else {
      Count(g_Stats.ReusedBlocks);
      Count(g_Stats.ReusedBytes, region.End - region.Begin);
    }
    block->second.LastUse = t_Blocks.Clock;
    regionBlocks.emplace_back(block->first, block->second.Id);
  }
  EnforceBlockCacheLimit();

  for (size_t i = 0; i < captured.size(); ++i) {
    const ClientArray& array = *captured[i];
    // The region containing this array's range: the last one starting at or before it.
    const auto region =
        std::upper_bound(regions.begin(), regions.end(), ranges[i].Begin,
                         [](uintptr_t value, const Region& r) { return value < r.Begin; }) -
        1;
    const auto& [blockStart, blockId] = regionBlocks[region - regions.begin()];
    command.m_Kinds.Data.push_back(static_cast<uint32_t>(array.Kind));
    command.m_Slots.Data.push_back(array.Slot);
    command.m_Sizes.Data.push_back(array.Size);
    command.m_Types.Data.push_back(array.Type);
    command.m_Strides.Data.push_back(array.Stride);
    command.m_Flags.Data.push_back(array.Flags);
    command.m_ArrayBlocks.Data.push_back(blockId);
    command.m_ArrayOffsets.Data.push_back(
        static_cast<int64_t>(reinterpret_cast<uintptr_t>(array.Pointer) - blockStart));
  }

  for (auto* field : {&command.m_ReleasedBlocks, &command.m_BlockIds, &command.m_BlockSizes,
                      &command.m_ArrayBlocks}) {
    field->HasValue = true;
  }
  for (auto* field : {&command.m_Kinds, &command.m_Slots, &command.m_Types, &command.m_Flags}) {
    field->HasValue = true;
  }
  command.m_Data.HasValue = true;
  command.m_Sizes.HasValue = true;
  command.m_Strides.HasValue = true;
  command.m_ArrayOffsets.HasValue = true;
  Count(g_Stats.CapturedDraws);
  CaptureManager::Get().RecordCommand(command, new ClientArraysMetaSerializer(command));
}

} // namespace gits::opengl2
