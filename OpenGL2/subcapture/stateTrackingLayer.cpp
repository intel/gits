// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateTrackingLayer.h"

#include "commandSerializersAuto.h"
#include "handleArgumentUpdaters.h"
#include "liveGl.h"
#include "pixelFormatMetaTypes.h"
#include "stateRestorer.h"
#include "uniformLocationMapService.h"

#include <algorithm>
#include <string>
#include <vector>

namespace gits::opengl2 {

namespace {

template <typename SerializerT, typename CommandT>
SampledCommand Sampled(StateTrackingService& state, const CommandT& command) {
  return state.Sample(Store<SerializerT>(command));
}

template <typename SerializerT, typename CommandT>
SampledCommand Unsampled(const CommandT& command) {
  return {nullptr, Store<SerializerT>(command)};
}

void CreateObjects(StateTrackingService& state,
                   GLObjectKind kind,
                   uint32_t threadId,
                   const HandleArrayArgument<GLuint>& names) {
  for (GITSKey key : names.Key) {
    state.CreateObject(kind, threadId, key);
  }
}

void DeleteObjects(StateTrackingService& state,
                   GLObjectKind kind,
                   const HandleArrayArgument<GLuint>& names) {
  for (GITSKey key : names.Key) {
    state.DeleteObject(kind, key);
  }
}

// Shaders attached at link, by type and source, so a program whose shaders are detached or
// changed afterwards can be linked again.
std::vector<std::pair<GLenum, std::string>> AttachedShaders(GLuint program) {
  GLint count = 0;
  GL_FN(glGetProgramiv)(program, GL_ATTACHED_SHADERS, &count);
  std::vector<GLuint> shaders(static_cast<size_t>(count > 0 ? count : 0));
  GLsizei written = 0;
  if (!shaders.empty()) {
    GL_FN(glGetAttachedShaders)(program, count, &written, shaders.data());
  }
  std::vector<std::pair<GLenum, std::string>> linked;
  for (GLsizei i = 0; i < written; ++i) {
    GLint type = 0;
    GL_FN(glGetShaderiv)(shaders[i], GL_SHADER_TYPE, &type);
    linked.emplace_back(static_cast<GLenum>(type), ShaderSource(shaders[i]));
  }
  return linked;
}

bool Linked(GLuint program) {
  GLint linked = GL_FALSE;
  GL_FN(glGetProgramiv)(program, GL_LINK_STATUS, &linked);
  return linked != GL_FALSE;
}

// A successful link or binary load replaces the program's executable. After a failed one, a
// program in use keeps using the previous executable, so that one stays tracked.
void ReplaceExecutable(ProgramState& tracked, GITSKey key) {
  // A program glCreateShaderProgramv created is restored like any other once linked again, and
  // stays separable.
  if (tracked.Creation) {
    glProgramParameteriCommand separable;
    separable.m_program.Key = key;
    separable.m_pname.Value = GL_PROGRAM_SEPARABLE;
    separable.m_value.Value = GL_TRUE;
    tracked.LinkSetup.insert(tracked.LinkSetup.begin(),
                             Store<glProgramParameteriCommandSerializer>(separable));
    tracked.Creation = nullptr;
  }
  tracked.Binary = nullptr;
  tracked.HandleUniforms.clear();
  tracked.LinkedShaders.clear();
  tracked.LinkedSetupCount = tracked.LinkSetup.size();
}

void SnapshotLink(StateTrackingService& state, GITSKey key, GLuint program) {
  ProgramState* tracked = state.Program(key);
  if (tracked != nullptr && Linked(program)) {
    ReplaceExecutable(*tracked, key);
    tracked->LinkedShaders = AttachedShaders(program);
  }
}

// A shader still attached, or a program still in use, stays live after its deletion.
GLuint PendingName(GLuint name) {
  return name != 0 && (GL_FN(glIsShader)(name) || GL_FN(glIsProgram)(name)) ? name : 0;
}

template <typename SerializerT, typename CommandT>
void AddLinkSetup(StateTrackingService& state, GITSKey key, const CommandT& command) {
  if (ProgramState* tracked = state.Program(key)) {
    tracked->LinkSetup.push_back(Store<SerializerT>(command));
  }
}

} // namespace

// ---- Window geometry and pixel formats ----------------------------------

void StateTrackingLayer::Post(WindowGeometryMetaCommand& command) {
  m_State.SetGeometry(command);
}

void StateTrackingLayer::Post(PixelFormatMetaCommand& command) {
  m_State.AddTopology(Unsampled<PixelFormatMetaSerializer>(command));
}

void StateTrackingLayer::Post(wglSetPixelFormatCommand& command) {
  m_State.AddTopology(Sampled<wglSetPixelFormatCommandSerializer>(m_State, command));
}

// ---- Contexts -----------------------------------------------------------

void StateTrackingLayer::Post(wglCreateContextCommand& command) {
  m_State.AddTopology(Sampled<wglCreateContextCommandSerializer>(m_State, command));
  m_State.CreateContext(command.m_Return.Key, 0);
}

void StateTrackingLayer::Post(wglCreateContextAttribsARBCommand& command) {
  m_State.AddTopology(Sampled<wglCreateContextAttribsARBCommandSerializer>(m_State, command));
  m_State.CreateContext(command.m_Return.Key, command.m_hShareContext.Key);
}

void StateTrackingLayer::Post(wglShareListsCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglShareListsCommandSerializer>(command));
  m_State.ShareLists(command.m_hrcSrvShare.Key, command.m_hrcSrvSource.Key);
}

void StateTrackingLayer::Post(wglDeleteContextCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglDeleteContextCommandSerializer>(command));
  m_State.DeleteContext(command.m_oldContext.Key);
}

void StateTrackingLayer::Post(wglMakeCurrentCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrent(command.m_ThreadId, command.m_newContext.Key, command.m_hDc.Value,
                     command.m_hDc.Value,
                     Sampled<wglMakeCurrentCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(wglMakeContextCurrentARBCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrent(command.m_ThreadId, command.m_hglrc.Key, command.m_hDrawDC.Value,
                     command.m_hReadDC.Value,
                     Sampled<wglMakeContextCurrentARBCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrent(command.m_ThreadId, command.m_hglrc.Key, command.m_hDrawDC.Value,
                     command.m_hReadDC.Value,
                     Sampled<wglMakeContextCurrentEXTCommandSerializer>(m_State, command));
}

// ---- Pbuffers -----------------------------------------------------------

void StateTrackingLayer::Post(wglCreatePbufferARBCommand& command) {
  m_State.AddTopology(Unsampled<wglCreatePbufferARBCommandSerializer>(command));
  m_State.CreatePbuffer(command.m_Return.Key, command.m_iWidth.Value, command.m_iHeight.Value);
}

void StateTrackingLayer::Post(wglCreatePbufferEXTCommand& command) {
  m_State.AddTopology(Unsampled<wglCreatePbufferEXTCommandSerializer>(command));
  m_State.CreatePbuffer(command.m_Return.Key, command.m_iWidth.Value, command.m_iHeight.Value);
}

void StateTrackingLayer::Post(wglGetPbufferDCARBCommand& command) {
  m_State.AddTopology(Unsampled<wglGetPbufferDCARBCommandSerializer>(command));
  m_State.GetPbufferDc(command.m_hPbuffer.Key, command.m_Return.Value);
}

void StateTrackingLayer::Post(wglGetPbufferDCEXTCommand& command) {
  m_State.AddTopology(Unsampled<wglGetPbufferDCEXTCommandSerializer>(command));
  m_State.GetPbufferDc(command.m_hPbuffer.Key, command.m_Return.Value);
}

void StateTrackingLayer::Post(wglReleasePbufferDCARBCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglReleasePbufferDCARBCommandSerializer>(command));
  m_State.ReleasePbufferDc(command.m_hDC.Value);
}

void StateTrackingLayer::Post(wglReleasePbufferDCEXTCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglReleasePbufferDCEXTCommandSerializer>(command));
  m_State.ReleasePbufferDc(command.m_hDC.Value);
}

void StateTrackingLayer::Post(wglDestroyPbufferARBCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglDestroyPbufferARBCommandSerializer>(command));
  m_State.DestroyPbuffer(command.m_hPbuffer.Key);
}

void StateTrackingLayer::Post(wglDestroyPbufferEXTCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.AddTopology(Unsampled<wglDestroyPbufferEXTCommandSerializer>(command));
  m_State.DestroyPbuffer(command.m_hPbuffer.Key);
}

void StateTrackingLayer::DeleteReadBack(GLObjectKind kind,
                                        uint32_t threadId,
                                        const HandleArrayArgument<GLuint>& names) {
  const bool beforeRange = m_Range.CurrentFrame() < m_Range.StartFrame();
  const GITSKey context = m_State.CurrentShareGroup(threadId).second;
  for (GITSKey key : names.Key) {
    ObjectSnapshot snapshot;
    if (beforeRange) {
      snapshot = StateRestorer(m_State, m_Recorder, m_Analysis).CaptureDeleted(threadId, kind, key);
    }
    const GLuint name = snapshot.Kept() ? ObjectNameMapService::Get().TryGetHandle(kind, key) : 0;
    m_State.DeleteObject(kind, key, name, std::move(snapshot), context);
  }
}

template <typename GenFn>
void StateTrackingLayer::Rename(
    GLObjectKind kind, uint32_t threadId, GITSKey key, GLuint& name, GenFn gen) {
  if (m_Range.CurrentFrame() >= m_Range.StartFrame()) {
    return;
  }
  const GITSKey shareGroup = m_State.CurrentShareGroup(threadId).first;
  const GLuint given = name;
  // The name GL gave stays in use, so GL does not give it again.
  while (name != 0 && m_State.IsDeletedName(shareGroup, kind, name)) {
    name = 0;
    gen(1, &name);
  }
  if (name != given) {
    ObjectNameMapService::Get().SetHandle(kind, key, name);
  }
}

template <typename GenFn>
void StateTrackingLayer::Rename(GLObjectKind kind,
                                uint32_t threadId,
                                HandleArrayArgument<GLuint>& names,
                                GenFn gen) {
  for (size_t i = 0; i < names.Key.size() && i < names.Value.size(); ++i) {
    Rename(kind, threadId, names.Key[i], names.Value[i], gen);
  }
}

void StateTrackingLayer::ForgetDetached(uint32_t threadId) {
  if (m_Range.CurrentFrame() >= m_Range.StartFrame()) {
    return;
  }
  for (GITSKey key : StateRestorer(m_State, m_Recorder, m_Analysis).DetachedDeleted(threadId)) {
    if (const GLuint storage = m_State.Object(key)->Snapshot.Storage; storage != 0) {
      GL_FN(glDeleteTextures)(1, &storage);
    }
    m_State.ForgetDeleted(key);
  }
}

// ---- Frames -------------------------------------------------------------

void StateTrackingLayer::Post(wglSwapBuffersCommand& command) {
  ForgetDetached(command.m_ThreadId);
}

void StateTrackingLayer::Post(wglSwapLayerBuffersCommand& command) {
  ForgetDetached(command.m_ThreadId);
}

// ---- Buffers ------------------------------------------------------------

void StateTrackingLayer::Post(glGenBuffersCommand& command) {
  Rename(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers, GL_FN(glGenBuffers));
  CreateObjects(m_State, GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers);
}

void StateTrackingLayer::Post(glGenBuffersARBCommand& command) {
  Rename(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers, GL_FN(glGenBuffersARB));
  CreateObjects(m_State, GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers);
}

void StateTrackingLayer::Post(glCreateBuffersCommand& command) {
  Rename(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers, GL_FN(glCreateBuffers));
  CreateObjects(m_State, GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers);
}

// A name bound for the first time creates the object.
void StateTrackingLayer::Pre(glBindBufferCommand& command) {
  Rename(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffer.Key, command.m_buffer.Value,
         GL_FN(glGenBuffers));
}

void StateTrackingLayer::Post(glBindBufferCommand& command) {
  m_State.CreateObject(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffer.Key);
}

void StateTrackingLayer::Pre(glBindBufferARBCommand& command) {
  Rename(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffer.Key, command.m_buffer.Value,
         GL_FN(glGenBuffersARB));
}

void StateTrackingLayer::Post(glBindBufferARBCommand& command) {
  m_State.CreateObject(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffer.Key);
}

void StateTrackingLayer::Pre(glDeleteBuffersCommand& command) {
  DeleteReadBack(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers);
}

void StateTrackingLayer::Pre(glDeleteBuffersARBCommand& command) {
  DeleteReadBack(GLObjectKind::Buffer, command.m_ThreadId, command.m_buffers);
}

void StateTrackingLayer::Post(glFreeObjectBufferATICommand& command) {
  m_State.DeleteObject(GLObjectKind::Buffer, command.m_buffer.Key);
}

// ---- Textures -----------------------------------------------------------

void StateTrackingLayer::Post(glGenTexturesCommand& command) {
  Rename(GLObjectKind::Texture, command.m_ThreadId, command.m_textures, GL_FN(glGenTextures));
  CreateObjects(m_State, GLObjectKind::Texture, command.m_ThreadId, command.m_textures);
}

void StateTrackingLayer::Post(glGenTexturesEXTCommand& command) {
  Rename(GLObjectKind::Texture, command.m_ThreadId, command.m_textures, GL_FN(glGenTexturesEXT));
  CreateObjects(m_State, GLObjectKind::Texture, command.m_ThreadId, command.m_textures);
}

void StateTrackingLayer::Post(glCreateTexturesCommand& command) {
  const auto create = [target = command.m_target.Value](GLsizei n, GLuint* textures) {
    GL_FN(glCreateTextures)(target, n, textures);
  };
  Rename(GLObjectKind::Texture, command.m_ThreadId, command.m_textures, create);
  for (GITSKey key : command.m_textures.Key) {
    m_State.CreateObject(GLObjectKind::Texture, command.m_ThreadId, key);
    m_State.SetTarget(key, command.m_target.Value);
  }
}

void StateTrackingLayer::Pre(glBindTextureCommand& command) {
  Rename(GLObjectKind::Texture, command.m_ThreadId, command.m_texture.Key, command.m_texture.Value,
         GL_FN(glGenTextures));
}

void StateTrackingLayer::Post(glBindTextureCommand& command) {
  m_State.CreateObject(GLObjectKind::Texture, command.m_ThreadId, command.m_texture.Key);
  m_State.SetTarget(command.m_texture.Key, command.m_target.Value);
}

void StateTrackingLayer::Pre(glBindTextureEXTCommand& command) {
  Rename(GLObjectKind::Texture, command.m_ThreadId, command.m_texture.Key, command.m_texture.Value,
         GL_FN(glGenTexturesEXT));
}

void StateTrackingLayer::Post(glBindTextureEXTCommand& command) {
  m_State.CreateObject(GLObjectKind::Texture, command.m_ThreadId, command.m_texture.Key);
  m_State.SetTarget(command.m_texture.Key, command.m_target.Value);
}

void StateTrackingLayer::Post(glBindMultiTextureEXTCommand& command) {
  m_State.SetTarget(command.m_texture.Key, command.m_target.Value);
}

template <typename SerializerT, typename CommandT>
void StateTrackingLayer::SetView(const CommandT& command) {
  if (!m_ViewNameUnused || !GL_FN(glIsTexture)(command.m_texture.Value)) {
    return;
  }
  m_State.SetTarget(command.m_texture.Key, command.m_target.Value);
  m_State.SetView(command.m_texture.Key, command.m_origtexture.Key, Store<SerializerT>(command));
}

void StateTrackingLayer::Pre(glTextureViewCommand& command) {
  m_ViewNameUnused = !GL_FN(glIsTexture)(command.m_texture.Value);
}

void StateTrackingLayer::Post(glTextureViewCommand& command) {
  SetView<glTextureViewCommandSerializer>(command);
}

void StateTrackingLayer::Pre(glTextureViewEXTCommand& command) {
  m_ViewNameUnused = !GL_FN(glIsTexture)(command.m_texture.Value);
}

void StateTrackingLayer::Post(glTextureViewEXTCommand& command) {
  SetView<glTextureViewEXTCommandSerializer>(command);
}

void StateTrackingLayer::Pre(glTextureViewOESCommand& command) {
  m_ViewNameUnused = !GL_FN(glIsTexture)(command.m_texture.Value);
}

void StateTrackingLayer::Post(glTextureViewOESCommand& command) {
  SetView<glTextureViewOESCommandSerializer>(command);
}

void StateTrackingLayer::Pre(glDeleteTexturesCommand& command) {
  DeleteReadBack(GLObjectKind::Texture, command.m_ThreadId, command.m_textures);
}

void StateTrackingLayer::Pre(glDeleteTexturesEXTCommand& command) {
  DeleteReadBack(GLObjectKind::Texture, command.m_ThreadId, command.m_textures);
}

// ---- Renderbuffers ------------------------------------------------------

void StateTrackingLayer::Post(glGenRenderbuffersCommand& command) {
  Rename(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers,
         GL_FN(glGenRenderbuffers));
  CreateObjects(m_State, GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Post(glGenRenderbuffersEXTCommand& command) {
  Rename(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers,
         GL_FN(glGenRenderbuffersEXT));
  CreateObjects(m_State, GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Post(glGenRenderbuffersOESCommand& command) {
  Rename(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers,
         GL_FN(glGenRenderbuffersOES));
  CreateObjects(m_State, GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Post(glCreateRenderbuffersCommand& command) {
  Rename(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers,
         GL_FN(glCreateRenderbuffers));
  CreateObjects(m_State, GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Pre(glBindRenderbufferEXTCommand& command) {
  Rename(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffer.Key,
         command.m_renderbuffer.Value, GL_FN(glGenRenderbuffersEXT));
}

void StateTrackingLayer::Post(glBindRenderbufferEXTCommand& command) {
  m_State.CreateObject(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffer.Key);
}

void StateTrackingLayer::Pre(glDeleteRenderbuffersCommand& command) {
  DeleteReadBack(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Pre(glDeleteRenderbuffersEXTCommand& command) {
  DeleteReadBack(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

void StateTrackingLayer::Pre(glDeleteRenderbuffersOESCommand& command) {
  DeleteReadBack(GLObjectKind::Renderbuffer, command.m_ThreadId, command.m_renderbuffers);
}

// ---- Samplers -----------------------------------------------------------

void StateTrackingLayer::Post(glGenSamplersCommand& command) {
  Rename(GLObjectKind::Sampler, command.m_ThreadId, command.m_samplers, GL_FN(glGenSamplers));
  CreateObjects(m_State, GLObjectKind::Sampler, command.m_ThreadId, command.m_samplers);
}

void StateTrackingLayer::Post(glCreateSamplersCommand& command) {
  Rename(GLObjectKind::Sampler, command.m_ThreadId, command.m_samplers, GL_FN(glCreateSamplers));
  CreateObjects(m_State, GLObjectKind::Sampler, command.m_ThreadId, command.m_samplers);
}

void StateTrackingLayer::Pre(glDeleteSamplersCommand& command) {
  DeleteReadBack(GLObjectKind::Sampler, command.m_ThreadId, command.m_samplers);
}

// ---- Bindless handles ---------------------------------------------------

namespace {

// The runner overwrites m_Return with the replay handle, so the recorded one is read in Pre.
template <typename SerializerT, typename CommandT>
void CreateHandle(StateTrackingService& state, const CommandT& command, GITSKey sampler = 0) {
  state.CreateHandle(command.m_Return.Value,
                     {command.m_texture.Key, sampler, Store<SerializerT>(command)});
}

template <typename SerializerT, typename CommandT>
void MakeResident(StateTrackingService& state, const CommandT& command) {
  state.MakeResident(command.m_ThreadId, command.m_handle.Value, Store<SerializerT>(command));
}

} // namespace

void StateTrackingLayer::Pre(glGetTextureHandleARBCommand& command) {
  CreateHandle<glGetTextureHandleARBCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Pre(glGetTextureHandleNVCommand& command) {
  CreateHandle<glGetTextureHandleNVCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Pre(glGetTextureSamplerHandleARBCommand& command) {
  CreateHandle<glGetTextureSamplerHandleARBCommandSerializer>(m_State, command,
                                                              command.m_sampler.Key);
}

void StateTrackingLayer::Pre(glGetTextureHandleIMGCommand& command) {
  CreateHandle<glGetTextureHandleIMGCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Pre(glGetTextureSamplerHandleNVCommand& command) {
  CreateHandle<glGetTextureSamplerHandleNVCommandSerializer>(m_State, command,
                                                             command.m_sampler.Key);
}

void StateTrackingLayer::Pre(glGetTextureSamplerHandleIMGCommand& command) {
  CreateHandle<glGetTextureSamplerHandleIMGCommandSerializer>(m_State, command,
                                                              command.m_sampler.Key);
}

void StateTrackingLayer::Pre(glGetImageHandleARBCommand& command) {
  CreateHandle<glGetImageHandleARBCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Pre(glGetImageHandleNVCommand& command) {
  CreateHandle<glGetImageHandleNVCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Post(glMakeTextureHandleResidentARBCommand& command) {
  MakeResident<glMakeTextureHandleResidentARBCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Post(glMakeTextureHandleResidentNVCommand& command) {
  MakeResident<glMakeTextureHandleResidentNVCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Post(glMakeTextureHandleNonResidentARBCommand& command) {
  m_State.MakeNonResident(command.m_ThreadId, command.m_handle.Value);
}

void StateTrackingLayer::Post(glMakeTextureHandleNonResidentNVCommand& command) {
  m_State.MakeNonResident(command.m_ThreadId, command.m_handle.Value);
}

void StateTrackingLayer::Post(glMakeImageHandleResidentARBCommand& command) {
  MakeResident<glMakeImageHandleResidentARBCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Post(glMakeImageHandleResidentNVCommand& command) {
  MakeResident<glMakeImageHandleResidentNVCommandSerializer>(m_State, command);
}

void StateTrackingLayer::Post(glMakeImageHandleNonResidentARBCommand& command) {
  m_State.MakeNonResident(command.m_ThreadId, command.m_handle.Value);
}

void StateTrackingLayer::Post(glMakeImageHandleNonResidentNVCommand& command) {
  m_State.MakeNonResident(command.m_ThreadId, command.m_handle.Value);
}

namespace {

// glUniform* calls set uniforms of the program in use, or without one, of the active program of
// the bound program pipeline.
GITSKey ProgramInUse() {
  return UniformLocationMapService::Get().GetUniformProgram(CurrentContext());
}

// The locations a call with `count` values sets from the live `location` on. GL ignores values
// past the end of the array `location` is an element of, so the following uniforms keep theirs.
std::vector<GLint> SetLocations(GITSKey program, GLint location, GLsizei count) {
  std::vector<GLint> set{location};
  const GLuint name = ObjectNameMapService::Get().TryGetHandle(GLObjectKind::Program, program);
  GLint linked = GL_FALSE;
  if (name != 0) {
    GL_FN(glGetProgramiv)(name, GL_LINK_STATUS, &linked);
  }
  if (linked == GL_FALSE) {
    return set;
  }
  GLint uniforms = 0;
  GLint maxLength = 0;
  GL_FN(glGetProgramiv)(name, GL_ACTIVE_UNIFORMS, &uniforms);
  GL_FN(glGetProgramiv)(name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
  std::vector<GLchar> buffer(static_cast<size_t>(maxLength > 0 ? maxLength : 1));
  for (GLint i = 0; i < uniforms; ++i) {
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    GL_FN(glGetActiveUniform)
    (name, static_cast<GLuint>(i), static_cast<GLsizei>(buffer.size()), &length, &size, &type,
     buffer.data());
    const std::string uniform(buffer.data(), static_cast<size_t>(length));
    if (size < 2 || !uniform.ends_with("[0]")) {
      continue;
    }
    const std::string base = uniform.substr(0, uniform.size() - 3);
    std::vector<GLint> elements;
    for (GLint element = 0; element < size; ++element) {
      const std::string elementName = base + "[" + std::to_string(element) + "]";
      elements.push_back(GL_FN(glGetUniformLocation)(name, elementName.c_str()));
    }
    const auto start = std::find(elements.begin(), elements.end(), location);
    if (start != elements.end()) {
      const auto end = elements.end() - start > count ? start + count : elements.end();
      return std::vector<GLint>(start, end);
    }
  }
  return set;
}

// Forgets the handle setters of the uniforms a call with `count` values sets from the captured
// `location` on. Several captured locations can resolve to the same uniform, so setters are
// matched by the resolved location.
void EraseHandleUniforms(ProgramState& tracked, GITSKey program, GLint location, GLsizei count) {
  auto& locations = UniformLocationMapService::Get();
  const GLint first = locations.ResolveForProgram(program, location);
  const auto resolve = [&](const auto& setter) {
    return locations.ResolveForProgram(program, setter.first);
  };
  // Only a setter past the first location needs the program's arrays.
  const bool past = std::any_of(tracked.HandleUniforms.begin(), tracked.HandleUniforms.end(),
                                [&](const auto& setter) {
                                  const GLint resolved = resolve(setter);
                                  return resolved > first && resolved - first < count;
                                });
  const std::vector<GLint> set =
      past ? SetLocations(program, first, count) : std::vector<GLint>{first};
  std::erase_if(tracked.HandleUniforms, [&](const auto& setter) {
    return std::find(set.begin(), set.end(), resolve(setter)) != set.end();
  });
}

// GL ignores location -1, so it changes no uniform.
template <typename SerializerT, typename CommandT>
void SetHandleUniform(StateTrackingService& state, GITSKey program, const CommandT& command) {
  ProgramState* tracked = state.Program(program);
  if (tracked == nullptr || command.m_location.Value < 0) {
    return;
  }
  EraseHandleUniforms(*tracked, program, command.m_location.Value, 1);
  tracked->HandleUniforms[command.m_location.Value] = Store<SerializerT>(command);
}

// A sampler or image uniform set to a texture unit no longer holds a handle. A call with no
// values or a negative count changes no uniform.
void SetUnitUniforms(StateTrackingService& state, GITSKey program, GLint location, GLsizei count) {
  ProgramState* tracked = state.Program(program);
  if (tracked == nullptr || tracked->HandleUniforms.empty() || location < 0 || count <= 0) {
    return;
  }
  EraseHandleUniforms(*tracked, program, location, count);
}

} // namespace

void StateTrackingLayer::Post(glUniformHandleui64ARBCommand& command) {
  SetHandleUniform<glUniformHandleui64ARBCommandSerializer>(m_State, ProgramInUse(), command);
}

void StateTrackingLayer::Post(glUniformHandleui64NVCommand& command) {
  SetHandleUniform<glUniformHandleui64NVCommandSerializer>(m_State, ProgramInUse(), command);
}

void StateTrackingLayer::Post(glUniformHandleui64IMGCommand& command) {
  SetHandleUniform<glUniformHandleui64IMGCommandSerializer>(m_State, ProgramInUse(), command);
}

void StateTrackingLayer::Post(glProgramUniformHandleui64ARBCommand& command) {
  SetHandleUniform<glProgramUniformHandleui64ARBCommandSerializer>(m_State, command.m_program.Key,
                                                                   command);
}

void StateTrackingLayer::Post(glProgramUniformHandleui64NVCommand& command) {
  SetHandleUniform<glProgramUniformHandleui64NVCommandSerializer>(m_State, command.m_program.Key,
                                                                  command);
}

void StateTrackingLayer::Post(glProgramUniformHandleui64IMGCommand& command) {
  SetHandleUniform<glProgramUniformHandleui64IMGCommandSerializer>(m_State, command.m_program.Key,
                                                                   command);
}

void StateTrackingLayer::Post(glUniform1iCommand& command) {
  SetUnitUniforms(m_State, ProgramInUse(), command.m_location.Value, 1);
}

void StateTrackingLayer::Post(glUniform1iARBCommand& command) {
  SetUnitUniforms(m_State, ProgramInUse(), command.m_location.Value, 1);
}

void StateTrackingLayer::Post(glUniform1ivCommand& command) {
  SetUnitUniforms(m_State, ProgramInUse(), command.m_location.Value, command.m_count.Value);
}

void StateTrackingLayer::Post(glUniform1ivARBCommand& command) {
  SetUnitUniforms(m_State, ProgramInUse(), command.m_location.Value, command.m_count.Value);
}

void StateTrackingLayer::Post(glProgramUniform1iCommand& command) {
  SetUnitUniforms(m_State, command.m_program.Key, command.m_location.Value, 1);
}

void StateTrackingLayer::Post(glProgramUniform1iEXTCommand& command) {
  SetUnitUniforms(m_State, command.m_program.Key, command.m_location.Value, 1);
}

void StateTrackingLayer::Post(glProgramUniform1ivCommand& command) {
  SetUnitUniforms(m_State, command.m_program.Key, command.m_location.Value, command.m_count.Value);
}

void StateTrackingLayer::Post(glProgramUniform1ivEXTCommand& command) {
  SetUnitUniforms(m_State, command.m_program.Key, command.m_location.Value, command.m_count.Value);
}

// ---- Shaders and programs -----------------------------------------------

void StateTrackingLayer::Post(glCreateShaderCommand& command) {
  m_State.ForgetReclaimed(command.m_ThreadId, command.m_Return.Value);
  m_State.CreateObject(GLObjectKind::Shader, command.m_ThreadId, command.m_Return.Key);
}

void StateTrackingLayer::Post(glCreateShaderObjectARBCommand& command) {
  m_State.ForgetReclaimed(command.m_ThreadId, static_cast<GLuint>(command.m_Return.Value));
  m_State.CreateObject(GLObjectKind::Shader, command.m_ThreadId, command.m_Return.Key);
}

void StateTrackingLayer::Post(glDeleteShaderCommand& command) {
  m_State.DeleteObject(GLObjectKind::Shader, command.m_shader.Key,
                       PendingName(command.m_shader.Value));
}

void StateTrackingLayer::Post(glCreateProgramCommand& command) {
  m_State.ForgetReclaimed(command.m_ThreadId, command.m_Return.Value);
  m_State.CreateObject(GLObjectKind::Program, command.m_ThreadId, command.m_Return.Key);
}

void StateTrackingLayer::Post(glCreateProgramObjectARBCommand& command) {
  m_State.ForgetReclaimed(command.m_ThreadId, static_cast<GLuint>(command.m_Return.Value));
  m_State.CreateObject(GLObjectKind::Program, command.m_ThreadId, command.m_Return.Key);
}

void StateTrackingLayer::Post(glCreateShaderProgramvCommand& command) {
  m_State.ForgetReclaimed(command.m_ThreadId, command.m_Return.Value);
  m_State.CreateObject(GLObjectKind::Program, command.m_ThreadId, command.m_Return.Key);
  if (ProgramState* program = m_State.Program(command.m_Return.Key)) {
    program->Creation = Store<glCreateShaderProgramvCommandSerializer>(command);
  }
}

void StateTrackingLayer::Post(glDeleteProgramCommand& command) {
  m_State.DeleteObject(GLObjectKind::Program, command.m_program.Key,
                       PendingName(command.m_program.Value));
}

void StateTrackingLayer::Post(glDeleteObjectARBCommand& command) {
  m_State.DeleteObject(GLObjectKind::Program, command.m_obj.Key,
                       PendingName(static_cast<GLuint>(command.m_obj.Value)));
}

void StateTrackingLayer::Post(glProgramBinaryCommand& command) {
  ProgramState* tracked = m_State.Program(command.m_program.Key);
  if (tracked != nullptr && Linked(command.m_program.Value)) {
    ReplaceExecutable(*tracked, command.m_program.Key);
    tracked->Binary = Store<glProgramBinaryCommandSerializer>(command);
  }
}

void StateTrackingLayer::Post(glLinkProgramCommand& command) {
  SnapshotLink(m_State, command.m_program.Key, command.m_program.Value);
}

void StateTrackingLayer::Post(glLinkProgramARBCommand& command) {
  SnapshotLink(m_State, command.m_programObj.Key, command.m_programObj.Value);
}

void StateTrackingLayer::Post(glBindFragDataLocationCommand& command) {
  AddLinkSetup<glBindFragDataLocationCommandSerializer>(m_State, command.m_program.Key, command);
}

void StateTrackingLayer::Post(glBindFragDataLocationEXTCommand& command) {
  AddLinkSetup<glBindFragDataLocationEXTCommandSerializer>(m_State, command.m_program.Key, command);
}

void StateTrackingLayer::Post(glTransformFeedbackVaryingsCommand& command) {
  AddLinkSetup<glTransformFeedbackVaryingsCommandSerializer>(m_State, command.m_program.Key,
                                                             command);
}

void StateTrackingLayer::Post(glProgramParameteriCommand& command) {
  AddLinkSetup<glProgramParameteriCommandSerializer>(m_State, command.m_program.Key, command);
}

void StateTrackingLayer::Post(glProgramParameteriARBCommand& command) {
  AddLinkSetup<glProgramParameteriARBCommandSerializer>(m_State, command.m_program.Key, command);
}

void StateTrackingLayer::Post(glProgramParameteriEXTCommand& command) {
  AddLinkSetup<glProgramParameteriEXTCommandSerializer>(m_State, command.m_program.Key, command);
}

// ---- ARB assembly programs ----------------------------------------------

void StateTrackingLayer::Post(glGenProgramsARBCommand& command) {
  for (GITSKey key : command.m_programs.Key) {
    m_State.CreateArbProgram(command.m_ThreadId, key);
  }
}

void StateTrackingLayer::Post(glBindProgramARBCommand& command) {
  m_State.CreateArbProgram(command.m_ThreadId, command.m_program.Key);
  m_State.SetArbProgramTarget(command.m_program.Key, command.m_target.Value);
}

void StateTrackingLayer::Post(glDeleteProgramsARBCommand& command) {
  for (GITSKey key : command.m_programs.Key) {
    m_State.DeleteArbProgram(key);
  }
}

// ---- Display lists ------------------------------------------------------

// GL_COMPILE_AND_EXECUTE already took effect before the range, so restore only compiles.
void StateTrackingLayer::Post(glNewListCommand& command) {
  glNewListCommand compile = command;
  compile.m_mode.Value = GL_COMPILE;
  m_State.BeginList(
      command.m_ThreadId, command.m_list.Value,
      StateRestorer(m_State, m_Recorder, m_Analysis).CaptureListCompileState(command.m_ThreadId),
      Store<glNewListCommandSerializer>(compile));
}

void StateTrackingLayer::Post(glEndListCommand& command) {
  if (m_State.CompilingList(command.m_ThreadId)) {
    m_State.EndList(command.m_ThreadId,
                    StateRestorer(m_State, m_Recorder, m_Analysis).ResetListCompileState());
  }
}

void StateTrackingLayer::Post(glDeleteListsCommand& command) {
  m_State.DeleteLists(command.m_ThreadId, command.m_list.Value, command.m_range.Value,
                      Store<glDeleteListsCommandSerializer>(command));
}

void StateTrackingLayer::Post(wglUseFontBitmapsACommand& command) {
  if (command.m_Return.Value) {
    m_State.CreateFontLists(command.m_ThreadId, command.m_listBase.Value, command.m_count.Value,
                            Store<wglUseFontBitmapsACommandSerializer>(command));
  }
}

void StateTrackingLayer::Post(wglUseFontBitmapsWCommand& command) {
  if (command.m_Return.Value) {
    m_State.CreateFontLists(command.m_ThreadId, command.m_listBase.Value, command.m_count.Value,
                            Store<wglUseFontBitmapsWCommandSerializer>(command));
  }
}

// ---- Program pipelines --------------------------------------------------

void StateTrackingLayer::Post(glGenProgramPipelinesCommand& command) {
  CreateObjects(m_State, GLObjectKind::ProgramPipeline, command.m_ThreadId, command.m_pipelines);
}

void StateTrackingLayer::Post(glGenProgramPipelinesEXTCommand& command) {
  CreateObjects(m_State, GLObjectKind::ProgramPipeline, command.m_ThreadId, command.m_pipelines);
}

void StateTrackingLayer::Post(glCreateProgramPipelinesCommand& command) {
  CreateObjects(m_State, GLObjectKind::ProgramPipeline, command.m_ThreadId, command.m_pipelines);
}

void StateTrackingLayer::Post(glDeleteProgramPipelinesCommand& command) {
  DeleteObjects(m_State, GLObjectKind::ProgramPipeline, command.m_pipelines);
}

void StateTrackingLayer::Post(glDeleteProgramPipelinesEXTCommand& command) {
  DeleteObjects(m_State, GLObjectKind::ProgramPipeline, command.m_pipelines);
}

// ---- Vertex arrays ------------------------------------------------------

void StateTrackingLayer::Post(glGenVertexArraysCommand& command) {
  CreateObjects(m_State, GLObjectKind::VertexArray, command.m_ThreadId, command.m_arrays);
}

void StateTrackingLayer::Post(glGenVertexArraysAPPLECommand& command) {
  CreateObjects(m_State, GLObjectKind::VertexArray, command.m_ThreadId, command.m_arrays);
}

void StateTrackingLayer::Post(glGenVertexArraysOESCommand& command) {
  CreateObjects(m_State, GLObjectKind::VertexArray, command.m_ThreadId, command.m_arrays);
}

void StateTrackingLayer::Post(glCreateVertexArraysCommand& command) {
  CreateObjects(m_State, GLObjectKind::VertexArray, command.m_ThreadId, command.m_arrays);
}

void StateTrackingLayer::Post(glBindVertexArrayAPPLECommand& command) {
  m_State.CreateObject(GLObjectKind::VertexArray, command.m_ThreadId, command.m_array.Key);
}

void StateTrackingLayer::Post(glDeleteVertexArraysCommand& command) {
  DeleteObjects(m_State, GLObjectKind::VertexArray, command.m_arrays);
}

void StateTrackingLayer::Post(glDeleteVertexArraysAPPLECommand& command) {
  DeleteObjects(m_State, GLObjectKind::VertexArray, command.m_arrays);
}

void StateTrackingLayer::Post(glDeleteVertexArraysOESCommand& command) {
  DeleteObjects(m_State, GLObjectKind::VertexArray, command.m_arrays);
}

// ---- Framebuffers -------------------------------------------------------

void StateTrackingLayer::Post(glGenFramebuffersCommand& command) {
  CreateObjects(m_State, GLObjectKind::Framebuffer, command.m_ThreadId, command.m_framebuffers);
}

void StateTrackingLayer::Post(glGenFramebuffersEXTCommand& command) {
  CreateObjects(m_State, GLObjectKind::Framebuffer, command.m_ThreadId, command.m_framebuffers);
}

void StateTrackingLayer::Post(glGenFramebuffersOESCommand& command) {
  CreateObjects(m_State, GLObjectKind::Framebuffer, command.m_ThreadId, command.m_framebuffers);
}

void StateTrackingLayer::Post(glCreateFramebuffersCommand& command) {
  CreateObjects(m_State, GLObjectKind::Framebuffer, command.m_ThreadId, command.m_framebuffers);
}

void StateTrackingLayer::Post(glBindFramebufferEXTCommand& command) {
  m_State.CreateObject(GLObjectKind::Framebuffer, command.m_ThreadId, command.m_framebuffer.Key);
}

void StateTrackingLayer::Post(glDeleteFramebuffersCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Framebuffer, command.m_framebuffers);
}

void StateTrackingLayer::Post(glDeleteFramebuffersEXTCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Framebuffer, command.m_framebuffers);
}

void StateTrackingLayer::Post(glDeleteFramebuffersOESCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Framebuffer, command.m_framebuffers);
}

// ---- Queries ------------------------------------------------------------

void StateTrackingLayer::Post(glGenQueriesCommand& command) {
  CreateObjects(m_State, GLObjectKind::Query, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glGenQueriesARBCommand& command) {
  CreateObjects(m_State, GLObjectKind::Query, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glGenQueriesEXTCommand& command) {
  CreateObjects(m_State, GLObjectKind::Query, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glCreateQueriesCommand& command) {
  CreateObjects(m_State, GLObjectKind::Query, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glBeginQueryCommand& command) {
  m_State.CreateObject(GLObjectKind::Query, command.m_ThreadId, command.m_id.Key);
}

void StateTrackingLayer::Post(glBeginQueryARBCommand& command) {
  m_State.CreateObject(GLObjectKind::Query, command.m_ThreadId, command.m_id.Key);
}

void StateTrackingLayer::Post(glBeginQueryIndexedCommand& command) {
  m_State.CreateObject(GLObjectKind::Query, command.m_ThreadId, command.m_id.Key);
}

void StateTrackingLayer::Post(glQueryCounterCommand& command) {
  m_State.CreateObject(GLObjectKind::Query, command.m_ThreadId, command.m_id.Key);
}

void StateTrackingLayer::Post(glDeleteQueriesCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Query, command.m_ids);
}

void StateTrackingLayer::Post(glDeleteQueriesARBCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Query, command.m_ids);
}

void StateTrackingLayer::Post(glDeleteQueriesEXTCommand& command) {
  DeleteObjects(m_State, GLObjectKind::Query, command.m_ids);
}

// ---- Transform feedbacks ------------------------------------------------

void StateTrackingLayer::Post(glGenTransformFeedbacksCommand& command) {
  CreateObjects(m_State, GLObjectKind::TransformFeedback, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glGenTransformFeedbacksNVCommand& command) {
  CreateObjects(m_State, GLObjectKind::TransformFeedback, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glCreateTransformFeedbacksCommand& command) {
  CreateObjects(m_State, GLObjectKind::TransformFeedback, command.m_ThreadId, command.m_ids);
}

void StateTrackingLayer::Post(glDeleteTransformFeedbacksCommand& command) {
  DeleteObjects(m_State, GLObjectKind::TransformFeedback, command.m_ids);
}

void StateTrackingLayer::Post(glDeleteTransformFeedbacksNVCommand& command) {
  DeleteObjects(m_State, GLObjectKind::TransformFeedback, command.m_ids);
}

// ---- Syncs --------------------------------------------------------------

void StateTrackingLayer::Post(glFenceSyncCommand& command) {
  m_State.CreateSync(command.m_ThreadId, command.m_Return.Key);
}

void StateTrackingLayer::Post(glDeleteSyncCommand& command) {
  m_State.DeleteSync(command.m_sync.Key);
}

} // namespace gits::opengl2
