// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateRestorer.h"

#include "commandSerializersAuto.h"
#include "liveGl.h"

#include <algorithm>

namespace gits::opengl2 {

namespace {

GLint GetAttachment(GLenum attachment, GLenum pname) {
  GLint value = 0;
  GL_FN(glGetFramebufferAttachmentParameteriv)(GL_FRAMEBUFFER, attachment, pname, &value);
  return value;
}

} // namespace

void StateRestorer::RestoreContainers(GITSKey context) {
  for (const auto& [key, object] : m_State.Objects()) {
    if (object.Owner != context) {
      continue;
    }
    const GLuint name = NameOf(object.Kind, key);
    switch (object.Kind) {
    case GLObjectKind::VertexArray:
      RestoreVertexArray(key, name);
      break;
    case GLObjectKind::Framebuffer:
      RestoreFramebuffer(key, name);
      break;
    case GLObjectKind::Query: {
      glGenQueriesCommand command;
      command.m_n.Value = 1;
      command.m_ids.Key = {key};
      command.m_ids.Value = {0};
      command.m_ids.HasValue = true;
      Emit<glGenQueriesCommandSerializer>(command);
      break;
    }
    case GLObjectKind::ProgramPipeline:
      RestoreProgramPipeline(key, name);
      break;
    case GLObjectKind::TransformFeedback:
      RestoreTransformFeedback(key, name);
      break;
    default:
      break;
    }
  }
}

void StateRestorer::RestoreVertexArray(GITSKey key, GLuint name) {
  glGenVertexArraysCommand gen;
  gen.m_n.Value = 1;
  gen.m_arrays.Key = {key};
  gen.m_arrays.Value = {0};
  gen.m_arrays.HasValue = true;
  Emit<glGenVertexArraysCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsVertexArray)(name)) {
    return;
  }
  const GLuint bound = GetName(GL_VERTEX_ARRAY_BINDING);
  GL_FN(glBindVertexArray)(name);
  glBindVertexArrayCommand bind;
  bind.m_array.Key = key;
  Emit<glBindVertexArrayCommandSerializer>(bind);
  RestoreVertexAttributes();
  GL_FN(glBindVertexArray)(bound);
}

// Restores the attributes of the bound vertex array into the vertex array bound in the
// subcapture. Attributes sourced from client memory are restored by the draws that use them.
void StateRestorer::RestoreVertexAttributes() {
  const GLint attributes =
      std::min(GetInteger(GL_MAX_VERTEX_ATTRIBS), MAX_RESTORED_VERTEX_ATTRIBUTES);
  const bool integer = m_Version.AtLeast(3, 0);
  const bool divisor = m_Version.AtLeast(3, 3);
  const bool doubles = m_Version.AtLeast(4, 1);
  const bool separateFormat = m_Version.AtLeast(4, 4);
  for (GLint i = 0; i < attributes; ++i) {
    const auto index = static_cast<GLuint>(i);
    const auto buffer =
        static_cast<GLuint>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING));
    if (separateFormat) {
      RestoreVertexAttributeFormat(index, buffer != 0);
    } else if (buffer != 0) {
      void* pointer = nullptr;
      GL_FN(glGetVertexAttribPointerv)(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
      const GLint size = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_SIZE);
      const auto type = static_cast<GLenum>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_TYPE));
      const GLint stride = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE);
      const auto offset = reinterpret_cast<uintptr_t>(pointer);

      glBindBufferCommand bind;
      bind.m_target.Value = GL_ARRAY_BUFFER;
      bind.m_buffer.Key = KeyOf(GLObjectKind::Buffer, buffer);
      Emit<glBindBufferCommandSerializer>(bind);
      if (doubles && GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_LONG)) {
        glVertexAttribLPointerCommand command;
        command.m_index.Value = index;
        command.m_size.Value = size;
        command.m_type.Value = type;
        command.m_stride.Value = stride;
        command.m_pointer.Value = offset;
        Emit<glVertexAttribLPointerCommandSerializer>(command);
      } else if (integer && GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER)) {
        glVertexAttribIPointerCommand command;
        command.m_index.Value = index;
        command.m_size.Value = size;
        command.m_type.Value = type;
        command.m_stride.Value = stride;
        command.m_pointer.Value = offset;
        Emit<glVertexAttribIPointerCommandSerializer>(command);
      } else {
        glVertexAttribPointerCommand command;
        command.m_index.Value = index;
        command.m_size.Value = size;
        command.m_type.Value = type;
        command.m_normalized.Value =
            static_cast<GLboolean>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED));
        command.m_stride.Value = stride;
        command.m_pointer.Value = offset;
        Emit<glVertexAttribPointerCommandSerializer>(command);
      }
    }
    if (GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED)) {
      glEnableVertexAttribArrayCommand command;
      command.m_index.Value = index;
      Emit<glEnableVertexAttribArrayCommandSerializer>(command);
    }
    if (divisor && !separateFormat) {
      if (const GLint value = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR)) {
        glVertexAttribDivisorCommand command;
        command.m_index.Value = index;
        command.m_divisor.Value = static_cast<GLuint>(value);
        Emit<glVertexAttribDivisorCommandSerializer>(command);
      }
    }
  }
  if (separateFormat) {
    RestoreVertexBufferBindings();
  }
  if (m_Version.Compatibility) {
    RestoreClientArrays();
  }
  if (const GLuint elements = GetName(GL_ELEMENT_ARRAY_BUFFER_BINDING)) {
    glBindBufferCommand bind;
    bind.m_target.Value = GL_ELEMENT_ARRAY_BUFFER;
    bind.m_buffer.Key = KeyOf(GLObjectKind::Buffer, elements);
    Emit<glBindBufferCommandSerializer>(bind);
  }
}

// The format of an attribute and the vertex buffer binding it reads. Written for attributes with
// a buffer or a state other than the initial one, so a later glBindVertexBuffer finds it.
void StateRestorer::RestoreVertexAttributeFormat(GLuint index, bool hasBuffer) {
  const GLint size = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_SIZE);
  const auto type = static_cast<GLenum>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_TYPE));
  const auto normalized =
      static_cast<GLboolean>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED));
  const bool integer = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER) != 0;
  const bool doubles = GetVertexAttribute(index, GL_VERTEX_ATTRIB_ARRAY_LONG) != 0;
  const auto offset =
      static_cast<GLuint>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_RELATIVE_OFFSET));
  const auto binding = static_cast<GLuint>(GetVertexAttribute(index, GL_VERTEX_ATTRIB_BINDING));
  const bool initial = size == 4 && type == GL_FLOAT && !normalized && !integer && !doubles &&
                       offset == 0 && binding == index;
  if (!hasBuffer && initial) {
    return;
  }
  if (doubles) {
    glVertexAttribLFormatCommand command;
    command.m_attribindex.Value = index;
    command.m_size.Value = size;
    command.m_type.Value = type;
    command.m_relativeoffset.Value = offset;
    Emit<glVertexAttribLFormatCommandSerializer>(command);
  } else if (integer) {
    glVertexAttribIFormatCommand command;
    command.m_attribindex.Value = index;
    command.m_size.Value = size;
    command.m_type.Value = type;
    command.m_relativeoffset.Value = offset;
    Emit<glVertexAttribIFormatCommandSerializer>(command);
  } else {
    glVertexAttribFormatCommand command;
    command.m_attribindex.Value = index;
    command.m_size.Value = size;
    command.m_type.Value = type;
    command.m_normalized.Value = normalized;
    command.m_relativeoffset.Value = offset;
    Emit<glVertexAttribFormatCommandSerializer>(command);
  }
  glVertexAttribBindingCommand command;
  command.m_attribindex.Value = index;
  command.m_bindingindex.Value = binding;
  Emit<glVertexAttribBindingCommandSerializer>(command);
}

void StateRestorer::RestoreVertexBufferBindings() {
  const GLint bindings = GetInteger(GL_MAX_VERTEX_ATTRIB_BINDINGS);
  for (GLint i = 0; i < bindings; ++i) {
    const auto index = static_cast<GLuint>(i);
    if (const auto buffer =
            static_cast<GLuint>(GetIntegerIndexed(GL_VERTEX_BINDING_BUFFER, index))) {
      glBindVertexBufferCommand command;
      command.m_bindingindex.Value = index;
      command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, buffer);
      command.m_offset.Value =
          static_cast<GLintptr>(GetInteger64Indexed(GL_VERTEX_BINDING_OFFSET, index));
      command.m_stride.Value = GetIntegerIndexed(GL_VERTEX_BINDING_STRIDE, index);
      Emit<glBindVertexBufferCommandSerializer>(command);
    }
    if (const GLint divisor = GetIntegerIndexed(GL_VERTEX_BINDING_DIVISOR, index)) {
      glVertexBindingDivisorCommand command;
      command.m_bindingindex.Value = index;
      command.m_divisor.Value = static_cast<GLuint>(divisor);
      Emit<glVertexBindingDivisorCommandSerializer>(command);
    }
  }
}

void StateRestorer::RestoreProgramPipeline(GITSKey key, GLuint name) {
  glGenProgramPipelinesCommand gen;
  gen.m_n.Value = 1;
  gen.m_pipelines.Key = {key};
  gen.m_pipelines.Value = {0};
  gen.m_pipelines.HasValue = true;
  Emit<glGenProgramPipelinesCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsProgramPipeline)(name)) {
    return;
  }
  struct Stage {
    GLenum Shader;
    GLbitfield Bit;
  };
  std::vector<Stage> stages{
      {GL_VERTEX_SHADER, GL_VERTEX_SHADER_BIT},
      {GL_TESS_CONTROL_SHADER, GL_TESS_CONTROL_SHADER_BIT},
      {GL_TESS_EVALUATION_SHADER, GL_TESS_EVALUATION_SHADER_BIT},
      {GL_GEOMETRY_SHADER, GL_GEOMETRY_SHADER_BIT},
      {GL_FRAGMENT_SHADER, GL_FRAGMENT_SHADER_BIT},
  };
  if (m_Version.AtLeast(4, 3)) {
    stages.push_back({GL_COMPUTE_SHADER, GL_COMPUTE_SHADER_BIT});
  }
  for (const Stage& stage : stages) {
    GLint program = 0;
    GL_FN(glGetProgramPipelineiv)(name, stage.Shader, &program);
    if (program != 0) {
      glUseProgramStagesCommand command;
      command.m_pipeline.Key = key;
      command.m_stages.Value = stage.Bit;
      command.m_program.Key = KeyOf(GLObjectKind::Program, static_cast<GLuint>(program));
      Emit<glUseProgramStagesCommandSerializer>(command);
    }
  }
  GLint active = 0;
  GL_FN(glGetProgramPipelineiv)(name, GL_ACTIVE_PROGRAM, &active);
  if (active != 0) {
    glActiveShaderProgramCommand command;
    command.m_pipeline.Key = key;
    command.m_program.Key = KeyOf(GLObjectKind::Program, static_cast<GLuint>(active));
    Emit<glActiveShaderProgramCommandSerializer>(command);
  }
}

void StateRestorer::RestoreFramebuffer(GITSKey key, GLuint name) {
  glGenFramebuffersCommand gen;
  gen.m_n.Value = 1;
  gen.m_framebuffers.Key = {key};
  gen.m_framebuffers.Value = {0};
  gen.m_framebuffers.HasValue = true;
  Emit<glGenFramebuffersCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsFramebuffer)(name)) {
    return;
  }
  const GLuint drawBound = GetName(GL_DRAW_FRAMEBUFFER_BINDING);
  const GLuint readBound = GetName(GL_READ_FRAMEBUFFER_BINDING);
  GL_FN(glBindFramebuffer)(GL_FRAMEBUFFER, name);
  glBindFramebufferCommand bind;
  bind.m_target.Value = GL_FRAMEBUFFER;
  bind.m_framebuffer.Key = key;
  Emit<glBindFramebufferCommandSerializer>(bind);

  std::vector<GLenum> attachments{GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
  const GLint colorAttachments = GetInteger(GL_MAX_COLOR_ATTACHMENTS);
  for (GLint i = 0; i < colorAttachments; ++i) {
    attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
  }
  struct DeletedAttachment {
    GLenum Attachment;
    GITSKey Key;
    GLint Level;
  };
  std::vector<DeletedAttachment> deletedAttachments;
  const auto addIfDeleted = [&](GLenum attachment, GITSKey key, GLint level) {
    const ObjectState* object = m_State.Object(key);
    if (object == nullptr || object->Snapshot.Commands.empty()) {
      return;
    }
    // A depth-stencil image attached to both points is read back once, with both.
    if (attachment == GL_STENCIL_ATTACHMENT && !deletedAttachments.empty() &&
        deletedAttachments.front().Key == key) {
      deletedAttachments.front().Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
      return;
    }
    deletedAttachments.push_back({attachment, key, level});
  };
  for (GLenum attachment : attachments) {
    const GLint type = GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE);
    const auto object =
        static_cast<GLuint>(GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME));
    if (type == GL_RENDERBUFFER) {
      glFramebufferRenderbufferCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      command.m_attachment.Value = attachment;
      command.m_renderbuffertarget.Value = GL_RENDERBUFFER;
      command.m_renderbuffer.Key = KeyOf(GLObjectKind::Renderbuffer, object);
      Emit<glFramebufferRenderbufferCommandSerializer>(command);
      addIfDeleted(attachment, command.m_renderbuffer.Key, 0);
      continue;
    }
    if (type != GL_TEXTURE) {
      continue;
    }
    const GITSKey texture = KeyOf(GLObjectKind::Texture, object);
    const GLenum target = TargetOf(texture);
    const GLint level = GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL);
    const GLint layer = GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER);
    const bool layered = m_Version.AtLeast(3, 2) &&
                         GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_LAYERED) != 0;
    if (!layered) {
      addIfDeleted(attachment, texture, level);
    }
    if (layered) {
      glFramebufferTextureCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      command.m_attachment.Value = attachment;
      command.m_texture.Key = texture;
      command.m_level.Value = level;
      Emit<glFramebufferTextureCommandSerializer>(command);
    } else if (target == GL_TEXTURE_1D) {
      glFramebufferTexture1DCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      command.m_attachment.Value = attachment;
      command.m_textarget.Value = target;
      command.m_texture.Key = texture;
      command.m_level.Value = level;
      Emit<glFramebufferTexture1DCommandSerializer>(command);
    } else if (target == GL_TEXTURE_3D || target == GL_TEXTURE_1D_ARRAY ||
               target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY ||
               target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
      glFramebufferTextureLayerCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      command.m_attachment.Value = attachment;
      command.m_texture.Key = texture;
      command.m_level.Value = level;
      command.m_layer.Value = layer;
      Emit<glFramebufferTextureLayerCommandSerializer>(command);
    } else {
      glFramebufferTexture2DCommand command;
      command.m_target.Value = GL_FRAMEBUFFER;
      command.m_attachment.Value = attachment;
      command.m_textarget.Value =
          target == GL_TEXTURE_CUBE_MAP
              ? static_cast<GLenum>(
                    GetAttachment(attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE))
              : (target != 0 ? target : GL_TEXTURE_2D);
      command.m_texture.Key = texture;
      command.m_level.Value = level;
      Emit<glFramebufferTexture2DCommandSerializer>(command);
    }
  }
  for (const DeletedAttachment& deleted : deletedAttachments) {
    RestoreDeletedAttachment(key, deleted.Attachment, deleted.Key, deleted.Level);
  }

  const GLint maxDrawBuffers = GetInteger(GL_MAX_DRAW_BUFFERS);
  std::vector<GLenum> drawBuffers;
  for (GLint i = 0; i < maxDrawBuffers; ++i) {
    drawBuffers.push_back(static_cast<GLenum>(GetInteger(GL_DRAW_BUFFER0 + i)));
  }
  while (drawBuffers.size() > 1 && drawBuffers.back() == GL_NONE) {
    drawBuffers.pop_back();
  }
  glDrawBuffersCommand draw;
  draw.m_n.Value = static_cast<GLsizei>(drawBuffers.size());
  draw.m_bufs.Data = std::move(drawBuffers);
  draw.m_bufs.HasValue = true;
  Emit<glDrawBuffersCommandSerializer>(draw);
  glReadBufferCommand read;
  read.m_src.Value = GetInteger(GL_READ_BUFFER);
  Emit<glReadBufferCommandSerializer>(read);

  GL_FN(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, drawBound);
  GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, readBound);
}

// The framebuffers a deleted texture or renderbuffer is attached to can still render to it after
// its deletion, so the attached image is read back through the bound framebuffer and blitted into
// the attachment of the restored one, which the subcapture has bound.
void StateRestorer::RestoreDeletedAttachment(GITSKey framebuffer,
                                             GLenum attachment,
                                             GITSKey key,
                                             GLint level) {
  const ObjectState* object = m_State.Object(key);
  if (m_Collecting || attachment == GL_STENCIL_ATTACHMENT || !Required(key)) {
    return;
  }
  const ObjectSnapshot& snapshot = object->Snapshot;
  if (snapshot.Samples > 0 || snapshot.Width == 0) {
    return;
  }
  const GLsizei width = std::max(snapshot.Width >> level, 1);
  const GLsizei height =
      object->Target == GL_TEXTURE_1D_ARRAY ? 1 : std::max(snapshot.Height >> level, 1);
  const bool color = attachment != GL_DEPTH_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT;

  const GLint readBuffer = GetInteger(GL_READ_BUFFER);
  if (color) {
    GL_FN(glReadBuffer)(attachment);
  }
  const PixelFormat format = AttachmentReadbackFormat(attachment);
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) *
                              format.PixelSize);
  GL_FN(glReadPixels)(0, 0, width, height, format.Format, format.Type, pixels.data());
  if (color) {
    GL_FN(glReadBuffer)(static_cast<GLenum>(readBuffer));
    glDrawBuffersCommand draw;
    draw.m_n.Value = 1;
    draw.m_bufs.Data = {attachment};
    draw.m_bufs.HasValue = true;
    Emit<glDrawBuffersCommandSerializer>(draw);
  }
  // The temporary read framebuffer reads its first color attachment.
  EmitBlit(color ? GL_COLOR_ATTACHMENT0 : attachment, snapshot.InternalFormat, width, height,
           format, std::move(pixels));
  glBindFramebufferCommand bind;
  bind.m_target.Value = GL_FRAMEBUFFER;
  bind.m_framebuffer.Key = framebuffer;
  Emit<glBindFramebufferCommandSerializer>(bind);
}

} // namespace gits::opengl2
