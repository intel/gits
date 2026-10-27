// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateRestorer.h"

#include "commandSerializersAuto.h"
#include "glxDrawableService.h"
#include "handleMapService.h"
#include "hdcVirtualizationService.h"
#include "liveGl.h"
#include "log.h"
#include "systemApi.h"
#include "threadContextService.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <map>
#include <sstream>
#include <tuple>

namespace gits::opengl {

namespace {

struct Capability {
  GLenum Cap;
  int Major;
  int Minor;
};

constexpr Capability CAPABILITIES[] = {
    {GL_BLEND, 1, 0},
    {GL_CULL_FACE, 1, 0},
    {GL_DEPTH_TEST, 1, 0},
    {GL_STENCIL_TEST, 1, 0},
    {GL_SCISSOR_TEST, 1, 0},
    {GL_DITHER, 1, 0},
    {GL_LINE_SMOOTH, 1, 0},
    {GL_POLYGON_SMOOTH, 1, 0},
    {GL_COLOR_LOGIC_OP, 1, 1},
    {GL_POLYGON_OFFSET_FILL, 1, 1},
    {GL_POLYGON_OFFSET_LINE, 1, 1},
    {GL_POLYGON_OFFSET_POINT, 1, 1},
    {GL_MULTISAMPLE, 1, 3},
    {GL_SAMPLE_ALPHA_TO_COVERAGE, 1, 3},
    {GL_SAMPLE_ALPHA_TO_ONE, 1, 3},
    {GL_SAMPLE_COVERAGE, 1, 3},
    {GL_FRAMEBUFFER_SRGB, 3, 0},
    {GL_RASTERIZER_DISCARD, 3, 0},
    {GL_PRIMITIVE_RESTART, 3, 1},
    {GL_DEPTH_CLAMP, 3, 2},
    {GL_PROGRAM_POINT_SIZE, 3, 2},
    {GL_TEXTURE_CUBE_MAP_SEAMLESS, 3, 2},
    {GL_SAMPLE_MASK, 3, 2},
    {GL_SAMPLE_SHADING, 4, 0},
    {GL_PRIMITIVE_RESTART_FIXED_INDEX, 4, 3},
};

constexpr GLenum UNPACK_STATE[] = {
    GL_UNPACK_SWAP_BYTES, GL_UNPACK_LSB_FIRST,   GL_UNPACK_ROW_LENGTH,  GL_UNPACK_IMAGE_HEIGHT,
    GL_UNPACK_SKIP_ROWS,  GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_IMAGES, GL_UNPACK_ALIGNMENT,
};

constexpr GLenum PACK_STATE[] = {
    GL_PACK_SWAP_BYTES, GL_PACK_LSB_FIRST,   GL_PACK_ROW_LENGTH,  GL_PACK_IMAGE_HEIGHT,
    GL_PACK_SKIP_ROWS,  GL_PACK_SKIP_PIXELS, GL_PACK_SKIP_IMAGES, GL_PACK_ALIGNMENT,
};

GLint DefaultPixelStore(GLenum pname) {
  return pname == GL_UNPACK_ALIGNMENT || pname == GL_PACK_ALIGNMENT ? 4 : 0;
}

struct QueryTarget {
  GLenum Target;
  // One query per vertex stream from 4.0.
  bool Indexed;
  int Major;
  int Minor;
};

constexpr QueryTarget QUERY_TARGETS[] = {
    {GL_SAMPLES_PASSED, false, 1, 5},
    {GL_ANY_SAMPLES_PASSED, false, 3, 3},
    {GL_ANY_SAMPLES_PASSED_CONSERVATIVE, false, 4, 3},
    {GL_TIME_ELAPSED, false, 3, 3},
    {GL_PRIMITIVES_GENERATED, true, 3, 0},
    {GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, true, 3, 0},
    {GL_TRANSFORM_FEEDBACK_OVERFLOW, false, 4, 6},
    {GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, true, 4, 6},
    {GL_VERTICES_SUBMITTED, false, 4, 6},
    {GL_PRIMITIVES_SUBMITTED, false, 4, 6},
    {GL_VERTEX_SHADER_INVOCATIONS, false, 4, 6},
    {GL_TESS_CONTROL_SHADER_PATCHES, false, 4, 6},
    {GL_TESS_EVALUATION_SHADER_INVOCATIONS, false, 4, 6},
    {GL_GEOMETRY_SHADER_INVOCATIONS, false, 4, 6},
    {GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED, false, 4, 6},
    {GL_FRAGMENT_SHADER_INVOCATIONS, false, 4, 6},
    {GL_COMPUTE_SHADER_INVOCATIONS, false, 4, 6},
    {GL_CLIPPING_INPUT_PRIMITIVES, false, 4, 6},
    {GL_CLIPPING_OUTPUT_PRIMITIVES, false, 4, 6},
};

struct BufferTarget {
  GLenum Target;
  GLenum Binding;
  int Major;
  int Minor;
};

constexpr BufferTarget BUFFER_TARGETS[] = {
    {GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, 1, 5},
    {GL_PIXEL_PACK_BUFFER, GL_PIXEL_PACK_BUFFER_BINDING, 2, 1},
    {GL_PIXEL_UNPACK_BUFFER, GL_PIXEL_UNPACK_BUFFER_BINDING, 2, 1},
    {GL_TRANSFORM_FEEDBACK_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 3, 0},
    {GL_COPY_READ_BUFFER, GL_COPY_READ_BUFFER_BINDING, 3, 1},
    {GL_COPY_WRITE_BUFFER, GL_COPY_WRITE_BUFFER_BINDING, 3, 1},
    {GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, 3, 1},
    {GL_DRAW_INDIRECT_BUFFER, GL_DRAW_INDIRECT_BUFFER_BINDING, 4, 0},
    {GL_ATOMIC_COUNTER_BUFFER, GL_ATOMIC_COUNTER_BUFFER_BINDING, 4, 2},
    {GL_DISPATCH_INDIRECT_BUFFER, GL_DISPATCH_INDIRECT_BUFFER_BINDING, 4, 3},
    {GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER_BINDING, 4, 3},
    {GL_QUERY_BUFFER, GL_QUERY_BUFFER_BINDING, 4, 4},
};

struct IndexedBufferTarget {
  GLenum Target;
  GLenum Binding;
  GLenum Start;
  GLenum Size;
  GLenum MaxBindings;
  int Major;
  int Minor;
};

constexpr IndexedBufferTarget INDEXED_BUFFER_TARGETS[] = {
    {GL_TRANSFORM_FEEDBACK_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING,
     GL_TRANSFORM_FEEDBACK_BUFFER_START, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE,
     GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, 3, 0},
    {GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, GL_UNIFORM_BUFFER_START, GL_UNIFORM_BUFFER_SIZE,
     GL_MAX_UNIFORM_BUFFER_BINDINGS, 3, 1},
    {GL_ATOMIC_COUNTER_BUFFER, GL_ATOMIC_COUNTER_BUFFER_BINDING, GL_ATOMIC_COUNTER_BUFFER_START,
     GL_ATOMIC_COUNTER_BUFFER_SIZE, GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, 4, 2},
    {GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER_BINDING, GL_SHADER_STORAGE_BUFFER_START,
     GL_SHADER_STORAGE_BUFFER_SIZE, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, 4, 3},
};

constexpr GLenum HINTS[] = {
    GL_LINE_SMOOTH_HINT,
    GL_POLYGON_SMOOTH_HINT,
    GL_TEXTURE_COMPRESSION_HINT,
    GL_FRAGMENT_SHADER_DERIVATIVE_HINT,
};

// Another transform feedback cannot be bound while the bound one is active and not paused.
bool TransformFeedbackBindable() {
  return !GetInteger(GL_TRANSFORM_FEEDBACK_ACTIVE) || GetInteger(GL_TRANSFORM_FEEDBACK_PAUSED);
}

int DrainErrors() {
  int count = 0;
  while (GL_FN(glGetError)() != GL_NO_ERROR && count < 64) {
    ++count;
  }
  return count;
}

} // namespace

void StateRestorer::Restore() {
  LOG_INFO << "Subcapture: restoring state";
  const auto live = ThreadContextService::Capture(ThreadContextService::Get().CurrentEntryPoint());

  for (const SampledCommand& command : m_State.Topology()) {
    Emit(command);
  }

  std::map<GITSKey, std::shared_ptr<MakeCurrent>> shareGroups;
  for (const auto& [key, context] : m_State.Contexts()) {
    if (context.LastMakeCurrent && !shareGroups.contains(context.ShareGroup)) {
      shareGroups[context.ShareGroup] = context.LastMakeCurrent;
    }
  }
  CollectRequired(shareGroups);
  bool blocksRestored = false;
  for (const auto& [shareGroup, current] : shareGroups) {
    if (!SwitchTo(*current)) {
      continue;
    }
    if (!std::exchange(blocksRestored, true)) {
      RestoreClientArrayBlocks();
    }
    MapNames(shareGroup, current->Context);
    BeginReadback();
    RestoreSharedObjects(shareGroup);
    EndReadback();
    RestoreDisplayLists(shareGroup);
    if (const int errors = DrainErrors()) {
      LOG_WARNING << "Subcapture: restoring shared objects of context " << current->Context
                  << " raised " << errors << " GL errors";
    }
  }

  for (const auto& [key, context] : m_State.Contexts()) {
    if (!context.LastMakeCurrent || !SwitchTo(*context.LastMakeCurrent)) {
      continue;
    }
    MapNames(context.ShareGroup, key);
    BeginReadback();
    RestoreContainers(key);
    if (m_Version.AtLeast(3, 0) && m_RestoredDrawables.insert(LiveDrawable()).second) {
      RestoreDefaultFramebuffer();
    }
    RestoreReadDrawable(*context.LastMakeCurrent);
    EndReadback();
    RestoreContextState();
    RestoreBindings();
    RestoreActiveQueries();
    if (const auto resident = m_State.ResidentHandles().find(key);
        resident != m_State.ResidentHandles().end()) {
      for (const auto& [handle, command] : resident->second) {
        command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
      }
    }
    if (const int errors = DrainErrors()) {
      LOG_WARNING << "Subcapture: restoring state of context " << key << " raised " << errors
                  << " GL errors";
    }
  }

  DeletePendingObjects(shareGroups);

  for (const auto& [threadId, display] : m_UsedThreads) {
    m_ThreadId = threadId;
    if (display != nullptr) {
      glXMakeCurrentCommand release;
      release.m_dpy.Value = display;
      release.m_Return.Value = 1;
      Emit<glXMakeCurrentCommandSerializer>(release);
    } else {
      wglMakeCurrentCommand release;
      Emit<wglMakeCurrentCommandSerializer>(release);
    }
  }
  for (const auto& [threadId, current] : m_State.Threads()) {
    if (current->Context != 0 && m_State.Contexts().contains(current->Context)) {
      Emit(current->Call);
    }
  }
  // After the make-current of the thread that still has the context or drawable current.
  for (const auto& [context, destroy] : m_State.PendingGlxDestroys()) {
    Emit(destroy);
  }
  for (const auto& [drawable, destroy] : m_State.PendingGlxDrawableDestroys()) {
    Emit(destroy);
  }

  ThreadContextService::MakeCurrent(live);
}

uint64_t StateRestorer::LiveDrawable() const {
  return m_RecordedDisplay != nullptr ? m_LiveDrawable : reinterpret_cast<uintptr_t>(m_LiveDc);
}

// A make-context-current call can read another window or pbuffer than the one it draws to. That
// drawable is restored as the draw one of a wglMakeCurrent, then the recorded call is made again.
void StateRestorer::RestoreReadDrawable(const MakeCurrent& current) {
  if (!m_Version.AtLeast(3, 0)) {
    return;
  }
  if (current.GlxDisplay != nullptr) {
    RestoreGlxReadDrawable(current);
    return;
  }
  if (current.ReadDc == nullptr || current.ReadDc == current.Dc) {
    return;
  }
  const HDC readDc = HdcVirtualizationService::Get().Resolve(current.ReadDc);
  const HGLRC context = ContextHandleMapService::Get().GetHandle(current.Context);
  if (readDc == nullptr || m_RestoredDrawables.contains(reinterpret_cast<uintptr_t>(readDc)) ||
      !GL_FN(wglMakeCurrent)(readDc, context)) {
    return;
  }
  m_RestoredDrawables.insert(reinterpret_cast<uintptr_t>(readDc));
  wglMakeCurrentCommand makeCurrent;
  makeCurrent.m_hDc.Value = current.ReadDc;
  makeCurrent.m_newContext.Key = current.Context;
  Emit<wglMakeCurrentCommandSerializer>(makeCurrent);

  const HDC drawDc = std::exchange(m_LiveDc, readDc);
  const HDC recordedDrawDc = std::exchange(m_RecordedDc, current.ReadDc);
  RestoreDefaultFramebuffer();
  m_LiveDc = drawDc;
  m_RecordedDc = recordedDrawDc;

  Emit(current.Call);
  GL_FN(wglMakeCurrent)(drawDc, context);
}

// The GLX counterpart of RestoreReadDrawable, through glXMakeContextCurrent with the read drawable
// as both drawables.
void StateRestorer::RestoreGlxReadDrawable(const MakeCurrent& current) {
  if (current.ReadDrawable == 0 || current.ReadDrawable == current.Drawable) {
    return;
  }
  Display* display = GlxDrawableService::Get().GetDisplay();
  const GLXDrawable read = GlxDrawableService::Get().Resolve(current.ReadDrawable);
  const GLXContext context = GlxContextHandleMapService::Get().GetHandle(current.Context);
  if (read == 0 || m_RestoredDrawables.contains(read) ||
      !GL_FN(glXMakeContextCurrent)(display, read, read, context)) {
    return;
  }
  m_RestoredDrawables.insert(read);
  glXMakeContextCurrentCommand makeCurrent;
  makeCurrent.m_dpy.Value = current.GlxDisplay;
  makeCurrent.m_draw.Value = current.ReadDrawable;
  makeCurrent.m_read.Value = current.ReadDrawable;
  makeCurrent.m_ctx.Key = current.Context;
  makeCurrent.m_Return.Value = 1;
  Emit<glXMakeContextCurrentCommandSerializer>(makeCurrent);

  const GLXDrawable drawDrawable = std::exchange(m_LiveDrawable, read);
  const GLXDrawable recordedDrawDrawable = std::exchange(m_RecordedDrawable, current.ReadDrawable);
  RestoreDefaultFramebuffer();
  m_LiveDrawable = drawDrawable;
  m_RecordedDrawable = recordedDrawDrawable;

  Emit(current.Call);
  GL_FN(glXMakeContextCurrent)(display, drawDrawable, drawDrawable, context);
}

// Programs deleted while in use are deleted again once the restored contexts use them. Deleted
// objects restored for the containers that reference them, the resident handles that keep them,
// or the views that share their storage, are deleted again once the containers, residency and
// views are restored, in the context that deleted them. No container is bound to it meanwhile, as one would lose its references to them.
void StateRestorer::DeletePendingObjects(
    const std::map<GITSKey, std::shared_ptr<MakeCurrent>>& shareGroups) {
  std::map<GITSKey, std::vector<std::pair<GITSKey, GLObjectKind>>> pending;
  for (const auto& [key, object] : m_State.Objects()) {
    const bool restored = object.Snapshot.Kept() && Required(key);
    const auto group = shareGroups.find(object.Owner);
    if (!object.DeletePending || (object.Kind != GLObjectKind::Program && !restored) ||
        group == shareGroups.end()) {
      continue;
    }
    GITSKey context = group->second->Context;
    const auto deletedIn = m_State.Contexts().find(object.DeletedIn);
    if (restored && deletedIn != m_State.Contexts().end() && deletedIn->second.LastMakeCurrent &&
        deletedIn->second.ShareGroup == object.Owner) {
      context = object.DeletedIn;
    }
    pending[context].emplace_back(key, object.Kind);
  }

  for (const auto& [context, objects] : pending) {
    const ContextState& state = m_State.Contexts().at(context);
    if (!SwitchTo(*state.LastMakeCurrent)) {
      continue;
    }
    MapNames(state.ShareGroup, context);
    const bool containers = m_Version.AtLeast(3, 0) &&
                            std::any_of(objects.begin(), objects.end(), [](const auto& entry) {
                              return entry.second != GLObjectKind::Program;
                            });
    const GLuint drawFramebuffer = containers ? GetName(GL_DRAW_FRAMEBUFFER_BINDING) : 0;
    const GLuint readFramebuffer = containers ? GetName(GL_READ_FRAMEBUFFER_BINDING) : 0;
    const GLuint vertexArray = containers ? GetName(GL_VERTEX_ARRAY_BINDING) : 0;
    const bool transformFeedbacks =
        containers && (m_Version.AtLeast(4, 0) || HasExtension("GL_ARB_transform_feedback2")) &&
        TransformFeedbackBindable();
    const GLuint transformFeedback =
        transformFeedbacks ? GetName(GL_TRANSFORM_FEEDBACK_BINDING) : 0;
    if (drawFramebuffer != 0 || readFramebuffer != 0) {
      glBindFramebufferCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      Emit<glBindFramebufferCommandSerializer>(command);
    }
    if (vertexArray != 0) {
      glBindVertexArrayCommand command;
      Emit<glBindVertexArrayCommandSerializer>(command);
    }
    if (transformFeedback != 0) {
      glBindTransformFeedbackCommand command;
      command.m_target.Value = GL_TRANSFORM_FEEDBACK;
      Emit<glBindTransformFeedbackCommandSerializer>(command);
    }

    for (const auto& [key, kind] : objects) {
      if (kind == GLObjectKind::Program) {
        const GLuint name = NameOf(GLObjectKind::Program, key);
        if (name != 0 && GL_FN(glIsProgram)(name)) {
          glDeleteProgramCommand command;
          command.m_program.Key = key;
          Emit<glDeleteProgramCommandSerializer>(command);
        }
      } else if (kind == GLObjectKind::Buffer) {
        glDeleteBuffersCommand command;
        command.m_n.Value = 1;
        command.m_buffers.Key = {key};
        command.m_buffers.Value = {0};
        command.m_buffers.HasValue = true;
        Emit<glDeleteBuffersCommandSerializer>(command);
      } else if (kind == GLObjectKind::Texture) {
        glDeleteTexturesCommand command;
        command.m_n.Value = 1;
        command.m_textures.Key = {key};
        command.m_textures.Value = {0};
        command.m_textures.HasValue = true;
        Emit<glDeleteTexturesCommandSerializer>(command);
      } else if (kind == GLObjectKind::Sampler) {
        glDeleteSamplersCommand command;
        command.m_count.Value = 1;
        command.m_samplers.Key = {key};
        command.m_samplers.Value = {0};
        command.m_samplers.HasValue = true;
        Emit<glDeleteSamplersCommandSerializer>(command);
      } else {
        glDeleteRenderbuffersCommand command;
        command.m_n.Value = 1;
        command.m_renderbuffers.Key = {key};
        command.m_renderbuffers.Value = {0};
        command.m_renderbuffers.HasValue = true;
        Emit<glDeleteRenderbuffersCommandSerializer>(command);
      }
    }

    if (drawFramebuffer != 0 || readFramebuffer != 0) {
      glBindFramebufferCommand bindDraw;
      bindDraw.m_target.Value = GL_DRAW_FRAMEBUFFER;
      bindDraw.m_framebuffer.Key = KeyOf(GLObjectKind::Framebuffer, drawFramebuffer);
      Emit<glBindFramebufferCommandSerializer>(bindDraw);
      glBindFramebufferCommand bindRead;
      bindRead.m_target.Value = GL_READ_FRAMEBUFFER;
      bindRead.m_framebuffer.Key = KeyOf(GLObjectKind::Framebuffer, readFramebuffer);
      Emit<glBindFramebufferCommandSerializer>(bindRead);
    }
    if (vertexArray != 0) {
      glBindVertexArrayCommand command;
      command.m_array.Key = KeyOf(GLObjectKind::VertexArray, vertexArray);
      Emit<glBindVertexArrayCommandSerializer>(command);
    }
    if (transformFeedback != 0) {
      glBindTransformFeedbackCommand command;
      command.m_target.Value = GL_TRANSFORM_FEEDBACK;
      command.m_id.Key = KeyOf(GLObjectKind::TransformFeedback, transformFeedback);
      Emit<glBindTransformFeedbackCommandSerializer>(command);
    }
  }
}

bool StateRestorer::Attached(
    GITSKey shareGroup, GITSKey context, GLObjectKind kind, GLuint name, bool unboundOnly) {
  const auto names = [this](GLObjectKind objectKind, GITSKey owner, GLenum target = 0) {
    std::vector<GLuint> names;
    for (const auto& [key, object] : m_State.Objects()) {
      if (object.Kind != objectKind || object.Owner != owner || object.DeletePending ||
          (target != 0 && object.Target != target)) {
        continue;
      }
      if (const GLuint objectName = NameOf(objectKind, key)) {
        names.push_back(objectName);
      }
    }
    return names;
  };

  bool attached = false;
  if (kind != GLObjectKind::Buffer) {
    const std::vector<GLuint> framebuffers = names(GLObjectKind::Framebuffer, context);
    if (framebuffers.empty()) {
      return false;
    }
    const GLuint drawBound = GetName(GL_DRAW_FRAMEBUFFER_BINDING);
    const GLuint readBound = GetName(GL_READ_FRAMEBUFFER_BINDING);
    const GLint type = kind == GLObjectKind::Texture ? GL_TEXTURE : GL_RENDERBUFFER;
    std::vector<GLenum> attachments{GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    const GLint colorAttachments = GetInteger(GL_MAX_COLOR_ATTACHMENTS);
    for (GLint i = 0; i < colorAttachments; ++i) {
      attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
    }
    for (size_t i = 0; i < framebuffers.size() && !attached; ++i) {
      const GLuint framebuffer = framebuffers[i];
      if ((unboundOnly && (framebuffer == drawBound || framebuffer == readBound)) ||
          !GL_FN(glIsFramebuffer)(framebuffer)) {
        continue;
      }
      GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, framebuffer);
      for (GLenum attachment : attachments) {
        GLint objectType = GL_NONE;
        GL_FN(glGetFramebufferAttachmentParameteriv)
        (GL_READ_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        GLint object = 0;
        if (objectType == type) {
          GL_FN(glGetFramebufferAttachmentParameteriv)
          (GL_READ_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &object);
        }
        if (objectType == type && static_cast<GLuint>(object) == name) {
          attached = true;
          break;
        }
      }
    }
    GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, readBound);
    return attached;
  }

  const std::vector<GLuint> vertexArrays = names(GLObjectKind::VertexArray, context);
  if (!vertexArrays.empty()) {
    const GLuint bound = GetName(GL_VERTEX_ARRAY_BINDING);
    const GLint attributes = GetInteger(GL_MAX_VERTEX_ATTRIBS);
    const GLint bindings = m_Version.AtLeast(4, 3) ? GetInteger(GL_MAX_VERTEX_ATTRIB_BINDINGS) : 0;
    for (size_t i = 0; i < vertexArrays.size() && !attached; ++i) {
      const GLuint vertexArray = vertexArrays[i];
      if ((unboundOnly && vertexArray == bound) || !GL_FN(glIsVertexArray)(vertexArray)) {
        continue;
      }
      GL_FN(glBindVertexArray)(vertexArray);
      attached = GetName(GL_ELEMENT_ARRAY_BUFFER_BINDING) == name;
      for (GLint index = 0; index < attributes && !attached; ++index) {
        attached = static_cast<GLuint>(GetVertexAttribute(
                       static_cast<GLuint>(index), GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING)) == name;
      }
      for (GLint index = 0; index < bindings && !attached; ++index) {
        attached = static_cast<GLuint>(GetIntegerIndexed(GL_VERTEX_BINDING_BUFFER,
                                                         static_cast<GLuint>(index))) == name;
      }
    }
    GL_FN(glBindVertexArray)(bound);
  }

  const std::vector<GLuint> transformFeedbacks = names(GLObjectKind::TransformFeedback, context);
  if (!attached && !transformFeedbacks.empty() && TransformFeedbackBindable()) {
    const GLuint bound = GetName(GL_TRANSFORM_FEEDBACK_BINDING);
    const GLint buffers = GetInteger(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS);
    for (size_t i = 0; i < transformFeedbacks.size() && !attached; ++i) {
      const GLuint transformFeedback = transformFeedbacks[i];
      if ((unboundOnly && transformFeedback == bound) ||
          !GL_FN(glIsTransformFeedback)(transformFeedback)) {
        continue;
      }
      GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, transformFeedback);
      for (GLint index = 0; index < buffers && !attached; ++index) {
        attached = static_cast<GLuint>(GetIntegerIndexed(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING,
                                                         static_cast<GLuint>(index))) == name;
      }
    }
    GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, bound);
  }

  const std::vector<GLuint> bufferTextures =
      names(GLObjectKind::Texture, shareGroup, GL_TEXTURE_BUFFER);
  if (!attached && !bufferTextures.empty()) {
    const GLuint bound = GetName(GL_TEXTURE_BINDING_BUFFER);
    for (size_t i = 0; i < bufferTextures.size() && !attached; ++i) {
      if (!GL_FN(glIsTexture)(bufferTextures[i])) {
        continue;
      }
      GL_FN(glBindTexture)(GL_TEXTURE_BUFFER, bufferTextures[i]);
      GLint buffer = 0;
      GL_FN(glGetTexLevelParameteriv)
      (GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, &buffer);
      attached = static_cast<GLuint>(buffer) == name;
    }
    GL_FN(glBindTexture)(GL_TEXTURE_BUFFER, bound);
  }
  return attached;
}

void StateRestorer::Emit(const SampledCommand& command) {
  if (m_Collecting) {
    return;
  }
  if (command.Geometry) {
    command.Geometry->Record(m_Recorder, m_Recorder.CreateStateRestoreKey());
  }
  command.Command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey());
}

ListCompileState StateRestorer::CaptureListCompileState(uint32_t threadId) {
  ListCompileState state;
  ReadVersion();
  if (!m_Version.Compatibility) {
    return state;
  }
  m_UnmappedNames = m_State.CurrentShareGroup(threadId);
  m_Capture = &state.Commands;
  if (m_Version.AtLeast(2, 0)) {
    RestoreVertexAttributes();
  } else {
    RestoreClientArrays();
  }
  for (GLenum pname : UNPACK_STATE) {
    const GLint value = GetInteger(pname);
    if (value != DefaultPixelStore(pname)) {
      glPixelStoreiCommand command;
      command.m_pname.Value = pname;
      command.m_param.Value = value;
      Emit<glPixelStoreiCommandSerializer>(command);
    }
  }
  if (m_Version.AtLeast(2, 1)) {
    if (const GLuint buffer = GetName(GL_PIXEL_UNPACK_BUFFER_BINDING)) {
      glBindBufferCommand command;
      command.m_target.Value = GL_PIXEL_UNPACK_BUFFER;
      command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, buffer);
      Emit<glBindBufferCommandSerializer>(command);
    }
  }
  m_Capture = nullptr;
  state.Buffers.assign(m_Required.begin(), m_Required.end());
  return state;
}

ListCompileState StateRestorer::ResetListCompileState() {
  ListCompileState state;
  ReadVersion();
  if (!m_Version.Compatibility) {
    return state;
  }
  m_Capture = &state.Commands;
  ResetClientArrays();
  if (m_Version.AtLeast(2, 0)) {
    const GLint attributes =
        std::min(GetInteger(GL_MAX_VERTEX_ATTRIBS), MAX_RESTORED_VERTEX_ATTRIBUTES);
    for (GLint i = 0; i < attributes; ++i) {
      const auto index = static_cast<GLuint>(i);
      if (GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED)) {
        glDisableVertexAttribArrayCommand command;
        command.m_index.Value = index;
        Emit<glDisableVertexAttribArrayCommandSerializer>(command);
      }
    }
  }
  for (const auto& [target, binding, major, minor] :
       {std::tuple{GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, 1, 5},
        std::tuple{GL_PIXEL_UNPACK_BUFFER, GL_PIXEL_UNPACK_BUFFER_BINDING, 2, 1}}) {
    if (m_Version.AtLeast(major, minor) && GetName(binding) != 0) {
      glBindBufferCommand command;
      command.m_target.Value = target;
      Emit<glBindBufferCommandSerializer>(command);
    }
  }
  for (GLenum pname : UNPACK_STATE) {
    if (GetInteger(pname) != DefaultPixelStore(pname)) {
      glPixelStoreiCommand command;
      command.m_pname.Value = pname;
      command.m_param.Value = DefaultPixelStore(pname);
      Emit<glPixelStoreiCommandSerializer>(command);
    }
  }
  m_Capture = nullptr;
  return state;
}

bool StateRestorer::SwitchTo(const MakeCurrent& current) {
  bool switched = false;
  if (current.GlxDisplay != nullptr) {
    const GLXContext context = GlxContextHandleMapService::Get().GetHandle(current.Context);
    const GLXDrawable drawable = GlxDrawableService::Get().Resolve(current.Drawable);
    switched =
        context != nullptr && GL_FN(glXMakeContextCurrent)(GlxDrawableService::Get().GetDisplay(),
                                                           drawable, drawable, context) != 0;
    if (switched) {
      m_LiveDrawable = drawable;
      m_RecordedDrawable = current.Drawable;
    }
  } else {
    const HGLRC context = ContextHandleMapService::Get().GetHandle(current.Context);
    const HDC dc = HdcVirtualizationService::Get().Resolve(current.Dc);
    switched = context != nullptr && GL_FN(wglMakeCurrent)(dc, context) != 0;
    if (switched) {
      m_LiveDc = dc;
      m_RecordedDc = current.Dc;
    }
  }
  if (!switched) {
    LOG_WARNING << "Subcapture: cannot make context " << current.Context
                << " current, its state is not restored";
    return false;
  }
  m_RecordedDisplay = current.GlxDisplay;
  DrainErrors();
  Emit(current.Call);
  m_ThreadId = current.ThreadId;
  m_UsedThreads[current.ThreadId] = current.GlxDisplay;

  ReadVersion();
  ReadExtensions();
  return true;
}

void StateRestorer::ReadExtensions() {
  m_Extensions.clear();
  if (m_Version.AtLeast(3, 0)) {
    const GLint count = GetInteger(GL_NUM_EXTENSIONS);
    for (GLint i = 0; i < count; ++i) {
      const auto* name =
          reinterpret_cast<const char*>(GL_FN(glGetStringi)(GL_EXTENSIONS, static_cast<GLuint>(i)));
      if (name != nullptr) {
        m_Extensions.insert(name);
      }
    }
  } else if (const auto* names = reinterpret_cast<const char*>(GL_FN(glGetString)(GL_EXTENSIONS))) {
    std::istringstream stream(names);
    for (std::string name; stream >> name;) {
      m_Extensions.insert(name);
    }
  }
}

void StateRestorer::ReadVersion() {
  m_Version = {};
  const auto* version = reinterpret_cast<const char*>(GL_FN(glGetString)(GL_VERSION));
  if (version != nullptr) {
    std::sscanf(version, "%d.%d", &m_Version.Major, &m_Version.Minor);
  }
  m_Version.Compatibility = !m_Version.AtLeast(3, 2) || (GetInteger(GL_CONTEXT_PROFILE_MASK) &
                                                         GL_CONTEXT_COMPATIBILITY_PROFILE_BIT);
}

void StateRestorer::MapNames(GITSKey shareGroup, GITSKey context) {
  m_UnmappedNames.reset();
  m_Keys.clear();
  for (const auto& [key, object] : m_State.Objects()) {
    if (object.Owner != (IsContainerKind(object.Kind) ? context : shareGroup)) {
      continue;
    }
    if (const GLuint name = NameOf(object.Kind, key)) {
      m_Keys[object.Kind][name] = key;
    }
  }
  m_ArbProgramKeys.clear();
  for (const auto& [key, program] : m_State.ArbPrograms()) {
    if (program.ShareGroup != shareGroup) {
      continue;
    }
    if (const GLuint name = NameOf(GLObjectKind::Program, key)) {
      m_ArbProgramKeys[name] = key;
    }
  }
}

GITSKey StateRestorer::ArbProgramKeyOf(GLuint name) const {
  const auto key = m_ArbProgramKeys.find(name);
  return key != m_ArbProgramKeys.end() ? key->second : 0;
}

GITSKey StateRestorer::KeyOf(GLObjectKind kind, GLuint name) {
  if (name == 0) {
    return 0;
  }
  if (m_UnmappedNames) {
    MapNames(m_UnmappedNames->first, m_UnmappedNames->second);
  }
  const auto names = m_Keys.find(kind);
  if (names == m_Keys.end()) {
    return 0;
  }
  const auto key = names->second.find(name);
  if (key == names->second.end()) {
    return 0;
  }
  if (m_Collecting || m_Capture != nullptr) {
    m_Required.insert(key->second);
  }
  return key->second;
}

void StateRestorer::CollectRequired(
    const std::map<GITSKey, std::shared_ptr<MakeCurrent>>& shareGroups) {
  const bool deleted = std::any_of(m_State.Objects().begin(), m_State.Objects().end(),
                                   [](const auto& entry) { return entry.second.Snapshot.Kept(); });
  if (!m_Analysis.Optimize() && !deleted) {
    return;
  }
  if (m_Analysis.Optimize()) {
    m_Required = m_Analysis.Objects();
  }
  // Shaders reach textures through handles in uniforms and buffers the analysis cannot follow,
  // and a handle can be all that keeps a deleted texture or sampler.
  for (const auto& [handle, state] : m_State.Handles()) {
    m_Required.insert(state.Texture);
    if (state.Sampler != 0) {
      m_Required.insert(state.Sampler);
    }
  }
  m_Collecting = true;
  // Bindings first, so the containers they reference are required when their own references
  // are collected.
  for (const auto& [key, context] : m_State.Contexts()) {
    if (!context.LastMakeCurrent || !SwitchTo(*context.LastMakeCurrent)) {
      continue;
    }
    MapNames(context.ShareGroup, key);
    RestoreContextState();
    RestoreBindings();
    RestoreActiveQueries();
    RestoreContainers(key);
  }
  for (const auto& [shareGroup, current] : shareGroups) {
    if (!SwitchTo(*current)) {
      continue;
    }
    MapNames(shareGroup, current->Context);
    for (const auto& [owner, list] : m_State.DisplayLists()) {
      if (owner.first == shareGroup) {
        m_Required.insert(list.Buffers.begin(), list.Buffers.end());
      }
    }
    for (const auto& [key, object] : m_State.Objects()) {
      if (object.Owner != shareGroup || !Required(key)) {
        continue;
      }
      m_Required.insert(object.Snapshot.References.begin(), object.Snapshot.References.end());
      for (GITSKey original = object.ViewOf; original != 0;) {
        m_Required.insert(original);
        const ObjectState* next = m_State.Object(original);
        original = next != nullptr ? next->ViewOf : 0;
      }
      const GLuint name = NameOf(object.Kind, key);
      if (object.Kind == GLObjectKind::Program && name != 0 && GL_FN(glIsProgram)(name)) {
        for (GLuint shader : AttachedShaderNames(name)) {
          KeyOf(GLObjectKind::Shader, shader);
        }
      } else if (object.Kind == GLObjectKind::Texture && object.Target == GL_TEXTURE_BUFFER &&
                 name != 0 && GL_FN(glIsTexture)(name)) {
        const GLuint bound = GetName(GL_TEXTURE_BINDING_BUFFER);
        GL_FN(glBindTexture)(GL_TEXTURE_BUFFER, name);
        GLint buffer = 0;
        GL_FN(glGetTexLevelParameteriv)
        (GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, &buffer);
        KeyOf(GLObjectKind::Buffer, static_cast<GLuint>(buffer));
        GL_FN(glBindTexture)(GL_TEXTURE_BUFFER, bound);
      }
    }
  }
  m_Collecting = false;
  DrainErrors();
  const auto required = std::count_if(m_State.Objects().begin(), m_State.Objects().end(),
                                      [this](const auto& entry) { return Required(entry.first); });
  LOG_INFO << "Subcapture: restoring the contents of " << required << " of "
           << m_State.Objects().size() << " objects";
}

GLuint StateRestorer::NameOf(GLObjectKind kind, GITSKey key) const {
  return key != 0 ? ObjectNameMapService::Get().TryGetHandle(kind, key) : 0;
}

GLenum StateRestorer::TargetOf(GITSKey texture) const {
  const ObjectState* object = m_State.Object(texture);
  return object != nullptr ? object->Target : 0;
}

bool StateRestorer::HasExtension(const char* name) const {
  return m_Extensions.contains(name);
}

std::vector<GLenum> StateRestorer::ArbProgramTargets() const {
  std::vector<GLenum> targets;
  if (HasExtension("GL_ARB_vertex_program")) {
    targets.push_back(GL_VERTEX_PROGRAM_ARB);
  }
  if (HasExtension("GL_ARB_fragment_program")) {
    targets.push_back(GL_FRAGMENT_PROGRAM_ARB);
  }
  return targets;
}

void StateRestorer::EmitCapability(GLenum cap, bool enabled) {
  if (enabled) {
    glEnableCommand command;
    command.m_cap.Value = cap;
    Emit<glEnableCommandSerializer>(command);
  } else {
    glDisableCommand command;
    command.m_cap.Value = cap;
    Emit<glDisableCommandSerializer>(command);
  }
}

void StateRestorer::EmitBlit(GLenum attachment,
                             GLenum internalFormat,
                             GLsizei width,
                             GLsizei height,
                             const PixelFormat& format,
                             std::vector<uint8_t> pixels) {
  const GITSKey texture = m_Recorder.CreateStateRestoreObjectKey();
  glGenTexturesCommand gen;
  gen.m_n.Value = 1;
  gen.m_textures.Key = {texture};
  gen.m_textures.Value = {0};
  gen.m_textures.HasValue = true;
  Emit<glGenTexturesCommandSerializer>(gen);
  glBindTextureCommand bind;
  bind.m_target.Value = GL_TEXTURE_2D;
  bind.m_texture.Key = texture;
  Emit<glBindTextureCommandSerializer>(bind);
  glTexImage2DCommand image;
  image.m_target.Value = GL_TEXTURE_2D;
  image.m_internalformat.Value = static_cast<GLint>(internalFormat);
  image.m_width.Value = width;
  image.m_height.Value = height;
  image.m_format.Value = format.Format;
  image.m_type.Value = format.Type;
  image.m_pixels.Data = std::move(pixels);
  image.m_pixels.HasValue = true;
  Emit<glTexImage2DCommandSerializer>(image);

  const GITSKey framebuffer = m_Recorder.CreateStateRestoreObjectKey();
  glGenFramebuffersCommand genFramebuffer;
  genFramebuffer.m_n.Value = 1;
  genFramebuffer.m_framebuffers.Key = {framebuffer};
  genFramebuffer.m_framebuffers.Value = {0};
  genFramebuffer.m_framebuffers.HasValue = true;
  Emit<glGenFramebuffersCommandSerializer>(genFramebuffer);
  glBindFramebufferCommand bindRead;
  bindRead.m_target.Value = GL_READ_FRAMEBUFFER;
  bindRead.m_framebuffer.Key = framebuffer;
  Emit<glBindFramebufferCommandSerializer>(bindRead);
  glFramebufferTexture2DCommand attach;
  attach.m_target.Value = GL_READ_FRAMEBUFFER;
  attach.m_attachment.Value = attachment;
  attach.m_textarget.Value = GL_TEXTURE_2D;
  attach.m_texture.Key = texture;
  Emit<glFramebufferTexture2DCommandSerializer>(attach);

  GLbitfield mask = GL_COLOR_BUFFER_BIT;
  if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
    mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
  } else if (attachment == GL_DEPTH_ATTACHMENT) {
    mask = GL_DEPTH_BUFFER_BIT;
  }
  glBlitFramebufferCommand blit;
  blit.m_srcX1.Value = width;
  blit.m_srcY1.Value = height;
  blit.m_dstX1.Value = width;
  blit.m_dstY1.Value = height;
  blit.m_mask.Value = mask;
  blit.m_filter.Value = GL_NEAREST;
  Emit<glBlitFramebufferCommandSerializer>(blit);

  glDeleteFramebuffersCommand deleteFramebuffer;
  deleteFramebuffer.m_n.Value = 1;
  deleteFramebuffer.m_framebuffers.Key = {framebuffer};
  deleteFramebuffer.m_framebuffers.Value = {0};
  deleteFramebuffer.m_framebuffers.HasValue = true;
  Emit<glDeleteFramebuffersCommandSerializer>(deleteFramebuffer);
  glDeleteTexturesCommand deleteTexture;
  deleteTexture.m_n.Value = 1;
  deleteTexture.m_textures.Key = {texture};
  deleteTexture.m_textures.Value = {0};
  deleteTexture.m_textures.HasValue = true;
  Emit<glDeleteTexturesCommandSerializer>(deleteTexture);
}

void StateRestorer::BeginReadback() {
  m_SavedPackState.clear();
  for (GLenum pname : PACK_STATE) {
    m_SavedPackState.emplace_back(pname, GetInteger(pname));
    GL_FN(glPixelStorei)(pname, pname == GL_PACK_ALIGNMENT ? 1 : 0);
  }
  if (m_Version.AtLeast(2, 1)) {
    m_SavedPackState.emplace_back(GL_PIXEL_PACK_BUFFER, GetInteger(GL_PIXEL_PACK_BUFFER_BINDING));
    GL_FN(glBindBuffer)(GL_PIXEL_PACK_BUFFER, 0);
  }
}

void StateRestorer::EndReadback() {
  for (const auto& [pname, value] : m_SavedPackState) {
    if (pname == GL_PIXEL_PACK_BUFFER) {
      GL_FN(glBindBuffer)(pname, static_cast<GLuint>(value));
    } else {
      GL_FN(glPixelStorei)(pname, value);
    }
  }
  m_SavedPackState.clear();
}

// Contents of the window or pbuffer the current context draws to. Color and depth-stencil are
// blitted separately, so a depth-stencil format the drawable does not match loses only
// depth-stencil. The front color of a double-buffered drawable is restored too, since a swap
// that exchanges the buffers shows it again after the next swap. A WGL drawable's is drawn to the
// back buffer and swapped to the front rather than drawn to the front buffer, which applications
// that only swap never draw to. A GLX window's is drawn to the front buffer, as glXSwapBuffers
// leaves the back buffer undefined, and has no result a swap could be recorded as failed with, so
// a swap would end a frame. A GLX pbuffer's or pixmap's is not restored, as drawing to it can
// raise X errors when the context is released, and it is never presented.
void StateRestorer::RestoreDefaultFramebuffer() {
  const bool glx = m_RecordedDisplay != nullptr;
  GLsizei width = 0;
  GLsizei height = 0;
  if (glx) {
    unsigned int value[2] = {};
    Display* display = GlxDrawableService::Get().GetDisplay();
    GL_FN(glXQueryDrawable)(display, m_LiveDrawable, GLX_WIDTH, &value[0]);
    GL_FN(glXQueryDrawable)(display, m_LiveDrawable, GLX_HEIGHT, &value[1]);
    width = static_cast<GLsizei>(value[0]);
    height = static_cast<GLsizei>(value[1]);
  } else if (const auto pbuffer = m_State.PbufferSize(m_RecordedDc)) {
    std::tie(width, height) = *pbuffer;
  } else if (!GetWindowClientSize(m_LiveDc, width, height)) {
    return;
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  const GLuint drawBound = GetName(GL_DRAW_FRAMEBUFFER_BINDING);
  const GLuint readBound = GetName(GL_READ_FRAMEBUFFER_BINDING);
  GL_FN(glBindFramebuffer)(GL_FRAMEBUFFER, 0);
  const GLint readBuffer = GetInteger(GL_READ_BUFFER);
  GLboolean doubleBuffered = GL_FALSE;
  GL_FN(glGetBooleanv)(GL_DOUBLEBUFFER, &doubleBuffered);
  const GLenum buffer = doubleBuffered ? GL_BACK : GL_FRONT;
  GL_FN(glReadBuffer)(buffer);

  const auto pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
  const PixelFormat colorFormat{GL_RGBA, GL_UNSIGNED_BYTE, 4};
  std::vector<uint8_t> color(pixelCount * colorFormat.PixelSize);
  GL_FN(glReadPixels)(0, 0, width, height, colorFormat.Format, colorFormat.Type, color.data());

  const auto attachmentSize = [](GLenum attachment, GLenum pname) {
    GLint type = GL_NONE;
    GL_FN(glGetFramebufferAttachmentParameteriv)
    (GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    GLint size = 0;
    if (type != GL_NONE) {
      GL_FN(glGetFramebufferAttachmentParameteriv)(GL_FRAMEBUFFER, attachment, pname, &size);
    }
    return size;
  };
  const GLint depthSize = attachmentSize(GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE);
  const GLint stencilSize = attachmentSize(GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE);
  GLenum depthAttachment = GL_NONE;
  GLenum depthInternalFormat = GL_NONE;
  PixelFormat depthFormat{};
  if (depthSize > 0 && stencilSize > 0) {
    depthAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
    depthInternalFormat = depthSize > 24 ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8;
    depthFormat = depthSize > 24
                      ? PixelFormat{GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8}
                      : PixelFormat{GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4};
  } else if (depthSize > 0) {
    depthAttachment = GL_DEPTH_ATTACHMENT;
    depthInternalFormat = depthSize > 24   ? GL_DEPTH_COMPONENT32
                          : depthSize > 16 ? GL_DEPTH_COMPONENT24
                                           : GL_DEPTH_COMPONENT16;
    depthFormat = {GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 4};
  }
  std::vector<uint8_t> depth;
  if (depthAttachment != GL_NONE) {
    depth.resize(pixelCount * depthFormat.PixelSize);
    GL_FN(glReadPixels)(0, 0, width, height, depthFormat.Format, depthFormat.Type, depth.data());
  }
  std::vector<uint8_t> frontColor;
  if (doubleBuffered != GL_FALSE &&
      (!glx || GlxDrawableService::Get().IsWindow(m_RecordedDrawable))) {
    GL_FN(glReadBuffer)(GL_FRONT);
    frontColor.resize(pixelCount * colorFormat.PixelSize);
    GL_FN(glReadPixels)
    (0, 0, width, height, colorFormat.Format, colorFormat.Type, frontColor.data());
  }

  GL_FN(glReadBuffer)(static_cast<GLenum>(readBuffer));
  GL_FN(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, drawBound);
  GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, readBound);

  for (GLenum pname : UNPACK_STATE) {
    glPixelStoreiCommand command;
    command.m_pname.Value = pname;
    command.m_param.Value = DefaultPixelStore(pname);
    Emit<glPixelStoreiCommandSerializer>(command);
  }
  glBindBufferCommand unpackBuffer;
  unpackBuffer.m_target.Value = GL_PIXEL_UNPACK_BUFFER;
  Emit<glBindBufferCommandSerializer>(unpackBuffer);
  glBindFramebufferCommand bindDraw;
  bindDraw.m_target.Value = GL_DRAW_FRAMEBUFFER;
  Emit<glBindFramebufferCommandSerializer>(bindDraw);
  glDrawBufferCommand drawBuffer;
  if (!frontColor.empty() && glx) {
    drawBuffer.m_buf.Value = GL_FRONT;
    Emit<glDrawBufferCommandSerializer>(drawBuffer);
    EmitBlit(GL_COLOR_ATTACHMENT0, GL_RGBA8, width, height, colorFormat, std::move(frontColor));
  }
  drawBuffer.m_buf.Value = buffer;
  Emit<glDrawBufferCommandSerializer>(drawBuffer);
  if (!frontColor.empty() && !glx) {
    EmitBlit(GL_COLOR_ATTACHMENT0, GL_RGBA8, width, height, colorFormat, std::move(frontColor));
    // A swap recorded as failed does not end a frame.
    wglSwapBuffersCommand swap;
    swap.m_hdc.Value = m_RecordedDc;
    swap.m_Return.Value = 0;
    Emit<wglSwapBuffersCommandSerializer>(swap);
  }
  EmitBlit(GL_COLOR_ATTACHMENT0, GL_RGBA8, width, height, colorFormat, std::move(color));
  if (depthAttachment != GL_NONE) {
    EmitBlit(depthAttachment, depthInternalFormat, width, height, depthFormat, std::move(depth));
  }
}

void StateRestorer::RestoreContextState() {
  for (const Capability& capability : CAPABILITIES) {
    if (m_Version.AtLeast(capability.Major, capability.Minor)) {
      EmitCapability(capability.Cap, GL_FN(glIsEnabled)(capability.Cap) != GL_FALSE);
    }
  }
  const GLint clipDistances =
      GetInteger(m_Version.AtLeast(3, 0) ? GL_MAX_CLIP_DISTANCES : GL_MAX_CLIP_PLANES);
  for (GLint i = 0; i < clipDistances; ++i) {
    const GLenum cap = GL_CLIP_DISTANCE0 + static_cast<GLenum>(i);
    if (GL_FN(glIsEnabled)(cap)) {
      EmitCapability(cap, true);
    }
  }

  if (m_Version.AtLeast(1, 4)) {
    glBlendFuncSeparateCommand command;
    command.m_sfactorRGB.Value = GetInteger(GL_BLEND_SRC_RGB);
    command.m_dfactorRGB.Value = GetInteger(GL_BLEND_DST_RGB);
    command.m_sfactorAlpha.Value = GetInteger(GL_BLEND_SRC_ALPHA);
    command.m_dfactorAlpha.Value = GetInteger(GL_BLEND_DST_ALPHA);
    Emit<glBlendFuncSeparateCommandSerializer>(command);
  } else {
    glBlendFuncCommand command;
    command.m_sfactor.Value = GetInteger(GL_BLEND_SRC);
    command.m_dfactor.Value = GetInteger(GL_BLEND_DST);
    Emit<glBlendFuncCommandSerializer>(command);
  }
  if (m_Version.AtLeast(2, 0)) {
    glBlendEquationSeparateCommand command;
    command.m_modeRGB.Value = GetInteger(GL_BLEND_EQUATION_RGB);
    command.m_modeAlpha.Value = GetInteger(GL_BLEND_EQUATION_ALPHA);
    Emit<glBlendEquationSeparateCommandSerializer>(command);
  } else if (m_Version.AtLeast(1, 4)) {
    glBlendEquationCommand command;
    command.m_mode.Value = GetInteger(GL_BLEND_EQUATION);
    Emit<glBlendEquationCommandSerializer>(command);
  }
  if (m_Version.AtLeast(1, 4)) {
    GLfloat color[4]{};
    GL_FN(glGetFloatv)(GL_BLEND_COLOR, color);
    glBlendColorCommand command;
    command.m_red.Value = color[0];
    command.m_green.Value = color[1];
    command.m_blue.Value = color[2];
    command.m_alpha.Value = color[3];
    Emit<glBlendColorCommandSerializer>(command);
  }
  {
    GLboolean mask[4]{};
    GL_FN(glGetBooleanv)(GL_COLOR_WRITEMASK, mask);
    glColorMaskCommand command;
    command.m_red.Value = mask[0];
    command.m_green.Value = mask[1];
    command.m_blue.Value = mask[2];
    command.m_alpha.Value = mask[3];
    Emit<glColorMaskCommandSerializer>(command);
  }
  if (m_Version.AtLeast(3, 0)) {
    RestoreDrawBufferState();
  }
  {
    glDepthFuncCommand func;
    func.m_func.Value = GetInteger(GL_DEPTH_FUNC);
    Emit<glDepthFuncCommandSerializer>(func);
    GLboolean writeMask = GL_TRUE;
    GL_FN(glGetBooleanv)(GL_DEPTH_WRITEMASK, &writeMask);
    glDepthMaskCommand mask;
    mask.m_flag.Value = writeMask;
    Emit<glDepthMaskCommandSerializer>(mask);
    GLdouble range[2]{};
    GL_FN(glGetDoublev)(GL_DEPTH_RANGE, range);
    glDepthRangeCommand depthRange;
    depthRange.m_n.Value = range[0];
    depthRange.m_f.Value = range[1];
    Emit<glDepthRangeCommandSerializer>(depthRange);
  }

  struct StencilFace {
    GLenum Face, Func, Ref, ValueMask, Fail, DepthFail, DepthPass, WriteMask;
  };
  constexpr StencilFace STENCIL_FACES[] = {
      {GL_FRONT, GL_STENCIL_FUNC, GL_STENCIL_REF, GL_STENCIL_VALUE_MASK, GL_STENCIL_FAIL,
       GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS, GL_STENCIL_WRITEMASK},
      {GL_BACK, GL_STENCIL_BACK_FUNC, GL_STENCIL_BACK_REF, GL_STENCIL_BACK_VALUE_MASK,
       GL_STENCIL_BACK_FAIL, GL_STENCIL_BACK_PASS_DEPTH_FAIL, GL_STENCIL_BACK_PASS_DEPTH_PASS,
       GL_STENCIL_BACK_WRITEMASK},
  };
  for (const StencilFace& face : STENCIL_FACES) {
    glStencilFuncSeparateCommand func;
    func.m_face.Value = face.Face;
    func.m_func.Value = GetInteger(face.Func);
    func.m_ref.Value = GetInteger(face.Ref);
    func.m_mask.Value = static_cast<GLuint>(GetInteger(face.ValueMask));
    Emit<glStencilFuncSeparateCommandSerializer>(func);
    glStencilOpSeparateCommand op;
    op.m_face.Value = face.Face;
    op.m_sfail.Value = GetInteger(face.Fail);
    op.m_dpfail.Value = GetInteger(face.DepthFail);
    op.m_dppass.Value = GetInteger(face.DepthPass);
    Emit<glStencilOpSeparateCommandSerializer>(op);
    glStencilMaskSeparateCommand mask;
    mask.m_face.Value = face.Face;
    mask.m_mask.Value = static_cast<GLuint>(GetInteger(face.WriteMask));
    Emit<glStencilMaskSeparateCommandSerializer>(mask);
  }

  {
    GLfloat color[4]{};
    GL_FN(glGetFloatv)(GL_COLOR_CLEAR_VALUE, color);
    glClearColorCommand command;
    command.m_red.Value = color[0];
    command.m_green.Value = color[1];
    command.m_blue.Value = color[2];
    command.m_alpha.Value = color[3];
    Emit<glClearColorCommandSerializer>(command);
    GLdouble depth = 1.0;
    GL_FN(glGetDoublev)(GL_DEPTH_CLEAR_VALUE, &depth);
    glClearDepthCommand clearDepth;
    clearDepth.m_depth.Value = depth;
    Emit<glClearDepthCommandSerializer>(clearDepth);
    glClearStencilCommand clearStencil;
    clearStencil.m_s.Value = GetInteger(GL_STENCIL_CLEAR_VALUE);
    Emit<glClearStencilCommandSerializer>(clearStencil);
  }
  {
    glCullFaceCommand cullFace;
    cullFace.m_mode.Value = GetInteger(GL_CULL_FACE_MODE);
    Emit<glCullFaceCommandSerializer>(cullFace);
    glFrontFaceCommand frontFace;
    frontFace.m_mode.Value = GetInteger(GL_FRONT_FACE);
    Emit<glFrontFaceCommandSerializer>(frontFace);
    GLint modes[2]{GL_FILL, GL_FILL};
    GL_FN(glGetIntegerv)(GL_POLYGON_MODE, modes);
    const bool separate = m_Version.Compatibility && modes[0] != modes[1];
    glPolygonModeCommand polygonMode;
    polygonMode.m_face.Value = separate ? GL_FRONT : GL_FRONT_AND_BACK;
    polygonMode.m_mode.Value = modes[0];
    Emit<glPolygonModeCommandSerializer>(polygonMode);
    if (separate) {
      polygonMode.m_face.Value = GL_BACK;
      polygonMode.m_mode.Value = modes[1];
      Emit<glPolygonModeCommandSerializer>(polygonMode);
    }
    if (m_Version.AtLeast(4, 6)) {
      glPolygonOffsetClampCommand polygonOffset;
      polygonOffset.m_factor.Value = GetFloat(GL_POLYGON_OFFSET_FACTOR);
      polygonOffset.m_units.Value = GetFloat(GL_POLYGON_OFFSET_UNITS);
      polygonOffset.m_clamp.Value = GetFloat(GL_POLYGON_OFFSET_CLAMP);
      Emit<glPolygonOffsetClampCommandSerializer>(polygonOffset);
    } else {
      glPolygonOffsetCommand polygonOffset;
      polygonOffset.m_factor.Value = GetFloat(GL_POLYGON_OFFSET_FACTOR);
      polygonOffset.m_units.Value = GetFloat(GL_POLYGON_OFFSET_UNITS);
      Emit<glPolygonOffsetCommandSerializer>(polygonOffset);
    }
    glLineWidthCommand lineWidth;
    lineWidth.m_width.Value = GetFloat(GL_LINE_WIDTH);
    Emit<glLineWidthCommandSerializer>(lineWidth);
    glPointSizeCommand pointSize;
    pointSize.m_size.Value = GetFloat(GL_POINT_SIZE);
    Emit<glPointSizeCommandSerializer>(pointSize);
    glLogicOpCommand logicOp;
    logicOp.m_opcode.Value = GetInteger(GL_LOGIC_OP_MODE);
    Emit<glLogicOpCommandSerializer>(logicOp);
    GLboolean invert = GL_FALSE;
    GL_FN(glGetBooleanv)(GL_SAMPLE_COVERAGE_INVERT, &invert);
    glSampleCoverageCommand sampleCoverage;
    sampleCoverage.m_value.Value = GetFloat(GL_SAMPLE_COVERAGE_VALUE);
    sampleCoverage.m_invert.Value = invert;
    Emit<glSampleCoverageCommandSerializer>(sampleCoverage);
  }
  if (m_Version.AtLeast(3, 2)) {
    const GLint words = GetInteger(GL_MAX_SAMPLE_MASK_WORDS);
    for (GLint i = 0; i < words; ++i) {
      glSampleMaskiCommand command;
      command.m_maskNumber.Value = static_cast<GLuint>(i);
      command.m_mask.Value =
          static_cast<GLbitfield>(GetIntegerIndexed(GL_SAMPLE_MASK_VALUE, static_cast<GLuint>(i)));
      Emit<glSampleMaskiCommandSerializer>(command);
    }
  }
  if (m_Version.AtLeast(4, 0)) {
    glMinSampleShadingCommand command;
    command.m_value.Value = GetFloat(GL_MIN_SAMPLE_SHADING_VALUE);
    Emit<glMinSampleShadingCommandSerializer>(command);
  }
  if (m_Version.AtLeast(1, 4)) {
    glPointParameterfCommand command;
    command.m_pname.Value = GL_POINT_FADE_THRESHOLD_SIZE;
    command.m_param.Value = GetFloat(GL_POINT_FADE_THRESHOLD_SIZE);
    Emit<glPointParameterfCommandSerializer>(command);
  }
  if (m_Version.AtLeast(2, 0)) {
    glPointParameteriCommand command;
    command.m_pname.Value = GL_POINT_SPRITE_COORD_ORIGIN;
    command.m_param.Value = GetInteger(GL_POINT_SPRITE_COORD_ORIGIN);
    Emit<glPointParameteriCommandSerializer>(command);
  }
  if (m_Version.AtLeast(3, 0)) {
    glClampColorCommand command;
    command.m_target.Value = GL_CLAMP_READ_COLOR;
    command.m_clamp.Value = GetInteger(GL_CLAMP_READ_COLOR);
    Emit<glClampColorCommandSerializer>(command);
  }
  {
    GLint box[4]{};
    GL_FN(glGetIntegerv)(GL_VIEWPORT, box);
    glViewportCommand viewport;
    viewport.m_x.Value = box[0];
    viewport.m_y.Value = box[1];
    viewport.m_width.Value = box[2];
    viewport.m_height.Value = box[3];
    Emit<glViewportCommandSerializer>(viewport);
    GL_FN(glGetIntegerv)(GL_SCISSOR_BOX, box);
    glScissorCommand scissor;
    scissor.m_x.Value = box[0];
    scissor.m_y.Value = box[1];
    scissor.m_width.Value = box[2];
    scissor.m_height.Value = box[3];
    Emit<glScissorCommandSerializer>(scissor);
  }
  if (m_Version.AtLeast(3, 1)) {
    glPrimitiveRestartIndexCommand command;
    command.m_index.Value = static_cast<GLuint>(GetInteger(GL_PRIMITIVE_RESTART_INDEX));
    Emit<glPrimitiveRestartIndexCommandSerializer>(command);
  }
  if (m_Version.AtLeast(3, 2)) {
    glProvokingVertexCommand command;
    command.m_mode.Value = GetInteger(GL_PROVOKING_VERTEX);
    Emit<glProvokingVertexCommandSerializer>(command);
  }
  if (m_Version.AtLeast(4, 0)) {
    glPatchParameteriCommand command;
    command.m_pname.Value = GL_PATCH_VERTICES;
    command.m_value.Value = GetInteger(GL_PATCH_VERTICES);
    Emit<glPatchParameteriCommandSerializer>(command);
    for (const auto& [pname, count] :
         {std::pair{GL_PATCH_DEFAULT_OUTER_LEVEL, 4}, std::pair{GL_PATCH_DEFAULT_INNER_LEVEL, 2}}) {
      glPatchParameterfvCommand levels;
      levels.m_pname.Value = pname;
      levels.m_values.Data.resize(4);
      levels.m_values.HasValue = true;
      GL_FN(glGetFloatv)(pname, levels.m_values.Data.data());
      levels.m_values.Data.resize(count);
      Emit<glPatchParameterfvCommandSerializer>(levels);
    }
  }
  for (GLenum hint : HINTS) {
    glHintCommand command;
    command.m_target.Value = hint;
    command.m_mode.Value = GetInteger(hint);
    Emit<glHintCommandSerializer>(command);
  }
  if (m_Version.AtLeast(2, 0)) {
    RestoreVertexAttributeValues();
  }
  if (m_Version.Compatibility) {
    RestoreCompatibilityState();
  }
  for (const GLenum* states : {UNPACK_STATE, PACK_STATE}) {
    for (size_t i = 0; i < std::size(UNPACK_STATE); ++i) {
      glPixelStoreiCommand command;
      command.m_pname.Value = states[i];
      command.m_param.Value = GetInteger(states[i]);
      Emit<glPixelStoreiCommandSerializer>(command);
    }
  }
}

// Blend and color mask state of each draw buffer, which the non-indexed queries report for draw
// buffer 0 only.
void StateRestorer::RestoreDrawBufferState() {
  const GLint drawBuffers = GetInteger(GL_MAX_DRAW_BUFFERS);
  for (GLint i = 0; i < drawBuffers; ++i) {
    const auto index = static_cast<GLuint>(i);
    if (GL_FN(glIsEnabledi)(GL_BLEND, index)) {
      glEnableiCommand command;
      command.m_target.Value = GL_BLEND;
      command.m_index.Value = index;
      Emit<glEnableiCommandSerializer>(command);
    } else {
      glDisableiCommand command;
      command.m_target.Value = GL_BLEND;
      command.m_index.Value = index;
      Emit<glDisableiCommandSerializer>(command);
    }
    GLboolean mask[4]{GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    GL_FN(glGetBooleani_v)(GL_COLOR_WRITEMASK, index, mask);
    glColorMaskiCommand colorMask;
    colorMask.m_index.Value = index;
    colorMask.m_r.Value = mask[0];
    colorMask.m_g.Value = mask[1];
    colorMask.m_b.Value = mask[2];
    colorMask.m_a.Value = mask[3];
    Emit<glColorMaskiCommandSerializer>(colorMask);
    if (!m_Version.AtLeast(4, 0)) {
      continue;
    }
    glBlendFuncSeparateiCommand func;
    func.m_buf.Value = index;
    func.m_srcRGB.Value = GetIntegerIndexed(GL_BLEND_SRC_RGB, index);
    func.m_dstRGB.Value = GetIntegerIndexed(GL_BLEND_DST_RGB, index);
    func.m_srcAlpha.Value = GetIntegerIndexed(GL_BLEND_SRC_ALPHA, index);
    func.m_dstAlpha.Value = GetIntegerIndexed(GL_BLEND_DST_ALPHA, index);
    Emit<glBlendFuncSeparateiCommandSerializer>(func);
    glBlendEquationSeparateiCommand equation;
    equation.m_buf.Value = index;
    equation.m_modeRGB.Value = GetIntegerIndexed(GL_BLEND_EQUATION_RGB, index);
    equation.m_modeAlpha.Value = GetIntegerIndexed(GL_BLEND_EQUATION_ALPHA, index);
    Emit<glBlendEquationSeparateiCommandSerializer>(equation);
  }
}

// Current values of generic vertex attributes, which disabled attribute arrays read. Attribute 0
// of a compatibility context aliases the vertex position and has none.
void StateRestorer::RestoreVertexAttributeValues() {
  const GLint attributes =
      std::min(GetInteger(GL_MAX_VERTEX_ATTRIBS), MAX_RESTORED_VERTEX_ATTRIBUTES);
  for (GLint i = m_Version.Compatibility ? 1 : 0; i < attributes; ++i) {
    glVertexAttrib4fvCommand command;
    command.m_index.Value = static_cast<GLuint>(i);
    command.m_v.Data.resize(4);
    command.m_v.HasValue = true;
    GL_FN(glGetVertexAttribfv)
    (command.m_index.Value, GL_CURRENT_VERTEX_ATTRIB, command.m_v.Data.data());
    const std::vector<GLfloat> defaults{0.0f, 0.0f, 0.0f, 1.0f};
    if (command.m_v.Data != defaults) {
      Emit<glVertexAttrib4fvCommandSerializer>(command);
    }
  }
}

// A query begun before the range can end in it. Active queries begin again last, so nothing else
// the restore draws counts in them.
void StateRestorer::RestoreActiveQueries() {
  const GLint streams = m_Version.AtLeast(4, 0) ? GetInteger(GL_MAX_VERTEX_STREAMS) : 1;
  for (const QueryTarget& target : QUERY_TARGETS) {
    if (!m_Version.AtLeast(target.Major, target.Minor)) {
      continue;
    }
    for (GLint i = 0; i < (target.Indexed ? streams : 1); ++i) {
      const auto index = static_cast<GLuint>(i);
      GLint query = 0;
      if (index == 0) {
        GL_FN(glGetQueryiv)(target.Target, GL_CURRENT_QUERY, &query);
      } else {
        GL_FN(glGetQueryIndexediv)(target.Target, index, GL_CURRENT_QUERY, &query);
      }
      const GITSKey key = KeyOf(GLObjectKind::Query, static_cast<GLuint>(query));
      if (key == 0) {
        continue;
      }
      if (index == 0) {
        glBeginQueryCommand command;
        command.m_target.Value = target.Target;
        command.m_id.Key = key;
        Emit<glBeginQueryCommandSerializer>(command);
      } else {
        glBeginQueryIndexedCommand command;
        command.m_target.Value = target.Target;
        command.m_index.Value = index;
        command.m_id.Key = key;
        Emit<glBeginQueryIndexedCommandSerializer>(command);
      }
    }
  }
}

// Restores the indexed transform feedback buffers of the bound transform feedback, or the other
// indexed buffers of the context.
void StateRestorer::RestoreIndexedBuffers(bool transformFeedback) {
  for (const IndexedBufferTarget& target : INDEXED_BUFFER_TARGETS) {
    if ((target.Target == GL_TRANSFORM_FEEDBACK_BUFFER) != transformFeedback ||
        !m_Version.AtLeast(target.Major, target.Minor)) {
      continue;
    }
    const GLint bindings = GetInteger(target.MaxBindings);
    for (GLint i = 0; i < bindings; ++i) {
      const auto index = static_cast<GLuint>(i);
      const GLuint name = static_cast<GLuint>(GetIntegerIndexed(target.Binding, index));
      if (name == 0) {
        continue;
      }
      const GLint64 size = GetInteger64Indexed(target.Size, index);
      if (size > 0) {
        glBindBufferRangeCommand command;
        command.m_target.Value = target.Target;
        command.m_index.Value = index;
        command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, name);
        command.m_offset.Value = static_cast<GLintptr>(GetInteger64Indexed(target.Start, index));
        command.m_size.Value = static_cast<GLsizeiptr>(size);
        Emit<glBindBufferRangeCommandSerializer>(command);
      } else {
        glBindBufferBaseCommand command;
        command.m_target.Value = target.Target;
        command.m_index.Value = index;
        command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, name);
        Emit<glBindBufferBaseCommandSerializer>(command);
      }
    }
  }
}

void StateRestorer::RestoreTransformFeedback(GITSKey key, GLuint name) {
  glGenTransformFeedbacksCommand gen;
  gen.m_n.Value = 1;
  gen.m_ids.Key = {key};
  gen.m_ids.Value = {0};
  gen.m_ids.HasValue = true;
  Emit<glGenTransformFeedbacksCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsTransformFeedback)(name) ||
      !TransformFeedbackBindable()) {
    return;
  }
  const GLuint bound = GetName(GL_TRANSFORM_FEEDBACK_BINDING);
  GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, name);
  glBindTransformFeedbackCommand bind;
  bind.m_target.Value = GL_TRANSFORM_FEEDBACK;
  bind.m_id.Key = key;
  Emit<glBindTransformFeedbackCommandSerializer>(bind);
  RestoreIndexedBuffers(true);
  GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, bound);
  glBindTransformFeedbackCommand unbind;
  unbind.m_target.Value = GL_TRANSFORM_FEEDBACK;
  Emit<glBindTransformFeedbackCommandSerializer>(unbind);
}

void StateRestorer::RestoreBindings() {
  const bool vertexArrays = m_Version.AtLeast(3, 0);
  if (m_Version.Compatibility) {
    const GLuint current = vertexArrays ? GetName(GL_VERTEX_ARRAY_BINDING) : 0;
    if (vertexArrays) {
      GL_FN(glBindVertexArray)(0);
      glBindVertexArrayCommand command;
      Emit<glBindVertexArrayCommandSerializer>(command);
    }
    RestoreVertexAttributes();
    if (vertexArrays) {
      GL_FN(glBindVertexArray)(current);
    }
  }
  if (vertexArrays) {
    glBindVertexArrayCommand command;
    command.m_array.Key = KeyOf(GLObjectKind::VertexArray, GetName(GL_VERTEX_ARRAY_BINDING));
    Emit<glBindVertexArrayCommandSerializer>(command);
  }

  RestoreIndexedBuffers(false);
  const bool transformFeedbacks =
      m_Version.AtLeast(4, 0) || HasExtension("GL_ARB_transform_feedback2");
  const GLuint transformFeedback = transformFeedbacks ? GetName(GL_TRANSFORM_FEEDBACK_BINDING) : 0;
  if (transformFeedback == 0) {
    RestoreIndexedBuffers(true);
  } else {
    // The default transform feedback's buffers are read with it bound.
    if (TransformFeedbackBindable()) {
      GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, 0);
      RestoreIndexedBuffers(true);
      GL_FN(glBindTransformFeedback)(GL_TRANSFORM_FEEDBACK, transformFeedback);
    }
    glBindTransformFeedbackCommand command;
    command.m_target.Value = GL_TRANSFORM_FEEDBACK;
    command.m_id.Key = KeyOf(GLObjectKind::TransformFeedback, transformFeedback);
    Emit<glBindTransformFeedbackCommandSerializer>(command);
  }
  for (const BufferTarget& target : BUFFER_TARGETS) {
    if (m_Version.AtLeast(target.Major, target.Minor)) {
      glBindBufferCommand command;
      command.m_target.Value = target.Target;
      command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, GetName(target.Binding));
      Emit<glBindBufferCommandSerializer>(command);
    }
  }

  const GLint activeTexture = GetInteger(GL_ACTIVE_TEXTURE);
  const GLint units = GetInteger(m_Version.AtLeast(2, 0) ? GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
                                                         : GL_MAX_TEXTURE_UNITS);
  const bool samplers = m_Version.AtLeast(3, 3);
  for (GLint unit = 0; unit < units; ++unit) {
    GL_FN(glActiveTexture)(GL_TEXTURE0 + static_cast<GLenum>(unit));
    std::vector<std::pair<GLenum, GITSKey>> textures;
    for (const TextureTarget& target : TEXTURE_TARGETS) {
      if (!m_Version.AtLeast(target.Major, target.Minor)) {
        continue;
      }
      const GITSKey key = KeyOf(GLObjectKind::Texture, GetName(target.Binding));
      if (unit == 0 || key != 0) {
        textures.emplace_back(target.Target, key);
      }
    }
    const GITSKey sampler =
        samplers ? KeyOf(GLObjectKind::Sampler, GetName(GL_SAMPLER_BINDING)) : 0;
    if (textures.empty() && sampler == 0) {
      continue;
    }
    glActiveTextureCommand active;
    active.m_texture.Value = GL_TEXTURE0 + static_cast<GLenum>(unit);
    Emit<glActiveTextureCommandSerializer>(active);
    for (const auto& [target, key] : textures) {
      glBindTextureCommand command;
      command.m_target.Value = target;
      command.m_texture.Key = key;
      Emit<glBindTextureCommandSerializer>(command);
    }
    if (sampler != 0) {
      glBindSamplerCommand command;
      command.m_unit.Value = static_cast<GLuint>(unit);
      command.m_sampler.Key = sampler;
      Emit<glBindSamplerCommandSerializer>(command);
    }
  }
  GL_FN(glActiveTexture)(static_cast<GLenum>(activeTexture));
  {
    glActiveTextureCommand command;
    command.m_texture.Value = static_cast<GLenum>(activeTexture);
    Emit<glActiveTextureCommandSerializer>(command);
  }

  if (m_Version.AtLeast(4, 2)) {
    const GLint imageUnits = GetInteger(GL_MAX_IMAGE_UNITS);
    for (GLint i = 0; i < imageUnits; ++i) {
      const auto unit = static_cast<GLuint>(i);
      const auto texture = static_cast<GLuint>(GetIntegerIndexed(GL_IMAGE_BINDING_NAME, unit));
      if (texture == 0) {
        continue;
      }
      glBindImageTextureCommand command;
      command.m_unit.Value = unit;
      command.m_texture.Key = KeyOf(GLObjectKind::Texture, texture);
      command.m_level.Value = GetIntegerIndexed(GL_IMAGE_BINDING_LEVEL, unit);
      command.m_layered.Value =
          static_cast<GLboolean>(GetIntegerIndexed(GL_IMAGE_BINDING_LAYERED, unit));
      command.m_layer.Value = GetIntegerIndexed(GL_IMAGE_BINDING_LAYER, unit);
      command.m_access.Value = GetIntegerIndexed(GL_IMAGE_BINDING_ACCESS, unit);
      command.m_format.Value = GetIntegerIndexed(GL_IMAGE_BINDING_FORMAT, unit);
      Emit<glBindImageTextureCommandSerializer>(command);
    }
  }

  if (m_Version.AtLeast(4, 1)) {
    if (const GLuint pipeline = GetName(GL_PROGRAM_PIPELINE_BINDING)) {
      glBindProgramPipelineCommand command;
      command.m_pipeline.Key = KeyOf(GLObjectKind::ProgramPipeline, pipeline);
      Emit<glBindProgramPipelineCommandSerializer>(command);
    }
  }
  if (m_Version.AtLeast(2, 0)) {
    const GLuint program = GetName(GL_CURRENT_PROGRAM);
    glUseProgramCommand command;
    command.m_program.Key = KeyOf(GLObjectKind::Program, program);
    Emit<glUseProgramCommandSerializer>(command);
    if (program != 0 && m_Version.AtLeast(4, 0)) {
      RestoreSubroutines(program);
    }
  }
  if (m_Version.Compatibility) {
    RestoreArbProgramBindings();
  }

  if (!m_Version.AtLeast(3, 0)) {
    glDrawBufferCommand drawBuffer;
    drawBuffer.m_buf.Value = GetInteger(GL_DRAW_BUFFER);
    Emit<glDrawBufferCommandSerializer>(drawBuffer);
    glReadBufferCommand readBuffer;
    readBuffer.m_src.Value = GetInteger(GL_READ_BUFFER);
    Emit<glReadBufferCommandSerializer>(readBuffer);
    return;
  }

  {
    glBindRenderbufferCommand command;
    command.m_target.Value = GL_RENDERBUFFER;
    command.m_renderbuffer.Key =
        KeyOf(GLObjectKind::Renderbuffer, GetName(GL_RENDERBUFFER_BINDING));
    Emit<glBindRenderbufferCommandSerializer>(command);
  }

  const GLuint drawFramebuffer = GetName(GL_DRAW_FRAMEBUFFER_BINDING);
  const GLuint readFramebuffer = GetName(GL_READ_FRAMEBUFFER_BINDING);
  GL_FN(glBindFramebuffer)(GL_FRAMEBUFFER, 0);
  {
    glBindFramebufferCommand bind;
    bind.m_target.Value = GL_FRAMEBUFFER;
    Emit<glBindFramebufferCommandSerializer>(bind);
    glDrawBufferCommand drawBuffer;
    drawBuffer.m_buf.Value = GetInteger(GL_DRAW_BUFFER);
    Emit<glDrawBufferCommandSerializer>(drawBuffer);
    glReadBufferCommand readBuffer;
    readBuffer.m_src.Value = GetInteger(GL_READ_BUFFER);
    Emit<glReadBufferCommandSerializer>(readBuffer);
  }
  GL_FN(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
  GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, readFramebuffer);
  glBindFramebufferCommand bindDraw;
  bindDraw.m_target.Value = GL_DRAW_FRAMEBUFFER;
  bindDraw.m_framebuffer.Key = KeyOf(GLObjectKind::Framebuffer, drawFramebuffer);
  Emit<glBindFramebufferCommandSerializer>(bindDraw);
  glBindFramebufferCommand bindRead;
  bindRead.m_target.Value = GL_READ_FRAMEBUFFER;
  bindRead.m_framebuffer.Key = KeyOf(GLObjectKind::Framebuffer, readFramebuffer);
  Emit<glBindFramebufferCommandSerializer>(bindRead);
}

} // namespace gits::opengl
