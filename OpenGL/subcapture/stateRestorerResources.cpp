// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateRestorer.h"

#include "clientArraysMetaRunner.h"
#include "clientArraysMetaTypes.h"
#include "commandSerializersAuto.h"
#include "liveGl.h"
#include "log.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

namespace gits::opengl {

namespace {

constexpr GLint MAX_TEXTURE_LEVELS = 16;

constexpr GLenum CUBE_MAP_FACES[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

constexpr GLenum TEXTURE_INTEGER_PARAMETERS[] = {
    GL_TEXTURE_MIN_FILTER,   GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S,    GL_TEXTURE_WRAP_T,
    GL_TEXTURE_WRAP_R,       GL_TEXTURE_BASE_LEVEL, GL_TEXTURE_MAX_LEVEL, GL_TEXTURE_COMPARE_MODE,
    GL_TEXTURE_COMPARE_FUNC, GL_TEXTURE_SWIZZLE_R,  GL_TEXTURE_SWIZZLE_G, GL_TEXTURE_SWIZZLE_B,
    GL_TEXTURE_SWIZZLE_A,
};

constexpr GLenum SAMPLER_INTEGER_PARAMETERS[] = {
    GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,   GL_TEXTURE_WRAP_S,       GL_TEXTURE_WRAP_T,
    GL_TEXTURE_WRAP_R,     GL_TEXTURE_COMPARE_MODE, GL_TEXTURE_COMPARE_FUNC,
};

constexpr GLenum FLOAT_PARAMETERS[] = {
    GL_TEXTURE_MIN_LOD,
    GL_TEXTURE_MAX_LOD,
    GL_TEXTURE_LOD_BIAS,
};

constexpr size_t MAX_SLAB_SIZE = size_t{256} << 20;

// Pixel rows of every format here are a multiple of 4 bytes, so the default unpack alignment
// of the subcapture contexts reads them back unchanged.
PixelFormat ReadbackFormat(GLenum imageTarget, GLint level, bool compatibility) {
  GLint depthSize = 0;
  GLint stencilSize = 0;
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_DEPTH_SIZE, &depthSize);
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_STENCIL_SIZE, &stencilSize);
  if (depthSize > 0 && stencilSize > 0) {
    GLint depthType = 0;
    GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_DEPTH_TYPE, &depthType);
    return depthType == GL_FLOAT
               ? PixelFormat{GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8}
               : PixelFormat{GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4};
  }
  if (depthSize > 0) {
    return {GL_DEPTH_COMPONENT, GL_FLOAT, 4};
  }
  if (stencilSize > 0) {
    return {GL_STENCIL_INDEX, GL_UNSIGNED_INT, 4};
  }

  std::vector<GLenum> typeQueries{GL_TEXTURE_RED_TYPE, GL_TEXTURE_ALPHA_TYPE};
  std::vector<GLenum> sizeQueries{GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE,
                                  GL_TEXTURE_ALPHA_SIZE};
  if (compatibility) {
    typeQueries.insert(typeQueries.end(), {GL_TEXTURE_LUMINANCE_TYPE, GL_TEXTURE_INTENSITY_TYPE});
    sizeQueries.insert(sizeQueries.end(), {GL_TEXTURE_LUMINANCE_SIZE, GL_TEXTURE_INTENSITY_SIZE});
  }
  GLint componentType = GL_NONE;
  for (GLenum pname : typeQueries) {
    GL_FN(glGetTexLevelParameteriv)(imageTarget, level, pname, &componentType);
    if (componentType != GL_NONE) {
      break;
    }
  }
  GLint bits = 0;
  GLint otherBits = 0;
  for (GLenum pname : sizeQueries) {
    GLint size = 0;
    GL_FN(glGetTexLevelParameteriv)(imageTarget, level, pname, &size);
    bits = std::max(bits, size);
    if (pname == GL_TEXTURE_GREEN_SIZE || pname == GL_TEXTURE_BLUE_SIZE ||
        pname == GL_TEXTURE_ALPHA_SIZE) {
      otherBits = std::max(otherBits, size);
    }
  }
  // A lone red, luminance or intensity channel is read as red, a quarter of the RGBA size.
  const bool red = otherBits == 0;
  if (componentType == GL_INT) {
    return red ? PixelFormat{GL_RED_INTEGER, GL_INT, 4} : PixelFormat{GL_RGBA_INTEGER, GL_INT, 16};
  }
  if (componentType == GL_UNSIGNED_INT) {
    return red ? PixelFormat{GL_RED_INTEGER, GL_UNSIGNED_INT, 4}
               : PixelFormat{GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16};
  }
  if (componentType == GL_UNSIGNED_NORMALIZED && bits <= 8) {
    return {GL_RGBA, GL_UNSIGNED_BYTE, 4};
  }
  return red ? PixelFormat{GL_RED, GL_FLOAT, 4} : PixelFormat{GL_RGBA, GL_FLOAT, 16};
}

struct TextureLevel {
  GLint Width{};
  GLint Height{};
  GLint Depth{};
  GLint InternalFormat{};
  bool Compressed{};
  GLint CompressedSize{};
};

TextureLevel QueryLevel(GLenum imageTarget, GLint level) {
  TextureLevel info;
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_WIDTH, &info.Width);
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_HEIGHT, &info.Height);
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_DEPTH, &info.Depth);
  GL_FN(glGetTexLevelParameteriv)
  (imageTarget, level, GL_TEXTURE_INTERNAL_FORMAT, &info.InternalFormat);
  GLint compressed = 0;
  GL_FN(glGetTexLevelParameteriv)(imageTarget, level, GL_TEXTURE_COMPRESSED, &compressed);
  info.Compressed = compressed != 0;
  if (info.Compressed) {
    GL_FN(glGetTexLevelParameteriv)
    (imageTarget, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &info.CompressedSize);
  }
  return info;
}

// Compressed images read with glGetCompressedTexImage can differ from the uploaded data. Their
// blocks copied into the texels of an uncompressed texture of the same size read back exactly.
// Images that are not a whole number of blocks are read directly.
void ReadCompressedImage(GLenum target,
                         GLuint texture,
                         GLenum imageTarget,
                         GLint level,
                         const TextureLevel& info,
                         std::vector<uint8_t>& data,
                         bool copy) {
  GLint blockWidth = 0;
  GLint blockHeight = 0;
  if (copy) {
    const auto format = static_cast<GLenum>(info.InternalFormat);
    GL_FN(glGetInternalformativ)
    (GL_TEXTURE_2D, format, GL_TEXTURE_COMPRESSED_BLOCK_WIDTH, 1, &blockWidth);
    GL_FN(glGetInternalformativ)
    (GL_TEXTURE_2D, format, GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT, 1, &blockHeight);
  }
  const GLsizei width = info.Width;
  const GLsizei height = std::max(info.Height, 1);
  const GLsizei layers = std::max(info.Depth, 1);
  const bool whole =
      blockWidth > 0 && blockHeight > 0 && width % blockWidth == 0 && height % blockHeight == 0;
  const GLsizei columns = whole ? width / blockWidth : 0;
  const GLsizei rows = whole ? height / blockHeight : 0;
  // The block size follows from the image size.
  const size_t blocks = static_cast<size_t>(columns) * rows * layers;
  const size_t blockSize = blocks > 0 && data.size() % blocks == 0 ? data.size() / blocks : 0;
  if (blockSize != 8 && blockSize != 16) {
    GL_FN(glGetCompressedTexImage)(imageTarget, level, data.data());
    return;
  }
  const bool wide = blockSize == 16;
  const GLint face =
      target == GL_TEXTURE_CUBE_MAP ? static_cast<GLint>(imageTarget - CUBE_MAP_FACES[0]) : 0;
  // A 3D texture can be deeper than an array texture can have layers, so the layers are copied
  // in parts.
  const GLsizei maxLayers = std::max(GetInteger(GL_MAX_ARRAY_TEXTURE_LAYERS), 1);
  const size_t layerSize = static_cast<size_t>(columns) * rows * blockSize;
  const GLuint bound = GetName(GL_TEXTURE_BINDING_2D_ARRAY);
  for (GLsizei first = 0; first < layers; first += maxLayers) {
    const GLsizei count = std::min(maxLayers, layers - first);
    GLuint uncompressed = 0;
    GL_FN(glGenTextures)(1, &uncompressed);
    GL_FN(glBindTexture)(GL_TEXTURE_2D_ARRAY, uncompressed);
    GL_FN(glTexStorage3D)
    (GL_TEXTURE_2D_ARRAY, 1, wide ? GL_RGBA32UI : GL_RG32UI, columns, rows, count);
    GL_FN(glCopyImageSubData)
    (texture, target, level, 0, 0, face + first, uncompressed, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
     width, height, count);
    GL_FN(glGetTexImage)
    (GL_TEXTURE_2D_ARRAY, 0, wide ? GL_RGBA_INTEGER : GL_RG_INTEGER, GL_UNSIGNED_INT,
     data.data() + first * layerSize);
    GL_FN(glDeleteTextures)(1, &uncompressed);
  }
  GL_FN(glBindTexture)(GL_TEXTURE_2D_ARRAY, bound);
}

// Levels above log2 of the maximum size are invalid to query.
GLint MaxLevels(GLenum target) {
  if (target == GL_TEXTURE_RECTANGLE) {
    return 1;
  }
  const GLenum pname = target == GL_TEXTURE_3D         ? GL_MAX_3D_TEXTURE_SIZE
                       : target == GL_TEXTURE_CUBE_MAP ? GL_MAX_CUBE_MAP_TEXTURE_SIZE
                                                       : GL_MAX_TEXTURE_SIZE;
  GLint levels = 1;
  for (GLint size = GetInteger(pname); size > 1; size >>= 1) {
    ++levels;
  }
  return std::min(levels, MAX_TEXTURE_LEVELS);
}

int UploadDimensions(GLenum target) {
  switch (target) {
  case GL_TEXTURE_1D:
    return 1;
  case GL_TEXTURE_3D:
  case GL_TEXTURE_2D_ARRAY:
  case GL_TEXTURE_CUBE_MAP_ARRAY:
  case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
    return 3;
  default:
    return 2;
  }
}

} // namespace

ObjectSnapshot StateRestorer::CaptureDeleted(uint32_t threadId, GLObjectKind kind, GITSKey key) {
  ObjectSnapshot snapshot;
  const ObjectState* object = m_State.Object(key);
  const GLuint name = NameOf(kind, key);
  if (object == nullptr || object->Kind != kind || object->DeletePending || name == 0) {
    return snapshot;
  }
  ReadVersion();
  ReadExtensions();
  if (kind == GLObjectKind::Buffer) {
    if (!GL_FN(glIsBuffer)(name)) {
      return snapshot;
    }
    // Reading a buffer mapped without GL_MAP_PERSISTENT_BIT would raise a GL error.
    const GLuint bound = GetName(GL_COPY_READ_BUFFER_BINDING);
    GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, name);
    GLint mapped = GL_FALSE;
    GLint access = 0;
    GL_FN(glGetBufferParameteriv)(GL_COPY_READ_BUFFER, GL_BUFFER_MAPPED, &mapped);
    if (mapped && (m_Version.AtLeast(4, 4) || HasExtension("GL_ARB_buffer_storage"))) {
      GL_FN(glGetBufferParameteriv)(GL_COPY_READ_BUFFER, GL_BUFFER_ACCESS_FLAGS, &access);
    }
    GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, bound);
    if (mapped && (access & GL_MAP_PERSISTENT_BIT) == 0) {
      return snapshot;
    }
  } else if (kind == GLObjectKind::Texture   ? !GL_FN(glIsTexture)(name)
             : kind == GLObjectKind::Sampler ? !GL_FN(glIsSampler)(name)
                                             : !GL_FN(glIsRenderbuffer)(name)) {
    return snapshot;
  }
  const auto owner = m_State.CurrentShareGroup(threadId);
  if (!m_State.HasResidentHandle(key) && !(kind == GLObjectKind::Texture && m_State.Viewed(key)) &&
      (kind == GLObjectKind::Sampler || !Attached(owner.first, owner.second, kind, name, true))) {
    return snapshot;
  }
  if (const TextureTarget* binding = FindTextureTarget(object->Target);
      kind == GLObjectKind::Texture && binding != nullptr) {
    const GLuint bound = GetName(binding->Binding);
    GL_FN(glBindTexture)(object->Target, name);
    snapshot.Storage = ViewWholeStorage(name, object->Target);
    if (snapshot.Storage != 0) {
      m_Capture = &snapshot.Parameters;
      RestoreTextureParameters(object->Target);
      m_Capture = nullptr;
    }
    GL_FN(glBindTexture)(object->Target, bound);
    if (snapshot.Storage != 0) {
      return snapshot;
    }
  }

  m_UnmappedNames = owner;
  m_Required.insert(key);
  m_Capture = &snapshot.Commands;
  BeginReadback();
  if (kind == GLObjectKind::Buffer) {
    RestoreBuffer(key, name);
  } else if (kind == GLObjectKind::Texture) {
    RestoreTexture(key, name, object->Target);
  } else if (kind == GLObjectKind::Sampler) {
    RestoreSampler(key, name);
  } else {
    RestoreRenderbuffer(key, name);
  }
  EndReadback();
  m_Capture = nullptr;
  m_Required.erase(key);
  snapshot.References.assign(m_Required.begin(), m_Required.end());

  if (kind == GLObjectKind::Renderbuffer) {
    const GLuint bound = GetName(GL_RENDERBUFFER_BINDING);
    GL_FN(glBindRenderbuffer)(GL_RENDERBUFFER, name);
    GLint internalFormat = 0;
    GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &snapshot.Width);
    GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &snapshot.Height);
    GL_FN(glGetRenderbufferParameteriv)
    (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &internalFormat);
    GL_FN(glGetRenderbufferParameteriv)
    (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &snapshot.Samples);
    GL_FN(glBindRenderbuffer)(GL_RENDERBUFFER, bound);
    snapshot.InternalFormat = static_cast<GLenum>(internalFormat);
  } else if (const TextureTarget* binding = FindTextureTarget(object->Target);
             kind == GLObjectKind::Texture && binding != nullptr &&
             object->Target != GL_TEXTURE_BUFFER) {
    const GLuint bound = GetName(binding->Binding);
    GL_FN(glBindTexture)(object->Target, name);
    const TextureLevel level =
        QueryLevel(object->Target == GL_TEXTURE_CUBE_MAP ? CUBE_MAP_FACES[0] : object->Target, 0);
    if (object->Target == GL_TEXTURE_2D_MULTISAMPLE ||
        object->Target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
      GL_FN(glGetTexLevelParameteriv)(object->Target, 0, GL_TEXTURE_SAMPLES, &snapshot.Samples);
    }
    GL_FN(glBindTexture)(object->Target, bound);
    snapshot.Width = level.Width;
    snapshot.Height = level.Height;
    snapshot.InternalFormat = static_cast<GLenum>(level.InternalFormat);
  }
  return snapshot;
}

GLuint StateRestorer::ViewWholeStorage(GLuint name, GLenum target) {
  if (target == GL_TEXTURE_BUFFER ||
      !(m_Version.AtLeast(4, 3) || HasExtension("GL_ARB_texture_view"))) {
    return 0;
  }
  GLint immutable = GL_FALSE;
  GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
  if (immutable == GL_FALSE) {
    return 0;
  }
  // The view levels and layers of immutable storage are all of it.
  GLint levels = 0;
  GLint layers = 0;
  GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_VIEW_NUM_LEVELS, &levels);
  GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_VIEW_NUM_LAYERS, &layers);
  const TextureLevel base =
      QueryLevel(target == GL_TEXTURE_CUBE_MAP ? CUBE_MAP_FACES[0] : target, 0);
  GLuint storage = 0;
  GL_FN(glGenTextures)(1, &storage);
  GL_FN(glTextureView)
  (storage, target, name, static_cast<GLenum>(base.InternalFormat), 0, levels, 0, layers);
  // A name glTextureView rejected has no texture, so the texture is read back instead.
  if (!GL_FN(glIsTexture)(storage)) {
    GL_FN(glDeleteTextures)(1, &storage);
    return 0;
  }
  return storage;
}

std::vector<GITSKey> StateRestorer::DetachedDeleted(uint32_t threadId) {
  std::vector<GITSKey> detached;
  const auto [shareGroup, context] = m_State.CurrentShareGroup(threadId);
  for (const auto& [key, object] : m_State.Objects()) {
    if (object.Snapshot.Kept() && object.DeletedIn == context && object.Owner == shareGroup) {
      detached.push_back(key);
    }
  }
  if (detached.empty()) {
    return detached;
  }
  ReadVersion();
  std::erase_if(detached, [&](GITSKey key) {
    const ObjectState& object = *m_State.Object(key);
    return m_State.HasResidentHandle(key) || m_State.Viewed(key) ||
           (object.Kind != GLObjectKind::Sampler &&
            Attached(shareGroup, context, object.Kind, object.LiveName, false));
  });
  return detached;
}

void StateRestorer::RestoreSharedObjects(GITSKey shareGroup) {
  const auto restore = [&](GLObjectKind kind, auto restoreObject) {
    for (const auto& [key, object] : m_State.Objects()) {
      if (object.Kind != kind || object.Owner != shareGroup) {
        continue;
      }
      if (object.Snapshot.Commands.empty()) {
        restoreObject(key, NameOf(kind, key), object);
      } else {
        RestoreDeleted(key, object);
      }
    }
  };
  restore(GLObjectKind::Buffer,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreBuffer(key, name); });
  restore(GLObjectKind::Texture, [this](GITSKey key, GLuint name, const ObjectState& object) {
    if (object.View == nullptr) {
      RestoreTexture(key, object.Snapshot.Storage != 0 ? object.Snapshot.Storage : name,
                     object.Target);
    }
  });
  std::set<GITSKey> views;
  for (const auto& [key, object] : m_State.Objects()) {
    if (object.View != nullptr && object.Owner == shareGroup && object.Snapshot.Commands.empty()) {
      RestoreTextureView(key, views);
    }
  }
  restore(GLObjectKind::Renderbuffer,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreRenderbuffer(key, name); });
  restore(GLObjectKind::Sampler,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreSampler(key, name); });
  // A handle makes the state of its texture and sampler immutable, so it is created after them.
  for (const auto& [handle, state] : m_State.Handles()) {
    const ObjectState* texture = m_State.Object(state.Texture);
    if (texture != nullptr && texture->Owner == shareGroup) {
      state.Creation->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    }
  }
  restore(GLObjectKind::Shader,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreShader(key, name); });
  restore(GLObjectKind::Program,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreProgram(key, name); });
  restore(GLObjectKind::Shader, [this](GITSKey key, GLuint name, const ObjectState& object) {
    if (object.DeletePending && name != 0 && GL_FN(glIsShader)(name)) {
      glDeleteShaderCommand command;
      command.m_shader.Key = key;
      Emit<glDeleteShaderCommandSerializer>(command);
    }
  });
  RestoreArbPrograms(shareGroup);

  for (const auto& [key, owner] : m_State.Syncs()) {
    if (owner == shareGroup) {
      glFenceSyncCommand command;
      command.m_condition.Value = GL_SYNC_GPU_COMMANDS_COMPLETE;
      command.m_Return.Key = key;
      Emit<glFenceSyncCommandSerializer>(command);
    }
  }
}

// A deleted object a container may still reference is created again as it was before its
// deletion.
void StateRestorer::RestoreDeleted(GITSKey key, const ObjectState& object) {
  if (!Required(key)) {
    return;
  }
  for (const auto& command : object.Snapshot.Commands) {
    command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
  }
}

// Client memory the player holds for arrays, which draws of the range can reference without
// sending it again.
void StateRestorer::RestoreClientArrayBlocks() {
  ClientArraysMetaCommand command;
  VisitClientArrayBlocks([&command](uint64_t id, const std::vector<uint8_t>& bytes) {
    command.m_BlockIds.Data.push_back(id);
    command.m_BlockSizes.Data.push_back(bytes.size());
    command.m_Data.Data.insert(command.m_Data.Data.end(), bytes.begin(), bytes.end());
  });
  if (command.m_BlockIds.Data.empty()) {
    return;
  }
  command.m_BlockIds.HasValue = true;
  command.m_BlockSizes.HasValue = true;
  command.m_Data.HasValue = true;
  Emit<ClientArraysMetaSerializer>(command);
}

// Display lists are compiled again from their recorded commands. Commands no list compiles run
// as they did before the range; the context state restored after them overrides their effect.
void StateRestorer::RestoreDisplayLists(GITSKey shareGroup) {
  for (const FontLists& font : m_State.Fonts()) {
    if (font.ShareGroup != shareGroup) {
      continue;
    }
    for (const auto& command : font.Commands) {
      command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    }
  }
  for (const auto& [owner, list] : m_State.DisplayLists()) {
    if (owner.first != shareGroup) {
      continue;
    }
    for (const auto& command : list.Commands) {
      command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    }
  }
}

void StateRestorer::RestoreBuffer(GITSKey key, GLuint name) {
  glGenBuffersCommand gen;
  gen.m_n.Value = 1;
  gen.m_buffers.Key = {key};
  gen.m_buffers.Value = {0};
  gen.m_buffers.HasValue = true;
  Emit<glGenBuffersCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsBuffer)(name)) {
    return;
  }

  const GLuint bound = GetName(GL_COPY_READ_BUFFER_BINDING);
  GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, name);
  GLint64 size = 0;
  GL_FN(glGetBufferParameteri64v)(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);
  GLint usage = GL_STATIC_DRAW;
  GL_FN(glGetBufferParameteriv)(GL_COPY_READ_BUFFER, GL_BUFFER_USAGE, &usage);
  GLint immutable = GL_FALSE;
  GLint flags = 0;
  if (m_Version.AtLeast(4, 4) || HasExtension("GL_ARB_buffer_storage")) {
    GL_FN(glGetBufferParameteriv)(GL_COPY_READ_BUFFER, GL_BUFFER_IMMUTABLE_STORAGE, &immutable);
    GL_FN(glGetBufferParameteriv)(GL_COPY_READ_BUFFER, GL_BUFFER_STORAGE_FLAGS, &flags);
  }
  std::vector<uint8_t> data(static_cast<size_t>(std::max<GLint64>(size, 0)));
  if (!data.empty()) {
    GL_FN(glGetBufferSubData)
    (GL_COPY_READ_BUFFER, 0, static_cast<GLsizeiptr>(data.size()), data.data());
  }
  GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, bound);

  glBindBufferCommand bind;
  bind.m_target.Value = GL_ARRAY_BUFFER;
  bind.m_buffer.Key = key;
  Emit<glBindBufferCommandSerializer>(bind);
  if (immutable) {
    glBufferStorageCommand storage;
    storage.m_target.Value = GL_ARRAY_BUFFER;
    storage.m_size.Value = static_cast<GLsizeiptr>(data.size());
    storage.m_data.Data = std::move(data);
    storage.m_data.HasValue = !storage.m_data.Data.empty();
    storage.m_flags.Value = static_cast<GLbitfield>(flags);
    Emit<glBufferStorageCommandSerializer>(storage);
  } else {
    glBufferDataCommand bufferData;
    bufferData.m_target.Value = GL_ARRAY_BUFFER;
    bufferData.m_size.Value = static_cast<GLsizeiptr>(data.size());
    bufferData.m_data.Data = std::move(data);
    bufferData.m_data.HasValue = !bufferData.m_data.Data.empty();
    bufferData.m_usage.Value = static_cast<GLenum>(usage);
    Emit<glBufferDataCommandSerializer>(bufferData);
  }
}

void StateRestorer::RestoreTexture(GITSKey key, GLuint name, GLenum target) {
  glGenTexturesCommand gen;
  gen.m_n.Value = 1;
  gen.m_textures.Key = {key};
  gen.m_textures.Value = {0};
  gen.m_textures.HasValue = true;
  Emit<glGenTexturesCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsTexture)(name)) {
    return;
  }
  const TextureTarget* binding = FindTextureTarget(target);
  if (binding == nullptr) {
    LOG_WARNING << "Subcapture: texture " << key << " has an unsupported target " << target
                << ", its contents are not restored";
    return;
  }

  const GLuint bound = GetName(binding->Binding);
  GL_FN(glBindTexture)(target, name);
  glBindTextureCommand bind;
  bind.m_target.Value = target;
  bind.m_texture.Key = key;
  Emit<glBindTextureCommandSerializer>(bind);

  if (target == GL_TEXTURE_BUFFER) {
    GLint internalFormat = 0;
    GLint buffer = 0;
    GLint offset = 0;
    GLint size = 0;
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, &buffer);
    // A range can start at offset 0; it is a range when it does not cover the whole buffer.
    bool range = false;
    if (buffer != 0 && (m_Version.AtLeast(4, 3) || HasExtension("GL_ARB_texture_buffer_range"))) {
      GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_BUFFER_OFFSET, &offset);
      GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_BUFFER_SIZE, &size);
      const GLuint boundBuffer = GetName(GL_COPY_READ_BUFFER_BINDING);
      GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, static_cast<GLuint>(buffer));
      GLint64 bufferSize = 0;
      GL_FN(glGetBufferParameteri64v)(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &bufferSize);
      GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, boundBuffer);
      range = offset != 0 || size != bufferSize;
    }
    if (range) {
      glTexBufferRangeCommand command;
      command.m_target.Value = target;
      command.m_internalformat.Value = static_cast<GLenum>(internalFormat);
      command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, static_cast<GLuint>(buffer));
      command.m_offset.Value = offset;
      command.m_size.Value = size;
      Emit<glTexBufferRangeCommandSerializer>(command);
    } else if (buffer != 0) {
      glTexBufferCommand command;
      command.m_target.Value = target;
      command.m_internalformat.Value = static_cast<GLenum>(internalFormat);
      command.m_buffer.Key = KeyOf(GLObjectKind::Buffer, static_cast<GLuint>(buffer));
      Emit<glTexBufferCommandSerializer>(command);
    }
    GL_FN(glBindTexture)(target, bound);
    return;
  }

  GLint immutable = GL_FALSE;
  if (m_Version.AtLeast(4, 2) || HasExtension("GL_ARB_texture_storage")) {
    GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
  }

  if (target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
    const TextureLevel level = QueryLevel(target, 0);
    GLint samples = 0;
    GLint fixedLocations = GL_TRUE;
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_SAMPLES, &samples);
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, &fixedLocations);
    GL_FN(glBindTexture)(target, bound);
    if (level.Width == 0) {
      return;
    }
    const auto format = static_cast<GLenum>(level.InternalFormat);
    const auto fixed = static_cast<GLboolean>(fixedLocations);
    if (target == GL_TEXTURE_2D_MULTISAMPLE && immutable) {
      glTexStorage2DMultisampleCommand command;
      command.m_target.Value = target;
      command.m_samples.Value = samples;
      command.m_internalformat.Value = format;
      command.m_width.Value = level.Width;
      command.m_height.Value = level.Height;
      command.m_fixedsamplelocations.Value = fixed;
      Emit<glTexStorage2DMultisampleCommandSerializer>(command);
    } else if (target == GL_TEXTURE_2D_MULTISAMPLE) {
      glTexImage2DMultisampleCommand command;
      command.m_target.Value = target;
      command.m_samples.Value = samples;
      command.m_internalformat.Value = format;
      command.m_width.Value = level.Width;
      command.m_height.Value = level.Height;
      command.m_fixedsamplelocations.Value = fixed;
      Emit<glTexImage2DMultisampleCommandSerializer>(command);
    } else if (immutable) {
      glTexStorage3DMultisampleCommand command;
      command.m_target.Value = target;
      command.m_samples.Value = samples;
      command.m_internalformat.Value = format;
      command.m_width.Value = level.Width;
      command.m_height.Value = level.Height;
      command.m_depth.Value = level.Depth;
      command.m_fixedsamplelocations.Value = fixed;
      Emit<glTexStorage3DMultisampleCommandSerializer>(command);
    } else {
      glTexImage3DMultisampleCommand command;
      command.m_target.Value = target;
      command.m_samples.Value = samples;
      command.m_internalformat.Value = format;
      command.m_width.Value = level.Width;
      command.m_height.Value = level.Height;
      command.m_depth.Value = level.Depth;
      command.m_fixedsamplelocations.Value = fixed;
      Emit<glTexImage3DMultisampleCommandSerializer>(command);
    }
    return;
  }

  const int dimensions = UploadDimensions(target);
  const bool cubeMap = target == GL_TEXTURE_CUBE_MAP;
  const GLenum firstImageTarget = cubeMap ? CUBE_MAP_FACES[0] : target;

  if (immutable) {
    GLint levels = 0;
    if (m_Version.AtLeast(4, 3) || HasExtension("GL_ARB_texture_view")) {
      GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_IMMUTABLE_LEVELS, &levels);
    } else {
      // GL reports the level count from 4.3; levels past the storage have no image.
      const GLint maxLevels = MaxLevels(target);
      while (levels < maxLevels && QueryLevel(firstImageTarget, levels).Width != 0) {
        ++levels;
      }
    }
    const TextureLevel base = QueryLevel(firstImageTarget, 0);
    const auto format = static_cast<GLenum>(base.InternalFormat);
    if (dimensions == 1) {
      glTexStorage1DCommand command;
      command.m_target.Value = target;
      command.m_levels.Value = levels;
      command.m_internalformat.Value = format;
      command.m_width.Value = base.Width;
      Emit<glTexStorage1DCommandSerializer>(command);
    } else if (dimensions == 2) {
      glTexStorage2DCommand command;
      command.m_target.Value = target;
      command.m_levels.Value = levels;
      command.m_internalformat.Value = format;
      command.m_width.Value = base.Width;
      command.m_height.Value = base.Height;
      Emit<glTexStorage2DCommandSerializer>(command);
    } else {
      glTexStorage3DCommand command;
      command.m_target.Value = target;
      command.m_levels.Value = levels;
      command.m_internalformat.Value = format;
      command.m_width.Value = base.Width;
      command.m_height.Value = base.Height;
      command.m_depth.Value = base.Depth;
      Emit<glTexStorage3DCommandSerializer>(command);
    }
  }

  const bool subImageReadback =
      m_Version.AtLeast(4, 5) || HasExtension("GL_ARB_get_texture_sub_image");
  const GLint levels = MaxLevels(target);
  for (GLint level = 0; level < levels; ++level) {
    for (size_t face = 0; face < (cubeMap ? std::size(CUBE_MAP_FACES) : 1); ++face) {
      const GLenum imageTarget = cubeMap ? CUBE_MAP_FACES[face] : target;
      const TextureLevel info = QueryLevel(imageTarget, level);
      if (info.Width == 0) {
        continue;
      }
      const auto width = static_cast<size_t>(info.Width);
      const auto height = static_cast<size_t>(std::max(info.Height, 1));
      const auto depth = static_cast<size_t>(std::max(info.Depth, 1));
      // A level of several gigabytes cannot be read or uploaded in one call, so 3D and array
      // levels move in slabs of whole slices, or in bands of whole rows of a larger slice.
      if (dimensions == 3 && !info.Compressed && subImageReadback) {
        const PixelFormat pixelFormat = ReadbackFormat(imageTarget, level, m_Version.Compatibility);
        const size_t rowSize = width * pixelFormat.PixelSize;
        const size_t sliceSize = rowSize * height;
        const auto slab = static_cast<GLint>(std::max<size_t>(1, MAX_SLAB_SIZE / sliceSize));
        const auto band =
            static_cast<GLint>(std::clamp<size_t>(MAX_SLAB_SIZE / rowSize, 1, height));
        if (!immutable) {
          glTexImage3DCommand command;
          command.m_target.Value = imageTarget;
          command.m_level.Value = level;
          command.m_internalformat.Value = info.InternalFormat;
          command.m_width.Value = info.Width;
          command.m_height.Value = info.Height;
          command.m_depth.Value = info.Depth;
          command.m_format.Value = pixelFormat.Format;
          command.m_type.Value = pixelFormat.Type;
          Emit<glTexImage3DCommandSerializer>(command);
        }
        for (GLint z = 0; z < info.Depth; z += slab) {
          for (GLint y = 0; y < info.Height; y += band) {
            glTexSubImage3DCommand command;
            command.m_target.Value = imageTarget;
            command.m_level.Value = level;
            command.m_yoffset.Value = y;
            command.m_zoffset.Value = z;
            command.m_width.Value = info.Width;
            command.m_height.Value = std::min(band, info.Height - y);
            command.m_depth.Value = std::min(slab, info.Depth - z);
            command.m_format.Value = pixelFormat.Format;
            command.m_type.Value = pixelFormat.Type;
            command.m_pixels.Data.resize(rowSize * command.m_height.Value * command.m_depth.Value);
            command.m_pixels.HasValue = true;
            GL_FN(glGetTextureSubImage)
            (name, level, 0, y, z, info.Width, command.m_height.Value, command.m_depth.Value,
             pixelFormat.Format, pixelFormat.Type,
             static_cast<GLsizei>(command.m_pixels.Data.size()), command.m_pixels.Data.data());
            Emit<glTexSubImage3DCommandSerializer>(command);
          }
        }
        continue;
      }
      std::vector<uint8_t> data;
      PixelFormat pixelFormat{};
      if (info.Compressed) {
        data.resize(static_cast<size_t>(std::max(info.CompressedSize, 0)));
        const bool copy = m_Version.AtLeast(4, 3) ||
                          (m_Version.AtLeast(3, 0) && HasExtension("GL_ARB_copy_image") &&
                           HasExtension("GL_ARB_texture_storage") &&
                           HasExtension("GL_ARB_internalformat_query2"));
        ReadCompressedImage(target, name, imageTarget, level, info, data, copy);
      } else {
        pixelFormat = ReadbackFormat(imageTarget, level, m_Version.Compatibility);
        data.resize(width * height * depth * pixelFormat.PixelSize);
        GL_FN(glGetTexImage)(imageTarget, level, pixelFormat.Format, pixelFormat.Type, data.data());
      }
      const auto internalFormat = static_cast<GLenum>(info.InternalFormat);
      const auto imageSize = static_cast<GLsizei>(data.size());
      ArrayArgument<uint8_t> pixels;
      pixels.Data = std::move(data);
      pixels.HasValue = true;

      if (dimensions == 1 && info.Compressed && immutable) {
        glCompressedTexSubImage1DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_format.Value = internalFormat;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexSubImage1DCommandSerializer>(command);
      } else if (dimensions == 1 && info.Compressed) {
        glCompressedTexImage1DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = internalFormat;
        command.m_width.Value = info.Width;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexImage1DCommandSerializer>(command);
      } else if (dimensions == 1 && immutable) {
        glTexSubImage1DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexSubImage1DCommandSerializer>(command);
      } else if (dimensions == 1) {
        glTexImage1DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = info.InternalFormat;
        command.m_width.Value = info.Width;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexImage1DCommandSerializer>(command);
      } else if (dimensions == 2 && info.Compressed && immutable) {
        glCompressedTexSubImage2DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_format.Value = internalFormat;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexSubImage2DCommandSerializer>(command);
      } else if (dimensions == 2 && info.Compressed) {
        glCompressedTexImage2DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = internalFormat;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexImage2DCommandSerializer>(command);
      } else if (dimensions == 2 && immutable) {
        glTexSubImage2DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexSubImage2DCommandSerializer>(command);
      } else if (dimensions == 2) {
        glTexImage2DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = info.InternalFormat;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexImage2DCommandSerializer>(command);
      } else if (info.Compressed && immutable) {
        glCompressedTexSubImage3DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_depth.Value = info.Depth;
        command.m_format.Value = internalFormat;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexSubImage3DCommandSerializer>(command);
      } else if (info.Compressed) {
        glCompressedTexImage3DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = internalFormat;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_depth.Value = info.Depth;
        command.m_imageSize.Value = imageSize;
        command.m_data = std::move(pixels);
        Emit<glCompressedTexImage3DCommandSerializer>(command);
      } else if (immutable) {
        glTexSubImage3DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_depth.Value = info.Depth;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexSubImage3DCommandSerializer>(command);
      } else {
        glTexImage3DCommand command;
        command.m_target.Value = imageTarget;
        command.m_level.Value = level;
        command.m_internalformat.Value = info.InternalFormat;
        command.m_width.Value = info.Width;
        command.m_height.Value = info.Height;
        command.m_depth.Value = info.Depth;
        command.m_format.Value = pixelFormat.Format;
        command.m_type.Value = pixelFormat.Type;
        command.m_pixels = std::move(pixels);
        Emit<glTexImage3DCommandSerializer>(command);
      }
    }
  }

  RestoreTextureParameters(key, target);
  GL_FN(glBindTexture)(target, bound);
}

// A view shares the storage of its original texture, so it is made again by its glTextureView
// after the original, before it is bound, and gets only its own parameters. A deleted original
// or view stays tracked while a view, container or resident handle keeps it, so views of one
// storage share it again. A view whose original is not tracked is restored as a texture of its
// own.
void StateRestorer::RestoreTextureView(GITSKey key, std::set<GITSKey>& restored) {
  if (!restored.insert(key).second) {
    return;
  }
  const ObjectState& object = *m_State.Object(key);
  const GLuint name =
      object.Snapshot.Storage != 0 ? object.Snapshot.Storage : NameOf(GLObjectKind::Texture, key);
  const ObjectState* original = m_State.Object(object.ViewOf);
  if (original == nullptr) {
    RestoreTexture(key, name, object.Target);
    return;
  }
  if (original->View != nullptr) {
    RestoreTextureView(object.ViewOf, restored);
  }

  glGenTexturesCommand gen;
  gen.m_n.Value = 1;
  gen.m_textures.Key = {key};
  gen.m_textures.Value = {0};
  gen.m_textures.HasValue = true;
  Emit<glGenTexturesCommandSerializer>(gen);
  const TextureTarget* binding = FindTextureTarget(object.Target);
  if (!Required(key) || name == 0 || !GL_FN(glIsTexture)(name) || binding == nullptr) {
    return;
  }
  object.View->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);

  const GLuint bound = GetName(binding->Binding);
  GL_FN(glBindTexture)(object.Target, name);
  glBindTextureCommand bind;
  bind.m_target.Value = object.Target;
  bind.m_texture.Key = key;
  Emit<glBindTextureCommandSerializer>(bind);
  RestoreTextureParameters(key, object.Target);
  GL_FN(glBindTexture)(object.Target, bound);
}

void StateRestorer::RestoreTextureParameters(GITSKey key, GLenum target) {
  const ObjectState* object = m_State.Object(key);
  if (object == nullptr || object->Snapshot.Storage == 0) {
    RestoreTextureParameters(target);
    return;
  }
  for (const auto& command : object->Snapshot.Parameters) {
    command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
  }
}

void StateRestorer::RestoreTextureParameters(GLenum target) {
  // Multisample textures have no sampler state.
  const bool multisample =
      target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
  for (GLenum pname : TEXTURE_INTEGER_PARAMETERS) {
    if (target == GL_TEXTURE_RECTANGLE &&
        (pname == GL_TEXTURE_BASE_LEVEL || pname == GL_TEXTURE_MAX_LEVEL)) {
      continue;
    }
    if (multisample && std::ranges::find(SAMPLER_INTEGER_PARAMETERS, pname) !=
                           std::end(SAMPLER_INTEGER_PARAMETERS)) {
      continue;
    }
    glTexParameteriCommand command;
    command.m_target.Value = target;
    command.m_pname.Value = pname;
    GL_FN(glGetTexParameteriv)(target, pname, &command.m_param.Value);
    Emit<glTexParameteriCommandSerializer>(command);
  }
  if (m_Version.AtLeast(4, 3)) {
    glTexParameteriCommand command;
    command.m_target.Value = target;
    command.m_pname.Value = GL_DEPTH_STENCIL_TEXTURE_MODE;
    GL_FN(glGetTexParameteriv)(target, GL_DEPTH_STENCIL_TEXTURE_MODE, &command.m_param.Value);
    Emit<glTexParameteriCommandSerializer>(command);
  }
  if (multisample) {
    return;
  }
  if (HasExtension("GL_EXT_texture_sRGB_decode")) {
    glTexParameteriCommand command;
    command.m_target.Value = target;
    command.m_pname.Value = GL_TEXTURE_SRGB_DECODE_EXT;
    GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_SRGB_DECODE_EXT, &command.m_param.Value);
    Emit<glTexParameteriCommandSerializer>(command);
  }
  std::vector<GLenum> floatParameters(std::begin(FLOAT_PARAMETERS), std::end(FLOAT_PARAMETERS));
  if (m_Version.AtLeast(4, 6) || HasExtension("GL_EXT_texture_filter_anisotropic") ||
      HasExtension("GL_ARB_texture_filter_anisotropic")) {
    floatParameters.push_back(GL_TEXTURE_MAX_ANISOTROPY);
  }
  for (GLenum pname : floatParameters) {
    glTexParameterfCommand command;
    command.m_target.Value = target;
    command.m_pname.Value = pname;
    GL_FN(glGetTexParameterfv)(target, pname, &command.m_param.Value);
    Emit<glTexParameterfCommandSerializer>(command);
  }
  glTexParameterfvCommand border;
  border.m_target.Value = target;
  border.m_pname.Value = GL_TEXTURE_BORDER_COLOR;
  border.m_params.Data.resize(4);
  border.m_params.HasValue = true;
  GL_FN(glGetTexParameterfv)(target, GL_TEXTURE_BORDER_COLOR, border.m_params.Data.data());
  Emit<glTexParameterfvCommandSerializer>(border);
}

void StateRestorer::RestoreRenderbuffer(GITSKey key, GLuint name) {
  glGenRenderbuffersCommand gen;
  gen.m_n.Value = 1;
  gen.m_renderbuffers.Key = {key};
  gen.m_renderbuffers.Value = {0};
  gen.m_renderbuffers.HasValue = true;
  Emit<glGenRenderbuffersCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsRenderbuffer)(name)) {
    return;
  }

  const GLuint bound = GetName(GL_RENDERBUFFER_BINDING);
  GL_FN(glBindRenderbuffer)(GL_RENDERBUFFER, name);
  GLint width = 0;
  GLint height = 0;
  GLint internalFormat = 0;
  GLint samples = 0;
  GLint depthSize = 0;
  GLint stencilSize = 0;
  GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
  GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
  GL_FN(glGetRenderbufferParameteriv)
  (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &internalFormat);
  GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
  GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &depthSize);
  GL_FN(glGetRenderbufferParameteriv)(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &stencilSize);
  GL_FN(glBindRenderbuffer)(GL_RENDERBUFFER, bound);

  glBindRenderbufferCommand bind;
  bind.m_target.Value = GL_RENDERBUFFER;
  bind.m_renderbuffer.Key = key;
  Emit<glBindRenderbufferCommandSerializer>(bind);
  if (width == 0 || height == 0) {
    return;
  }
  if (samples > 0) {
    glRenderbufferStorageMultisampleCommand command;
    command.m_target.Value = GL_RENDERBUFFER;
    command.m_samples.Value = samples;
    command.m_internalformat.Value = static_cast<GLenum>(internalFormat);
    command.m_width.Value = width;
    command.m_height.Value = height;
    Emit<glRenderbufferStorageMultisampleCommandSerializer>(command);
  } else {
    glRenderbufferStorageCommand command;
    command.m_target.Value = GL_RENDERBUFFER;
    command.m_internalformat.Value = static_cast<GLenum>(internalFormat);
    command.m_width.Value = width;
    command.m_height.Value = height;
    Emit<glRenderbufferStorageCommandSerializer>(command);
    // Multisampled contents and stencil-only renderbuffers, which no texture format matches
    // before GL 4.4, keep the storage only.
    if (m_Version.AtLeast(3, 0) && (depthSize > 0 || stencilSize == 0)) {
      RestoreRenderbufferContents(key, name, width, height, static_cast<GLenum>(internalFormat),
                                  depthSize, stencilSize);
    }
  }
}

PixelFormat StateRestorer::AttachmentReadbackFormat(GLenum attachment) {
  const auto attachmentParameter = [](GLenum attachmentPoint, GLenum pname) {
    GLint value = 0;
    GL_FN(glGetFramebufferAttachmentParameteriv)
    (GL_READ_FRAMEBUFFER, attachmentPoint, pname, &value);
    return value;
  };
  if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
    return attachmentParameter(GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE) ==
                   GL_FLOAT
               ? PixelFormat{GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8}
               : PixelFormat{GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 4};
  }
  if (attachment == GL_DEPTH_ATTACHMENT) {
    return {GL_DEPTH_COMPONENT, GL_FLOAT, 4};
  }
  const GLint componentType =
      attachmentParameter(attachment, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE);
  GLint bits = 0;
  for (GLenum pname : {GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
                       GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE}) {
    bits = std::max(bits, attachmentParameter(attachment, pname));
  }
  if (componentType == GL_INT) {
    return {GL_RGBA_INTEGER, GL_INT, 16};
  }
  if (componentType == GL_UNSIGNED_INT) {
    return {GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16};
  }
  if (componentType == GL_UNSIGNED_NORMALIZED && bits <= 8) {
    return {GL_RGBA, GL_UNSIGNED_BYTE, 4};
  }
  return {GL_RGBA, GL_FLOAT, 16};
}

// Renderbuffers have no image upload, so the contents are read back through a framebuffer and
// written through a texture blitted into the renderbuffer.
void StateRestorer::RestoreRenderbufferContents(GITSKey key,
                                                GLuint name,
                                                GLsizei width,
                                                GLsizei height,
                                                GLenum internalFormat,
                                                GLint depthSize,
                                                GLint stencilSize) {
  const GLenum attachment = depthSize > 0 && stencilSize > 0 ? GL_DEPTH_STENCIL_ATTACHMENT
                            : depthSize > 0                  ? GL_DEPTH_ATTACHMENT
                                                             : GL_COLOR_ATTACHMENT0;
  const GLuint readBound = GetName(GL_READ_FRAMEBUFFER_BINDING);
  GLuint framebuffer = 0;
  GL_FN(glGenFramebuffers)(1, &framebuffer);
  GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, framebuffer);
  GL_FN(glFramebufferRenderbuffer)(GL_READ_FRAMEBUFFER, attachment, GL_RENDERBUFFER, name);
  const PixelFormat format = AttachmentReadbackFormat(attachment);
  GL_FN(glReadBuffer)(attachment == GL_COLOR_ATTACHMENT0 ? attachment : GL_NONE);
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) *
                              format.PixelSize);
  GL_FN(glReadPixels)(0, 0, width, height, format.Format, format.Type, pixels.data());
  GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, readBound);
  GL_FN(glDeleteFramebuffers)(1, &framebuffer);

  const GITSKey target = m_Recorder.CreateStateRestoreObjectKey();
  glGenFramebuffersCommand gen;
  gen.m_n.Value = 1;
  gen.m_framebuffers.Key = {target};
  gen.m_framebuffers.Value = {0};
  gen.m_framebuffers.HasValue = true;
  Emit<glGenFramebuffersCommandSerializer>(gen);
  glBindFramebufferCommand bind;
  bind.m_target.Value = GL_DRAW_FRAMEBUFFER;
  bind.m_framebuffer.Key = target;
  Emit<glBindFramebufferCommandSerializer>(bind);
  glFramebufferRenderbufferCommand attach;
  attach.m_target.Value = GL_DRAW_FRAMEBUFFER;
  attach.m_attachment.Value = attachment;
  attach.m_renderbuffertarget.Value = GL_RENDERBUFFER;
  attach.m_renderbuffer.Key = key;
  Emit<glFramebufferRenderbufferCommandSerializer>(attach);
  EmitBlit(attachment, internalFormat, width, height, format, std::move(pixels));
  glDeleteFramebuffersCommand remove;
  remove.m_n.Value = 1;
  remove.m_framebuffers.Key = {target};
  remove.m_framebuffers.Value = {0};
  remove.m_framebuffers.HasValue = true;
  Emit<glDeleteFramebuffersCommandSerializer>(remove);
}

void StateRestorer::RestoreSampler(GITSKey key, GLuint name) {
  glGenSamplersCommand gen;
  gen.m_count.Value = 1;
  gen.m_samplers.Key = {key};
  gen.m_samplers.Value = {0};
  gen.m_samplers.HasValue = true;
  Emit<glGenSamplersCommandSerializer>(gen);
  if (!Required(key) || name == 0 || !GL_FN(glIsSampler)(name)) {
    return;
  }

  for (GLenum pname : SAMPLER_INTEGER_PARAMETERS) {
    glSamplerParameteriCommand command;
    command.m_sampler.Key = key;
    command.m_pname.Value = pname;
    GL_FN(glGetSamplerParameteriv)(name, pname, &command.m_param.Value);
    Emit<glSamplerParameteriCommandSerializer>(command);
  }
  if (HasExtension("GL_EXT_texture_sRGB_decode")) {
    glSamplerParameteriCommand command;
    command.m_sampler.Key = key;
    command.m_pname.Value = GL_TEXTURE_SRGB_DECODE_EXT;
    GL_FN(glGetSamplerParameteriv)(name, GL_TEXTURE_SRGB_DECODE_EXT, &command.m_param.Value);
    Emit<glSamplerParameteriCommandSerializer>(command);
  }
  std::vector<GLenum> floatParameters(std::begin(FLOAT_PARAMETERS), std::end(FLOAT_PARAMETERS));
  if (m_Version.AtLeast(4, 6) || HasExtension("GL_EXT_texture_filter_anisotropic") ||
      HasExtension("GL_ARB_texture_filter_anisotropic")) {
    floatParameters.push_back(GL_TEXTURE_MAX_ANISOTROPY);
  }
  for (GLenum pname : floatParameters) {
    glSamplerParameterfCommand command;
    command.m_sampler.Key = key;
    command.m_pname.Value = pname;
    GL_FN(glGetSamplerParameterfv)(name, pname, &command.m_param.Value);
    Emit<glSamplerParameterfCommandSerializer>(command);
  }
  glSamplerParameterfvCommand border;
  border.m_sampler.Key = key;
  border.m_pname.Value = GL_TEXTURE_BORDER_COLOR;
  border.m_param.Data.resize(4);
  border.m_param.HasValue = true;
  GL_FN(glGetSamplerParameterfv)(name, GL_TEXTURE_BORDER_COLOR, border.m_param.Data.data());
  Emit<glSamplerParameterfvCommandSerializer>(border);
}

} // namespace gits::opengl
