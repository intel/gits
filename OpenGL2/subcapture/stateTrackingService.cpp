// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingService.h"

#include <algorithm>
#include <iterator>

namespace gits::opengl2 {

bool IsContainerKind(GLObjectKind kind) {
  switch (kind) {
  case GLObjectKind::Framebuffer:
  case GLObjectKind::VertexArray:
  case GLObjectKind::Query:
  case GLObjectKind::ProgramPipeline:
  case GLObjectKind::TransformFeedback:
    return true;
  default:
    return false;
  }
}

void StateTrackingService::SetGeometry(const WindowGeometryMetaCommand& command) {
  m_Geometry = Store<WindowGeometryMetaSerializer>(command);
}

SampledCommand StateTrackingService::Sample(std::shared_ptr<StoredCommand> command) {
  return {std::exchange(m_Geometry, nullptr), std::move(command)};
}

void StateTrackingService::AddTopology(SampledCommand command) {
  m_Topology.push_back(std::move(command));
}

void StateTrackingService::CreateContext(GITSKey context, GITSKey shareContext) {
  if (context == 0) {
    return;
  }
  const auto shared = m_Contexts.find(shareContext);
  m_Contexts[context].ShareGroup = shared != m_Contexts.end() ? shared->second.ShareGroup : context;
}

void StateTrackingService::ShareLists(GITSKey context, GITSKey otherContext) {
  const auto source = m_Contexts.find(context);
  const auto target = m_Contexts.find(otherContext);
  if (source == m_Contexts.end() || target == m_Contexts.end()) {
    return;
  }
  const GITSKey oldGroup = target->second.ShareGroup;
  const GITSKey newGroup = source->second.ShareGroup;
  if (oldGroup == newGroup) {
    return;
  }
  for (auto& [key, state] : m_Contexts) {
    if (state.ShareGroup == oldGroup) {
      state.ShareGroup = newGroup;
    }
  }
  for (auto& [key, object] : m_Objects) {
    if (!IsContainerKind(object.Kind) && object.Owner == oldGroup) {
      object.Owner = newGroup;
    }
  }
  std::multiset<std::tuple<GITSKey, GLObjectKind, GLuint>> deletedNames;
  for (auto [shareGroup, kind, name] : m_DeletedNames) {
    deletedNames.emplace(shareGroup == oldGroup ? newGroup : shareGroup, kind, name);
  }
  m_DeletedNames = std::move(deletedNames);
  for (auto& [key, program] : m_ArbPrograms) {
    if (program.ShareGroup == oldGroup) {
      program.ShareGroup = newGroup;
    }
  }
  for (auto& [key, owner] : m_Syncs) {
    if (owner == oldGroup) {
      owner = newGroup;
    }
  }
  for (auto list = m_DisplayLists.begin(); list != m_DisplayLists.end();) {
    if (list->first.first == oldGroup) {
      m_DisplayLists[{newGroup, list->first.second}] = std::move(list->second);
      list = m_DisplayLists.erase(list);
    } else {
      ++list;
    }
  }
  for (FontLists& font : m_Fonts) {
    if (font.ShareGroup == oldGroup) {
      font.ShareGroup = newGroup;
    }
  }
  for (auto handle = m_Handles.begin(); handle != m_Handles.end();) {
    if (handle->first.first == oldGroup) {
      m_Handles[{newGroup, handle->first.second}] = std::move(handle->second);
      handle = m_Handles.erase(handle);
    } else {
      ++handle;
    }
  }
}

void StateTrackingService::DeleteContext(GITSKey context) {
  m_Contexts.erase(context);
  m_ResidentHandles.erase(context);
  std::erase_if(m_Objects, [context](const auto& entry) {
    return IsContainerKind(entry.second.Kind) && entry.second.Owner == context;
  });
}

void StateTrackingService::SetCurrent(
    uint32_t threadId, GITSKey context, HDC dc, HDC readDc, SampledCommand call) {
  auto current = std::make_shared<MakeCurrent>();
  current->Call = std::move(call);
  current->ThreadId = threadId;
  current->Context = context;
  current->Dc = dc;
  current->ReadDc = readDc;
  m_Threads[threadId] = current;
  const auto state = m_Contexts.find(context);
  if (state == m_Contexts.end()) {
    return;
  }
  // Commands replayed later may need the context current, as wglCreateContextAttribsARB needs
  // a current context to be loaded.
  if (!state->second.LastMakeCurrent) {
    m_Topology.push_back(current->Call);
  }
  state->second.LastMakeCurrent = std::move(current);
}

void StateTrackingService::CreatePbuffer(GITSKey pbuffer, GLsizei width, GLsizei height) {
  m_PbufferSizes[pbuffer] = {width, height};
}

void StateTrackingService::GetPbufferDc(GITSKey pbuffer, HDC dc) {
  if (m_PbufferSizes.contains(pbuffer)) {
    m_PbufferDcs[dc] = pbuffer;
  }
}

void StateTrackingService::ReleasePbufferDc(HDC dc) {
  m_PbufferDcs.erase(dc);
}

void StateTrackingService::DestroyPbuffer(GITSKey pbuffer) {
  m_PbufferSizes.erase(pbuffer);
  std::erase_if(m_PbufferDcs, [pbuffer](const auto& dc) { return dc.second == pbuffer; });
}

std::optional<std::pair<GLsizei, GLsizei>> StateTrackingService::PbufferSize(HDC dc) const {
  const auto pbuffer = m_PbufferDcs.find(dc);
  if (pbuffer == m_PbufferDcs.end()) {
    return std::nullopt;
  }
  const auto size = m_PbufferSizes.find(pbuffer->second);
  if (size == m_PbufferSizes.end()) {
    return std::nullopt;
  }
  return size->second;
}

GITSKey StateTrackingService::CurrentContext(uint32_t threadId) const {
  const auto thread = m_Threads.find(threadId);
  return thread != m_Threads.end() ? thread->second->Context : 0;
}

std::pair<GITSKey, GITSKey> StateTrackingService::CurrentShareGroup(uint32_t threadId) const {
  return {OwnerFor(GLObjectKind::Program, threadId), CurrentContext(threadId)};
}

GITSKey StateTrackingService::OwnerFor(GLObjectKind kind, uint32_t threadId) const {
  const GITSKey context = CurrentContext(threadId);
  return IsContainerKind(kind) ? context : ShareGroupOf(context);
}

GITSKey StateTrackingService::ShareGroupOf(GITSKey context) const {
  const auto state = m_Contexts.find(context);
  return state != m_Contexts.end() ? state->second.ShareGroup : 0;
}

void StateTrackingService::CreateObject(GLObjectKind kind, uint32_t threadId, GITSKey key) {
  if (key == 0 || key == UNMAPPED_OBJECT_NAME_KEY || m_Objects.contains(key)) {
    return;
  }
  m_Objects[key] = {kind, OwnerFor(kind, threadId)};
}

namespace {

// Shaders and programs share one name space, and glDeleteObjectARB deletes either.
bool IsShaderObject(GLObjectKind kind) {
  return kind == GLObjectKind::Program || kind == GLObjectKind::Shader;
}

} // namespace

void StateTrackingService::DeleteObject(
    GLObjectKind kind, GITSKey key, GLuint liveName, ObjectSnapshot snapshot, GITSKey context) {
  const auto object = m_Objects.find(key);
  if (object == m_Objects.end()) {
    return;
  }
  if (object->second.Kind != kind &&
      !(IsShaderObject(object->second.Kind) && IsShaderObject(kind))) {
    return;
  }
  // Deleting a deleted object again does nothing.
  if (object->second.DeletePending) {
    return;
  }
  if ((kind == GLObjectKind::Texture || kind == GLObjectKind::Sampler) &&
      (liveName == 0 || !HasResidentHandle(key))) {
    DeleteHandles(key);
  }
  if (liveName != 0) {
    object->second.DeletePending = true;
    object->second.LiveName = liveName;
    object->second.Snapshot = std::move(snapshot);
    object->second.DeletedIn = context;
    if (object->second.Snapshot.Kept()) {
      m_DeletedNames.emplace(object->second.Owner, kind, liveName);
    }
    return;
  }
  EraseObject(object);
  m_Programs.erase(key);
}

void StateTrackingService::EraseObject(std::map<GITSKey, ObjectState>::iterator object) {
  if (const auto original = m_ViewOriginals.find(object->second.ViewOf);
      original != m_ViewOriginals.end()) {
    m_ViewOriginals.erase(original);
  }
  m_Objects.erase(object);
}

void StateTrackingService::ForgetDeleted(GITSKey key) {
  const auto object = m_Objects.find(key);
  if (object == m_Objects.end() || !object->second.Snapshot.Kept()) {
    return;
  }
  const auto name =
      m_DeletedNames.find({object->second.Owner, object->second.Kind, object->second.LiveName});
  if (name != m_DeletedNames.end()) {
    m_DeletedNames.erase(name);
  }
  if (object->second.Kind == GLObjectKind::Texture ||
      object->second.Kind == GLObjectKind::Sampler) {
    DeleteHandles(key);
  }
  EraseObject(object);
}

void StateTrackingService::ForgetReclaimed(uint32_t threadId, GLuint name) {
  const GITSKey shareGroup = OwnerFor(GLObjectKind::Program, threadId);
  for (auto object = m_Objects.begin(); object != m_Objects.end();) {
    const ObjectState& state = object->second;
    if (state.DeletePending && state.LiveName == name && state.Owner == shareGroup &&
        IsShaderObject(state.Kind)) {
      m_Programs.erase(object->first);
      object = m_Objects.erase(object);
    } else {
      ++object;
    }
  }
}

void StateTrackingService::SetTarget(GITSKey texture, GLenum target) {
  const auto object = m_Objects.find(texture);
  if (object != m_Objects.end() && object->second.Target == 0) {
    object->second.Target = target;
  }
}

void StateTrackingService::SetView(GITSKey texture,
                                   GITSKey original,
                                   std::shared_ptr<StoredCommand> view) {
  const auto object = m_Objects.find(texture);
  if (object != m_Objects.end() && object->second.ViewOf == 0 && original != 0) {
    object->second.View = std::move(view);
    object->second.ViewOf = original;
    m_ViewOriginals.insert(original);
  }
}

ProgramState* StateTrackingService::Program(GITSKey program) {
  if (!m_Objects.contains(program)) {
    return nullptr;
  }
  return &m_Programs[program];
}

const ObjectState* StateTrackingService::Object(GITSKey key) const {
  const auto object = m_Objects.find(key);
  return object != m_Objects.end() ? &object->second : nullptr;
}

const ProgramState* StateTrackingService::FindProgram(GITSKey program) const {
  const auto state = m_Programs.find(program);
  return state != m_Programs.end() ? &state->second : nullptr;
}

void StateTrackingService::CreateSync(uint32_t threadId, GITSKey key) {
  if (key != 0) {
    m_Syncs[key] = OwnerFor(GLObjectKind::Buffer, threadId);
  }
}

void StateTrackingService::DeleteSync(GITSKey key) {
  m_Syncs.erase(key);
}

void StateTrackingService::CreateArbProgram(uint32_t threadId, GITSKey key) {
  if (key != 0 && key != UNMAPPED_OBJECT_NAME_KEY && !m_ArbPrograms.contains(key)) {
    m_ArbPrograms[key].ShareGroup = OwnerFor(GLObjectKind::Program, threadId);
  }
}

void StateTrackingService::SetArbProgramTarget(GITSKey key, GLenum target) {
  const auto program = m_ArbPrograms.find(key);
  if (program != m_ArbPrograms.end()) {
    program->second.Target = target;
  }
}

void StateTrackingService::DeleteArbProgram(GITSKey key) {
  m_ArbPrograms.erase(key);
}

void StateTrackingService::CreateHandle(GLuint64 handle, BindlessHandle state) {
  const auto texture = m_Objects.find(state.Texture);
  if (handle != 0 && texture != m_Objects.end()) {
    m_Handles[{texture->second.Owner, handle}] = std::move(state);
  }
}

void StateTrackingService::MakeResident(uint32_t threadId,
                                        GLuint64 handle,
                                        std::shared_ptr<StoredCommand> command) {
  if (m_Handles.contains({OwnerFor(GLObjectKind::Texture, threadId), handle})) {
    m_ResidentHandles[CurrentContext(threadId)].try_emplace(handle, std::move(command));
  }
}

void StateTrackingService::MakeNonResident(uint32_t threadId, GLuint64 handle) {
  const auto resident = m_ResidentHandles.find(CurrentContext(threadId));
  if (resident != m_ResidentHandles.end()) {
    resident->second.erase(handle);
  }
}

bool StateTrackingService::HasResidentHandle(GITSKey object) const {
  for (const auto& [context, resident] : m_ResidentHandles) {
    const GITSKey shareGroup = ShareGroupOf(context);
    for (const auto& [handle, command] : resident) {
      const auto state = m_Handles.find({shareGroup, handle});
      if (state != m_Handles.end() &&
          (state->second.Texture == object || state->second.Sampler == object)) {
        return true;
      }
    }
  }
  return false;
}

void StateTrackingService::DeleteHandles(GITSKey object) {
  for (auto handle = m_Handles.begin(); handle != m_Handles.end();) {
    if (handle->second.Texture != object && handle->second.Sampler != object) {
      ++handle;
      continue;
    }
    for (auto& [context, resident] : m_ResidentHandles) {
      if (ShareGroupOf(context) == handle->first.first) {
        resident.erase(handle->first.second);
      }
    }
    handle = m_Handles.erase(handle);
  }
}

void StateTrackingService::BeginList(uint32_t threadId,
                                     GLuint list,
                                     ListCompileState compileState,
                                     std::shared_ptr<StoredCommand> newList) {
  auto& compiling = m_CompilingLists[threadId];
  compiling.first = {OwnerFor(GLObjectKind::Program, threadId), list};
  compiling.second.Commands = std::move(compileState.Commands);
  compiling.second.Commands.push_back(std::move(newList));
  compiling.second.Buffers = std::move(compileState.Buffers);
}

void StateTrackingService::AddListCommand(uint32_t threadId,
                                          std::shared_ptr<StoredCommand> command) {
  const auto compiling = m_CompilingLists.find(threadId);
  if (compiling != m_CompilingLists.end()) {
    compiling->second.second.Commands.push_back(std::move(command));
  }
}

void StateTrackingService::EndList(uint32_t threadId, ListCompileState reset) {
  const auto compiling = m_CompilingLists.find(threadId);
  if (compiling == m_CompilingLists.end()) {
    return;
  }
  DisplayList& list = compiling->second.second;
  list.Commands.insert(list.Commands.end(), std::make_move_iterator(reset.Commands.begin()),
                       std::make_move_iterator(reset.Commands.end()));
  m_DisplayLists[compiling->second.first] = std::move(list);
  m_CompilingLists.erase(compiling);
}

// Font lists are restored before the display lists, so a list compiled into the range of an
// earlier font replaces the font's list, and a font replaces the lists compiled into its range
// before.
void StateTrackingService::CreateFontLists(uint32_t threadId,
                                           GLuint first,
                                           GLuint count,
                                           std::shared_ptr<StoredCommand> command) {
  const GITSKey shareGroup = OwnerFor(GLObjectKind::Program, threadId);
  const uint64_t end = uint64_t{first} + count;
  std::erase_if(m_DisplayLists, [&](const auto& entry) {
    return entry.first.first == shareGroup && entry.first.second >= first &&
           entry.first.second < end;
  });
  std::erase_if(m_Fonts, [&](const FontLists& font) {
    return font.ShareGroup == shareGroup && font.First >= first &&
           uint64_t{font.First} + font.Count <= end;
  });
  m_Fonts.push_back({shareGroup, first, count, {std::move(command)}});
}

void StateTrackingService::DeleteLists(uint32_t threadId,
                                       GLuint list,
                                       GLsizei range,
                                       std::shared_ptr<StoredCommand> command) {
  const GITSKey shareGroup = OwnerFor(GLObjectKind::Program, threadId);
  const uint64_t end = uint64_t{list} + static_cast<uint64_t>(std::max<GLsizei>(range, 0));
  std::erase_if(m_DisplayLists, [&](const auto& entry) {
    return entry.first.first == shareGroup && entry.first.second >= list &&
           entry.first.second < end;
  });
  std::erase_if(m_Fonts, [&](const FontLists& font) {
    return font.ShareGroup == shareGroup && font.First >= list &&
           uint64_t{font.First} + font.Count <= end;
  });
  for (FontLists& font : m_Fonts) {
    if (font.ShareGroup == shareGroup && font.First < end &&
        list < uint64_t{font.First} + font.Count) {
      font.Commands.push_back(command);
    }
  }
}

} // namespace gits::opengl2
