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
#include <cstdio>
#include <string>
#include <vector>

namespace gits::opengl {

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
std::vector<std::pair<GLenum, std::string>> AttachedShaders(GLuint program,
                                                            const ProgramState& tracked) {
  std::vector<GLuint> order;
  for (GITSKey shader : tracked.AttachOrder) {
    order.push_back(ObjectNameMapService::Get().TryGetHandle(GLObjectKind::Shader, shader));
  }
  std::vector<std::pair<GLenum, std::string>> linked;
  for (GLuint shader : AttachedShaderNames(program, order)) {
    GLint type = 0;
    GL_FN(glGetShaderiv)(shader, GL_SHADER_TYPE, &type);
    linked.emplace_back(static_cast<GLenum>(type), ShaderSource(shader));
  }
  return linked;
}

void AttachShader(StateTrackingService& state, GITSKey program, GITSKey shader) {
  ProgramState* tracked = state.Program(program);
  if (tracked != nullptr && std::find(tracked->AttachOrder.begin(), tracked->AttachOrder.end(),
                                      shader) == tracked->AttachOrder.end()) {
    tracked->AttachOrder.push_back(shader);
  }
}

void DetachShader(StateTrackingService& state, GITSKey program, GITSKey shader) {
  if (ProgramState* tracked = state.Program(program)) {
    std::erase(tracked->AttachOrder, shader);
  }
}

GLuint TextureName(GITSKey key) {
  return ObjectNameMapService::Get().TryGetHandle(GLObjectKind::Texture, key);
}

GLenum TextureTargetOf(GLenum imageTarget) {
  return imageTarget >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
                 imageTarget <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
             ? GL_TEXTURE_CUBE_MAP
             : imageTarget;
}

// The texture bound to the target of `imageTarget`; 0 for a proxy target, which has no image.
GLuint BoundTexture(GLenum imageTarget) {
  const TextureTarget* entry = FindTextureTarget(TextureTargetOf(imageTarget));
  return entry != nullptr ? GetName(entry->Binding) : 0;
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
  tracked.UniformBlockBindings.clear();
  tracked.StorageBlockBindings.clear();
  tracked.LinkedShaders.clear();
  tracked.LinkedSetupCount = tracked.LinkSetup.size();
}

void SnapshotLink(StateTrackingService& state, GITSKey key, GLuint program) {
  ProgramState* tracked = state.Program(key);
  if (tracked != nullptr && Linked(program)) {
    ReplaceExecutable(*tracked, key);
    tracked->LinkedShaders = AttachedShaders(program, *tracked);
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

void StateTrackingLayer::Post(glXChooseVisualCommand& command) {
  m_State.AddTopology(Unsampled<glXChooseVisualCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXGetVisualFromFBConfigCommand& command) {
  m_State.AddTopology(Unsampled<glXGetVisualFromFBConfigCommandSerializer>(command));
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

void StateTrackingLayer::Post(glXCreateContextCommand& command) {
  m_State.AddTopology(Unsampled<glXCreateContextCommandSerializer>(command));
  m_State.CreateContext(command.m_Return.Key, command.m_shareList.Key);
}

void StateTrackingLayer::Post(glXCreateNewContextCommand& command) {
  m_State.AddTopology(Unsampled<glXCreateNewContextCommandSerializer>(command));
  m_State.CreateContext(command.m_Return.Key, command.m_share_list.Key);
}

void StateTrackingLayer::Post(glXCreateContextAttribsARBCommand& command) {
  m_State.AddTopology(Unsampled<glXCreateContextAttribsARBCommandSerializer>(command));
  m_State.CreateContext(command.m_Return.Key, command.m_share_context.Key);
}

void StateTrackingLayer::Post(glXDestroyContextCommand& command) {
  m_State.DestroyGlxContext(command.m_ctx.Key,
                            Unsampled<glXDestroyContextCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXMakeCurrentCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrentGlx(command.m_ThreadId, command.m_ctx.Key, command.m_dpy.Value,
                        command.m_drawable.Value, command.m_drawable.Value,
                        Sampled<glXMakeCurrentCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(glXMakeContextCurrentCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrentGlx(command.m_ThreadId, command.m_ctx.Key, command.m_dpy.Value,
                        command.m_draw.Value, command.m_read.Value,
                        Sampled<glXMakeContextCurrentCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(glXMakeCurrentReadSGICommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  m_State.SetCurrentGlx(command.m_ThreadId, command.m_ctx.Key, command.m_dpy.Value,
                        command.m_draw.Value, command.m_read.Value,
                        Sampled<glXMakeCurrentReadSGICommandSerializer>(m_State, command));
}

// ---- GLX windows, pbuffers and pixmaps ----------------------------------

void StateTrackingLayer::Post(glXCreateWindowCommand& command) {
  m_State.AddTopology(Sampled<glXCreateWindowCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(glXDestroyWindowCommand& command) {
  m_State.DestroyGlxDrawable(command.m_win.Value,
                             Unsampled<glXDestroyWindowCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXCreatePbufferCommand& command) {
  m_State.AddTopology(Unsampled<glXCreatePbufferCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXDestroyPbufferCommand& command) {
  m_State.DestroyGlxDrawable(command.m_pbuf.Value,
                             Unsampled<glXDestroyPbufferCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXCreatePixmapCommand& command) {
  m_State.AddTopology(Sampled<glXCreatePixmapCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(glXDestroyPixmapCommand& command) {
  m_State.DestroyGlxDrawable(command.m_pixmap.Value,
                             Unsampled<glXDestroyPixmapCommandSerializer>(command));
}

void StateTrackingLayer::Post(glXCreateGLXPixmapCommand& command) {
  m_State.AddTopology(Sampled<glXCreateGLXPixmapCommandSerializer>(m_State, command));
}

void StateTrackingLayer::Post(glXDestroyGLXPixmapCommand& command) {
  m_State.DestroyGlxDrawable(command.m_pixmap.Value,
                             Unsampled<glXDestroyGLXPixmapCommandSerializer>(command));
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
  const auto [shareGroup, context] = m_State.CurrentShareGroup(threadId);
  for (GITSKey key : names.Key) {
    ObjectSnapshot snapshot;
    if (beforeRange) {
      snapshot = StateRestorer(m_State, m_Recorder, m_Analysis).CaptureDeleted(threadId, kind, key);
    }
    if (kind == GLObjectKind::Texture) {
      m_State.ForgetCompressedImages(shareGroup, TextureName(key));
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

void StateTrackingLayer::Post(glXSwapBuffersCommand& command) {
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

void StateTrackingLayer::KeepCompressedImage(uint32_t threadId,
                                             GLuint texture,
                                             GLenum imageTarget,
                                             GLint level,
                                             CompressedImage image,
                                             GLsizei imageSize,
                                             const ArrayArgument<uint8_t>& data) {
  if (!TextureCommandExecutes(threadId, true)) {
    return;
  }
  const bool bound = texture == 0;
  if (bound) {
    texture = BoundTexture(imageTarget);
  }
  if (texture == 0) {
    return;
  }
  const GITSKey shareGroup = m_State.CurrentShareGroup(threadId).first;
  if (bound ? GeneratesBoundLevels(threadId, imageTarget, level)
            : GeneratesTextureLevels(threadId, texture, imageTarget, level)) {
    m_State.ForgetCompressedImages(shareGroup, texture);
  }
  if (imageSize > 0 && data.Data.size() == static_cast<size_t>(imageSize)) {
    image.Data = data.Data;
  }
  m_State.KeepCompressedImage(shareGroup, texture, imageTarget, level, std::move(image));
}

template <typename CommandT>
void StateTrackingLayer::KeepBoundCompressedImage(const CommandT& command,
                                                  GLsizei height,
                                                  GLsizei depth) {
  KeepCompressedImage(command.m_ThreadId, 0, command.m_target.Value, command.m_level.Value,
                      {command.m_internalformat.Value, command.m_width.Value, height, depth},
                      command.m_imageSize.Value, command.m_data);
}

template <typename CommandT>
void StateTrackingLayer::KeepTextureCompressedImage(const CommandT& command,
                                                    GLsizei height,
                                                    GLsizei depth) {
  const GLuint texture =
      ObjectNameMapService::Get().TryGetHandle(GLObjectKind::Texture, command.m_texture.Key);
  if (texture == 0) {
    return;
  }
  KeepCompressedImage(command.m_ThreadId, texture, command.m_target.Value, command.m_level.Value,
                      {command.m_internalformat.Value, command.m_width.Value, height, depth},
                      command.m_imageSize.Value, command.m_bits);
}

bool StateTrackingLayer::Compatibility(uint32_t threadId) {
  const GITSKey context = m_State.CurrentShareGroup(threadId).second;
  const auto known = m_Compatibility.find(context);
  if (known != m_Compatibility.end()) {
    return known->second;
  }
  int major = 0;
  int minor = 0;
  if (const auto* version = reinterpret_cast<const char*>(GL_FN(glGetString)(GL_VERSION));
      version != nullptr) {
    std::sscanf(version, "%d.%d", &major, &minor);
  }
  const bool compatibility =
      major < 3 || (major == 3 && minor < 2) ||
      (GetInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
  m_Compatibility.emplace(context, compatibility);
  return compatibility;
}

bool StateTrackingLayer::GeneratesLevels(uint32_t threadId,
                                         GLint level,
                                         const std::function<GLint(GLenum)>& parameter) {
  return m_Range.CurrentFrame() < m_Range.StartFrame() && Compatibility(threadId) &&
         parameter(GL_GENERATE_MIPMAP) != GL_FALSE && parameter(GL_TEXTURE_BASE_LEVEL) == level;
}

bool StateTrackingLayer::GeneratesBoundLevels(uint32_t threadId, GLenum imageTarget, GLint level) {
  return BoundTexture(imageTarget) != 0 &&
         GeneratesLevels(threadId, level, [imageTarget](GLenum pname) {
           GLint value = 0;
           GL_FN(glGetTexParameteriv)(TextureTargetOf(imageTarget), pname, &value);
           return value;
         });
}

bool StateTrackingLayer::GeneratesTextureLevels(uint32_t threadId,
                                                GLuint texture,
                                                GLenum imageTarget,
                                                GLint level) {
  return texture != 0 && FindTextureTarget(TextureTargetOf(imageTarget)) != nullptr &&
         GeneratesLevels(threadId, level, [texture, imageTarget](GLenum pname) {
           GLint value = 0;
           GL_FN(glGetTextureParameterivEXT)(texture, TextureTargetOf(imageTarget), pname, &value);
           return value;
         });
}

bool StateTrackingLayer::GeneratesUnitLevels(uint32_t threadId,
                                             GLenum unit,
                                             GLenum imageTarget,
                                             GLint level) {
  return FindTextureTarget(TextureTargetOf(imageTarget)) != nullptr &&
         GeneratesLevels(threadId, level, [unit, imageTarget](GLenum pname) {
           GLint value = 0;
           GL_FN(glGetMultiTexParameterivEXT)(unit, TextureTargetOf(imageTarget), pname, &value);
           return value;
         });
}

void StateTrackingLayer::Post(glCompressedTexImage1DCommand& command) {
  KeepBoundCompressedImage(command, 1, 1);
}

void StateTrackingLayer::Post(glCompressedTexImage1DARBCommand& command) {
  KeepBoundCompressedImage(command, 1, 1);
}

void StateTrackingLayer::Post(glCompressedTexImage2DCommand& command) {
  KeepBoundCompressedImage(command, command.m_height.Value, 1);
}

void StateTrackingLayer::Post(glCompressedTexImage2DARBCommand& command) {
  KeepBoundCompressedImage(command, command.m_height.Value, 1);
}

void StateTrackingLayer::Post(glCompressedTexImage3DCommand& command) {
  KeepBoundCompressedImage(command, command.m_height.Value, command.m_depth.Value);
}

void StateTrackingLayer::Post(glCompressedTexImage3DARBCommand& command) {
  KeepBoundCompressedImage(command, command.m_height.Value, command.m_depth.Value);
}

void StateTrackingLayer::Post(glCompressedTextureImage1DEXTCommand& command) {
  KeepTextureCompressedImage(command, 1, 1);
}

void StateTrackingLayer::Post(glCompressedTextureImage2DEXTCommand& command) {
  KeepTextureCompressedImage(command, command.m_height.Value, 1);
}

void StateTrackingLayer::Post(glCompressedTextureImage3DEXTCommand& command) {
  KeepTextureCompressedImage(command, command.m_height.Value, command.m_depth.Value);
}

bool StateTrackingLayer::TextureCommandExecutes(uint32_t threadId, bool listable) {
  if (listable && m_State.CompilingList(threadId)) {
    m_State.MarkListChangesTextures(threadId);
  }
  return m_Range.CurrentFrame() < m_Range.StartFrame() &&
         !(listable && m_State.CompilingOnly(threadId));
}

void StateTrackingLayer::ForgetCompressedImages(uint32_t threadId,
                                                bool listable,
                                                std::optional<GLuint> texture,
                                                std::optional<GLenum> imageTarget,
                                                std::optional<GLint> level) {
  if (TextureCommandExecutes(threadId, listable) && (!texture || *texture != 0)) {
    m_State.ForgetCompressedImages(m_State.CurrentShareGroup(threadId).first, texture, imageTarget,
                                   level);
  }
}

template <typename CommandT>
void StateTrackingLayer::ForgetBoundCompressedImage(const CommandT& command) {
  const GLenum target = command.m_target.Value;
  const GLint level = command.m_level.Value;
  if (GeneratesBoundLevels(command.m_ThreadId, target, level)) {
    ForgetCompressedImages(command.m_ThreadId, true, BoundTexture(target));
  } else {
    ForgetCompressedImages(command.m_ThreadId, true, BoundTexture(target), target, level);
  }
}

template <typename CommandT>
void StateTrackingLayer::ForgetBoundCompressedImages(const CommandT& command) {
  ForgetCompressedImages(command.m_ThreadId, false, BoundTexture(command.m_target.Value));
}

template <typename CommandT>
void StateTrackingLayer::ForgetTextureCompressedImage(const CommandT& command) {
  const GLuint texture = TextureName(command.m_texture.Key);
  const GLenum target = command.m_target.Value;
  const GLint level = command.m_level.Value;
  if (GeneratesTextureLevels(command.m_ThreadId, texture, target, level)) {
    ForgetCompressedImages(command.m_ThreadId, true, texture);
  } else {
    ForgetCompressedImages(command.m_ThreadId, true, texture, target, level);
  }
}

template <typename CommandT>
void StateTrackingLayer::ForgetTextureCompressedLevel(const CommandT& command) {
  ForgetCompressedImages(command.m_ThreadId, false, TextureName(command.m_texture.Key),
                         std::nullopt, command.m_level.Value);
}

template <typename CommandT>
void StateTrackingLayer::ForgetTextureSubImageLevel(const CommandT& command) {
  const GLuint texture = TextureName(command.m_texture.Key);
  const GLint level = command.m_level.Value;
  const bool generates =
      texture != 0 && GeneratesLevels(command.m_ThreadId, level, [texture](GLenum pname) {
        GLint value = 0;
        GL_FN(glGetTextureParameteriv)(texture, pname, &value);
        return value;
      });
  if (generates) {
    ForgetCompressedImages(command.m_ThreadId, false, texture);
  } else {
    ForgetCompressedImages(command.m_ThreadId, false, texture, std::nullopt, level);
  }
}

template <typename CommandT>
void StateTrackingLayer::ForgetTextureCompressedImages(const CommandT& command) {
  ForgetCompressedImages(command.m_ThreadId, false, TextureName(command.m_texture.Key));
}

template <typename CommandT>
void StateTrackingLayer::ForgetCopyDestinationCompressedImages(const CommandT& command) {
  if (command.m_dstTarget.Value != GL_RENDERBUFFER) {
    ForgetCompressedImages(command.m_ThreadId, false, TextureName(command.m_dstName.Key),
                           std::nullopt, command.m_dstLevel.Value);
  }
}

template <typename CommandT>
void StateTrackingLayer::ForgetUnitCompressedImage(const CommandT& command) {
  const GLenum target = command.m_target.Value;
  const GLint level = command.m_level.Value;
  if (GeneratesUnitLevels(command.m_ThreadId, command.m_texunit.Value, target, level)) {
    ForgetCompressedImages(command.m_ThreadId, true, std::nullopt);
  } else {
    ForgetCompressedImages(command.m_ThreadId, true, std::nullopt, target, level);
  }
}

void StateTrackingLayer::Post(glCompressedTexSubImage1DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTexSubImage1DARBCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTexSubImage2DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTexSubImage2DARBCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTexSubImage3DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTexSubImage3DARBCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage1DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage1DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage2DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage2DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage3DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexSubImage3DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage1DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage1DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage2DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage2DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage3DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexSubImage3DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glGenerateMipmapCommand& command) {
  ForgetBoundCompressedImages(command);
}

void StateTrackingLayer::Post(glGenerateMipmapEXTCommand& command) {
  ForgetBoundCompressedImages(command);
}

void StateTrackingLayer::Post(glTexImage1DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexImage2DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexImage3DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glTexImage3DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexImage1DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexImage1DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexImage2DCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTexImage2DEXTCommand& command) {
  ForgetBoundCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage1DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage1DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage2DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage2DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage3DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCompressedTextureSubImage3DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glTextureSubImage1DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glTextureSubImage1DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glTextureSubImage2DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glTextureSubImage2DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glTextureSubImage3DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glTextureSubImage3DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage1DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage1DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage2DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage2DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage3DCommand& command) {
  ForgetTextureSubImageLevel(command);
}

void StateTrackingLayer::Post(glCopyTextureSubImage3DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glGenerateTextureMipmapCommand& command) {
  ForgetTextureCompressedImages(command);
}

void StateTrackingLayer::Post(glGenerateTextureMipmapEXTCommand& command) {
  ForgetTextureCompressedImages(command);
}

void StateTrackingLayer::Post(glClearTexImageCommand& command) {
  ForgetTextureCompressedLevel(command);
}

void StateTrackingLayer::Post(glClearTexImageEXTCommand& command) {
  ForgetTextureCompressedLevel(command);
}

void StateTrackingLayer::Post(glClearTexSubImageCommand& command) {
  ForgetTextureCompressedLevel(command);
}

void StateTrackingLayer::Post(glClearTexSubImageEXTCommand& command) {
  ForgetTextureCompressedLevel(command);
}

void StateTrackingLayer::Post(glTextureImage1DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glTextureImage2DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glTextureImage3DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTextureImage1DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyTextureImage2DEXTCommand& command) {
  ForgetTextureCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyImageSubDataCommand& command) {
  ForgetCopyDestinationCompressedImages(command);
}

void StateTrackingLayer::Post(glCopyImageSubDataEXTCommand& command) {
  ForgetCopyDestinationCompressedImages(command);
}

void StateTrackingLayer::Post(glCopyImageSubDataNVCommand& command) {
  ForgetCopyDestinationCompressedImages(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexImage3DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexSubImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexSubImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCompressedMultiTexSubImage3DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexImage3DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexSubImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexSubImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glMultiTexSubImage3DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyMultiTexImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyMultiTexImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyMultiTexSubImage1DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyMultiTexSubImage2DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glCopyMultiTexSubImage3DEXTCommand& command) {
  ForgetUnitCompressedImage(command);
}

void StateTrackingLayer::Post(glGenerateMultiTexMipmapEXTCommand& command) {
  ForgetCompressedImages(command.m_ThreadId, false, std::nullopt);
}

void StateTrackingLayer::Post(MappedTextureDataMetaCommand& command) {
  const size_t count = std::min(command.m_Textures.Value.size(), command.m_Levels.Data.size());
  for (size_t i = 0; i < count; ++i) {
    ForgetCompressedImages(command.m_ThreadId, false, command.m_Textures.Value[i], std::nullopt,
                           command.m_Levels.Data[i]);
  }
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

void StateTrackingLayer::Post(glAttachShaderCommand& command) {
  AttachShader(m_State, command.m_program.Key, command.m_shader.Key);
}

void StateTrackingLayer::Post(glAttachObjectARBCommand& command) {
  AttachShader(m_State, command.m_containerObj.Key, command.m_obj.Key);
}

void StateTrackingLayer::Post(glDetachShaderCommand& command) {
  DetachShader(m_State, command.m_program.Key, command.m_shader.Key);
}

void StateTrackingLayer::Post(glDetachObjectARBCommand& command) {
  DetachShader(m_State, command.m_containerObj.Key, command.m_attachedObj.Key);
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

void StateTrackingLayer::Post(glUniformBlockBindingCommand& command) {
  if (ProgramState* tracked = m_State.Program(command.m_program.Key)) {
    tracked->UniformBlockBindings[command.m_uniformBlockIndex.Value] =
        Store<glUniformBlockBindingCommandSerializer>(command);
  }
}

void StateTrackingLayer::Post(glShaderStorageBlockBindingCommand& command) {
  if (ProgramState* tracked = m_State.Program(command.m_program.Key)) {
    tracked->StorageBlockBindings[command.m_storageBlockIndex.Value] =
        Store<glShaderStorageBlockBindingCommandSerializer>(command);
  }
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
      command.m_ThreadId, command.m_list.Value, command.m_mode.Value == GL_COMPILE_AND_EXECUTE,
      StateRestorer(m_State, m_Recorder, m_Analysis).CaptureListCompileState(command.m_ThreadId),
      Store<glNewListCommandSerializer>(compile));
}

void StateTrackingLayer::Post(glEndListCommand& command) {
  if (m_State.CompilingList(command.m_ThreadId)) {
    m_State.EndList(command.m_ThreadId,
                    StateRestorer(m_State, m_Recorder, m_Analysis).ResetListCompileState());
  }
}

// The texture commands of a list change images only when it executes, on the textures bound then,
// so executing a list that can change them forgets the kept compressed images of the share group.
void StateTrackingLayer::Post(glCallListCommand& command) {
  const uint32_t threadId = command.m_ThreadId;
  if (m_State.CompilingList(threadId) ||
      m_State.ListChangesTextures(m_State.CurrentShareGroup(threadId).first,
                                  command.m_list.Value)) {
    ForgetCompressedImages(threadId, true, std::nullopt);
  }
}

void StateTrackingLayer::Post(glCallListsCommand& command) {
  const uint32_t threadId = command.m_ThreadId;
  if (m_State.CompilingList(threadId) ||
      m_State.ListChangesTextures(m_State.CurrentShareGroup(threadId).first, std::nullopt)) {
    ForgetCompressedImages(threadId, true, std::nullopt);
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

} // namespace gits::opengl
