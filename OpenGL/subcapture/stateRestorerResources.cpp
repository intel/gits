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
#include "subcaptureSession.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
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

// Stops the subcapture when a compressed image cannot be read exactly, since writing it from
// another read could change its blocks.
void FailCompressedRead(GLuint texture, GLint level, const char* step, GLenum error) {
  std::ostringstream message;
  message << "Subcapture: " << step << " to read level " << level << " of compressed texture "
          << texture << " raised GL error 0x" << std::hex << error << std::dec << ".";
  if (error == GL_OUT_OF_MEMORY) {
    message << " The replay system ran out of GPU memory; record the subcapture on a system with "
               "more GPU memory.";
  } else {
    message << " Its compressed blocks cannot be read exactly on this system.";
  }
  GITS_ASSERT(false, message.str());
}

// Stops the subcapture when the samples of the multisample object `key` cannot be restored, since
// its restored storage would have undefined contents.
void FailSamples(GITSKey key, const std::string& reason) {
  std::ostringstream message;
  message << "Subcapture: the samples of multisample object " << key
          << " cannot be restored: " << reason;
  GITS_ASSERT(false, message.str());
}

void FailSampleRead(GITSKey key, const char* step, GLenum error) {
  std::ostringstream reason;
  reason << step << " to read them raised GL error 0x" << std::hex << error << std::dec << ".";
  if (error == GL_OUT_OF_MEMORY) {
    reason << " The replay system ran out of GPU memory; record the subcapture on a system with "
              "more GPU memory.";
  } else {
    reason << " They cannot be read on this system.";
  }
  FailSamples(key, reason.str());
}

// Clears the GL error flags, returning how many errors they held, so that the error of a call
// checked next is its own.
int TakeErrors() {
  int errors = 0;
  while (errors < 64 && GL_FN(glGetError)() != GL_NO_ERROR) {
    ++errors;
  }
  return errors;
}

// The compressed formats glCopyImageSubData can copy to and from uncompressed ones: those of its
// copy compatibility classes, with the S3TC formats of GL_EXT_texture_compression_s3tc and
// GL_EXT_texture_sRGB.
bool Copyable(GLenum format) {
  switch (format) {
  case GL_COMPRESSED_RED_RGTC1:
  case GL_COMPRESSED_SIGNED_RED_RGTC1:
  case GL_COMPRESSED_RG_RGTC2:
  case GL_COMPRESSED_SIGNED_RG_RGTC2:
  case GL_COMPRESSED_RGBA_BPTC_UNORM:
  case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
  case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
  case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
  case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
  case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
    return true;
  default:
    return false;
  }
}

// Compressed images read with glGetCompressedTexImage can differ from the uploaded data. Their
// blocks copied into the texels of an uncompressed texture of the same size read back exactly.
// The copy covers the whole image, so it can end inside the last blocks, as for levels smaller
// than a block. Images in formats glCopyImageSubData cannot copy are read directly. A failed
// allocation or copy stops the subcapture. Returns the number of GL errors raised before the
// copy, which the copy's own errors must be told apart from.
int ReadCompressedImage(GLenum target,
                        GLuint texture,
                        GLenum imageTarget,
                        GLint level,
                        const TextureLevel& info,
                        std::vector<uint8_t>& data,
                        bool copy,
                        bool handles) {
  GLint blockWidth = 0;
  GLint blockHeight = 0;
  if (copy && Copyable(static_cast<GLenum>(info.InternalFormat))) {
    const auto format = static_cast<GLenum>(info.InternalFormat);
    GL_FN(glGetInternalformativ)
    (GL_TEXTURE_2D, format, GL_TEXTURE_COMPRESSED_BLOCK_WIDTH, 1, &blockWidth);
    GL_FN(glGetInternalformativ)
    (GL_TEXTURE_2D, format, GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT, 1, &blockHeight);
  }
  const GLsizei width = info.Width;
  const GLsizei height = std::max(info.Height, 1);
  const GLsizei layers = std::max(info.Depth, 1);
  const bool blocked = blockWidth > 0 && blockHeight > 0;
  const GLsizei columns = blocked ? (width + blockWidth - 1) / blockWidth : 0;
  const GLsizei rows = blocked ? (height + blockHeight - 1) / blockHeight : 0;
  // The block size follows from the image size.
  const size_t blocks = static_cast<size_t>(columns) * rows * layers;
  const size_t blockSize = blocks > 0 && data.size() % blocks == 0 ? data.size() / blocks : 0;
  if (blockSize != 8 && blockSize != 16) {
    GL_FN(glGetCompressedTexImage)(imageTarget, level, data.data());
    return 0;
  }
  const bool wide = blockSize == 16;
  const GLint face =
      target == GL_TEXTURE_CUBE_MAP ? static_cast<GLint>(imageTarget - CUBE_MAP_FACES[0]) : 0;
  // A 3D texture can be deeper than an array texture can have layers, so the layers are copied
  // in parts.
  const GLsizei maxLayers = std::max(GetInteger(GL_MAX_ARRAY_TEXTURE_LAYERS), 1);
  const size_t layerSize = static_cast<size_t>(columns) * rows * blockSize;
  const GLuint bound = GetName(GL_TEXTURE_BINDING_2D_ARRAY);
  const int errors = TakeErrors();
  // glCopyImageSubData requires a complete texture, so only the level read is in use meanwhile.
  // A texture with a handle was complete when the handle was created, and GL rejects changing
  // its parameters or defining its images again since, so it is copied as it is.
  GLint baseLevel = 0;
  GLint maxLevel = 0;
  if (!handles) {
    GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_BASE_LEVEL, &baseLevel);
    GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_MAX_LEVEL, &maxLevel);
    GL_FN(glTexParameteri)(target, GL_TEXTURE_BASE_LEVEL, level);
    GL_FN(glTexParameteri)(target, GL_TEXTURE_MAX_LEVEL, level);
  }
  for (GLsizei first = 0; first < layers; first += maxLayers) {
    const GLsizei count = std::min(maxLayers, layers - first);
    GLuint uncompressed = 0;
    GL_FN(glGenTextures)(1, &uncompressed);
    GL_FN(glBindTexture)(GL_TEXTURE_2D_ARRAY, uncompressed);
    // Integer textures are complete only with nearest filtering.
    GL_FN(glTexParameteri)(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL_FN(glTexParameteri)(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL_FN(glTexStorage3D)
    (GL_TEXTURE_2D_ARRAY, 1, wide ? GL_RGBA32UI : GL_RG32UI, columns, rows, count);
    if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
      FailCompressedRead(texture, level, "allocating a texture", error);
    }
    const auto copyBlocks = [&](GLsizei copyWidth, GLsizei copyHeight) {
      GL_FN(glCopyImageSubData)
      (texture, target, level, 0, 0, face + first, uncompressed, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
       copyWidth, copyHeight, count);
      return GL_FN(glGetError)();
    };
    // The region can end inside the last blocks at the image edge or be rounded up to whole
    // blocks; both copy the same blocks, so the rounded one is copied when the first is rejected.
    GLenum error = copyBlocks(width, height);
    if (error == GL_INVALID_VALUE && (width % blockWidth != 0 || height % blockHeight != 0)) {
      error = copyBlocks(columns * blockWidth, rows * blockHeight);
    }
    if (error != GL_NO_ERROR) {
      FailCompressedRead(texture, level, "copying the blocks", error);
    }
    GL_FN(glGetTexImage)
    (GL_TEXTURE_2D_ARRAY, 0, wide ? GL_RGBA_INTEGER : GL_RG_INTEGER, GL_UNSIGNED_INT,
     data.data() + first * layerSize);
    GL_FN(glDeleteTextures)(1, &uncompressed);
  }
  GL_FN(glBindTexture)(GL_TEXTURE_2D_ARRAY, bound);
  if (!handles) {
    GL_FN(glTexParameteri)(target, GL_TEXTURE_BASE_LEVEL, baseLevel);
    GL_FN(glTexParameteri)(target, GL_TEXTURE_MAX_LEVEL, maxLevel);
  }
  return errors;
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
  m_Context = owner.second;
  if (!m_State.HasResidentHandle(key) && !(kind == GLObjectKind::Texture && m_State.Viewed(key)) &&
      !(kind == GLObjectKind::Buffer && m_State.ListReferences(owner.first, key)) &&
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
  ReadExtensions();
  std::erase_if(detached, [&](GITSKey key) {
    const ObjectState& object = *m_State.Object(key);
    return m_State.HasResidentHandle(key) || m_State.Viewed(key) ||
           (object.Kind == GLObjectKind::Buffer && m_State.ListReferences(shareGroup, key)) ||
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
  // Imported before the buffers and textures that have storage in them.
  restore(GLObjectKind::MemoryObject, [this](GITSKey key, GLuint, const ObjectState& object) {
    glCreateMemoryObjectsEXTCommand command;
    command.m_n.Value = 1;
    command.m_memoryObjects.Key = {key};
    command.m_memoryObjects.Value = {0};
    command.m_memoryObjects.HasValue = true;
    Emit<glCreateMemoryObjectsEXTCommandSerializer>(command);
    EmitSetup(object);
  });
  restore(GLObjectKind::Semaphore, [this](GITSKey key, GLuint, const ObjectState& object) {
    if (object.CreatedByNv) {
      glCreateSemaphoresNVCommand command;
      command.m_n.Value = 1;
      command.m_semaphores.Key = {key};
      command.m_semaphores.Value = {0};
      command.m_semaphores.HasValue = true;
      Emit<glCreateSemaphoresNVCommandSerializer>(command);
    } else {
      glGenSemaphoresEXTCommand command;
      command.m_n.Value = 1;
      command.m_semaphores.Key = {key};
      command.m_semaphores.Value = {0};
      command.m_semaphores.HasValue = true;
      Emit<glGenSemaphoresEXTCommandSerializer>(command);
    }
    EmitSetup(object);
  });
  restore(GLObjectKind::Buffer,
          [this](GITSKey key, GLuint name, const ObjectState&) { RestoreBuffer(key, name); });
  restore(GLObjectKind::Texture, [this](GITSKey key, GLuint name, const ObjectState& object) {
    if (object.View == nullptr) {
      RestoreTexture(key, object.Snapshot.Storage != 0 ? object.Snapshot.Storage : name,
                     object.Target);
    }
  });
  // A memory object the application deleted after giving storage in it.
  restore(GLObjectKind::MemoryObject, [this](GITSKey key, GLuint, const ObjectState& object) {
    if (object.DeletePending) {
      glDeleteMemoryObjectsEXTCommand command;
      command.m_n.Value = 1;
      command.m_memoryObjects.Key = {key};
      command.m_memoryObjects.Value = {0};
      command.m_memoryObjects.HasValue = true;
      Emit<glDeleteMemoryObjectsEXTCommandSerializer>(command);
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

void StateRestorer::EmitSetup(const ObjectState& object) {
  for (const auto& command : object.Setup) {
    command->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
  }
}

void StateRestorer::RestoreBuffer(GITSKey key, GLuint name) {
  glGenBuffersCommand gen;
  gen.m_n.Value = 1;
  gen.m_buffers.Key = {key};
  gen.m_buffers.Value = {0};
  gen.m_buffers.HasValue = true;
  Emit<glGenBuffersCommandSerializer>(gen);
  // Storage in a memory object is given whether or not the range uses the buffer, as it is
  // shared with the other API of the stream, which restores its contents.
  if (const ObjectState* object = m_State.Object(key);
      object != nullptr && object->MemoryStorage != nullptr) {
    glBindBufferCommand bind;
    bind.m_target.Value = GL_ARRAY_BUFFER;
    bind.m_buffer.Key = key;
    Emit<glBindBufferCommandSerializer>(bind);
    EmitStored(object->MemoryStorage);
    if (m_Capture == nullptr && name != 0 && GL_FN(glIsBuffer)(name)) {
      RestoreUnsharedContents(key, name, *object);
    }
    return;
  }
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

void StateRestorer::RestoreUnsharedContents(GITSKey key, GLuint name, const ObjectState& object) {
  const ObjectState* memory = m_State.Object(object.StorageMemory);
  const stream::RestoredContents* contents =
      memory != nullptr && memory->ImportedExport != 0
          ? stream::SubcaptureSession::Get().GetRestoredContents(memory->ImportedExport)
          : nullptr;
  if (contents == nullptr) {
    return;
  }
  // Storage in a memory object may be immutable without GL_DYNAMIC_STORAGE_BIT, so the contents
  // are copied into it from a temporary buffer.
  for (const stream::ContentRange& range :
       stream::UnrestoredRanges(*contents, {object.StorageOffset, object.StorageSize})) {
    const auto offset = static_cast<GLintptr>(range.Offset - object.StorageOffset);
    std::vector<uint8_t> data(static_cast<size_t>(range.Size));
    const GLuint bound = GetName(GL_COPY_READ_BUFFER_BINDING);
    GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, name);
    GL_FN(glGetBufferSubData)
    (GL_COPY_READ_BUFFER, offset, static_cast<GLsizeiptr>(data.size()), data.data());
    GL_FN(glBindBuffer)(GL_COPY_READ_BUFFER, bound);

    const GITSKey temporary = m_Recorder.CreateStateRestoreObjectKey();
    glGenBuffersCommand gen;
    gen.m_n.Value = 1;
    gen.m_buffers.Key = {temporary};
    gen.m_buffers.Value = {0};
    gen.m_buffers.HasValue = true;
    Emit<glGenBuffersCommandSerializer>(gen);
    glBindBufferCommand bindRead;
    bindRead.m_target.Value = GL_COPY_READ_BUFFER;
    bindRead.m_buffer.Key = temporary;
    Emit<glBindBufferCommandSerializer>(bindRead);
    glBufferDataCommand bufferData;
    bufferData.m_target.Value = GL_COPY_READ_BUFFER;
    bufferData.m_size.Value = static_cast<GLsizeiptr>(data.size());
    bufferData.m_data.Data = std::move(data);
    bufferData.m_data.HasValue = true;
    bufferData.m_usage.Value = GL_STATIC_DRAW;
    Emit<glBufferDataCommandSerializer>(bufferData);
    glBindBufferCommand bindWrite;
    bindWrite.m_target.Value = GL_COPY_WRITE_BUFFER;
    bindWrite.m_buffer.Key = key;
    Emit<glBindBufferCommandSerializer>(bindWrite);
    glCopyBufferSubDataCommand copy;
    copy.m_readTarget.Value = GL_COPY_READ_BUFFER;
    copy.m_writeTarget.Value = GL_COPY_WRITE_BUFFER;
    copy.m_readOffset.Value = 0;
    copy.m_writeOffset.Value = offset;
    copy.m_size.Value = static_cast<GLsizeiptr>(range.Size);
    Emit<glCopyBufferSubDataCommandSerializer>(copy);
    glDeleteBuffersCommand remove;
    remove.m_n.Value = 1;
    remove.m_buffers.Key = {temporary};
    remove.m_buffers.Value = {0};
    remove.m_buffers.HasValue = true;
    Emit<glDeleteBuffersCommandSerializer>(remove);
  }
}

void StateRestorer::RestoreTexture(GITSKey key, GLuint name, GLenum target) {
  glGenTexturesCommand gen;
  gen.m_n.Value = 1;
  gen.m_textures.Key = {key};
  gen.m_textures.Value = {0};
  gen.m_textures.HasValue = true;
  Emit<glGenTexturesCommandSerializer>(gen);
  const TextureTarget* binding = FindTextureTarget(target);
  // Storage in a memory object is given whether or not the range uses the texture, as it is
  // shared with the other API of the stream, which restores its contents.
  if (const ObjectState* object = m_State.Object(key);
      object != nullptr && object->MemoryStorage != nullptr && binding != nullptr) {
    glBindTextureCommand bind;
    bind.m_target.Value = target;
    bind.m_texture.Key = key;
    Emit<glBindTextureCommandSerializer>(bind);
    // The tiling is read when the storage is given, and must match the image of the other API.
    if (object->Tiling) {
      glTexParameteriCommand parameter;
      parameter.m_target.Value = target;
      parameter.m_pname.Value = GL_TEXTURE_TILING_EXT;
      parameter.m_param.Value = *object->Tiling;
      Emit<glTexParameteriCommandSerializer>(parameter);
    }
    EmitStored(object->MemoryStorage);
    if (name != 0 && GL_FN(glIsTexture)(name)) {
      const GLuint bound = GetName(binding->Binding);
      GL_FN(glBindTexture)(target, name);
      RestoreTextureParameters(key, target);
      GL_FN(glBindTexture)(target, bound);
    }
    return;
  }
  if (!Required(key) || name == 0 || !GL_FN(glIsTexture)(name)) {
    return;
  }
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
    RestoreMultisampleContents(key, target, name);
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
  // A texture with mutable storage whose levels have different formats, one of them compressed,
  // can read back its compressed images wrong without a GL error, decompressed or not, so those
  // are written from their kept uploads.
  bool mixedFormats = false;
  if (!immutable) {
    std::set<GLint> formats;
    bool compressed = false;
    for (GLint level = 0; level < levels; ++level) {
      for (size_t face = 0; face < (cubeMap ? std::size(CUBE_MAP_FACES) : 1); ++face) {
        const TextureLevel info = QueryLevel(cubeMap ? CUBE_MAP_FACES[face] : target, level);
        if (info.Width != 0) {
          formats.insert(info.InternalFormat);
          compressed = compressed || info.Compressed;
        }
      }
    }
    mixedFormats = compressed && formats.size() > 1;
  }
  const ObjectState* object = m_State.Object(key);
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
        const CompressedImage* kept =
            mixedFormats && object != nullptr
                ? m_State.FindCompressedImage(object->Owner, name, imageTarget, level)
                : nullptr;
        if (kept != nullptr && kept->InternalFormat == static_cast<GLenum>(info.InternalFormat) &&
            kept->Width == info.Width && kept->Height == std::max(info.Height, 1) &&
            kept->Depth == std::max(info.Depth, 1) && kept->Data.size() == data.size()) {
          data = kept->Data;
        } else {
          if (mixedFormats) {
            std::ostringstream message;
            message << "Subcapture: cannot restore texture " << key << ", level " << level
                    << " exactly: its levels have different formats, so its compressed image can "
                       "read back wrong, and no matching upload of it is kept.";
            GITS_ASSERT(false, message.str());
          }
          const bool copy = m_Version.AtLeast(4, 3) ||
                            (m_Version.AtLeast(3, 0) && HasExtension("GL_ARB_copy_image") &&
                             HasExtension("GL_ARB_texture_storage") &&
                             HasExtension("GL_ARB_internalformat_query2"));
          m_RaisedErrors += ReadCompressedImage(target, name, imageTarget, level, info, data, copy,
                                                m_State.HandleCreated(key));
        }
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
  if (Supports(4, 3, "GL_ARB_stencil_texturing")) {
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
  // Written after the images, so the restored levels are not generated again.
  if (m_Version.Compatibility && target != GL_TEXTURE_RECTANGLE) {
    glTexParameteriCommand command;
    command.m_target.Value = target;
    command.m_pname.Value = GL_GENERATE_MIPMAP;
    GL_FN(glGetTexParameteriv)(target, GL_GENERATE_MIPMAP, &command.m_param.Value);
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
    RestoreMultisampleContents(key, GL_RENDERBUFFER, name);
  } else {
    glRenderbufferStorageCommand command;
    command.m_target.Value = GL_RENDERBUFFER;
    command.m_internalformat.Value = static_cast<GLenum>(internalFormat);
    command.m_width.Value = width;
    command.m_height.Value = height;
    Emit<glRenderbufferStorageCommandSerializer>(command);
    // Stencil-only renderbuffers, which no texture format matches before GL 4.4, keep the
    // storage only.
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

namespace {

// The images of a multisample texture or renderbuffer drawn separately.
enum class SampleAspect {
  Color,
  Depth,
  Stencil,
};

constexpr const char* COVERING_TRIANGLE_SHADER =
    "#version 150\n"
    "void main() {\n"
    "  gl_Position = vec4(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0, 0.0, "
    "1.0);\n"
    "}\n";

constexpr std::pair<GLenum, GLenum> SAMPLE_TEXTURE_BINDINGS[] = {
    {GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D},
    {GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BINDING_2D_MULTISAMPLE},
    {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY},
};

// The GLSL sampler prefix and output type of a color format of `componentType`.
std::pair<const char*, const char*> ColorShaderTypes(GLint componentType) {
  if (componentType == GL_INT) {
    return {"i", "ivec4"};
  }
  if (componentType == GL_UNSIGNED_INT) {
    return {"u", "uvec4"};
  }
  return {"", "vec4"};
}

// Depth and stencil samples pass through color textures, which hold them exactly.
GLenum SampleTextureFormat(SampleAspect aspect, GLenum internalFormat) {
  if (aspect == SampleAspect::Depth) {
    return GL_R32F;
  }
  return aspect == SampleAspect::Stencil ? GL_R32UI : internalFormat;
}

bool SrgbFormat(GLenum internalFormat) {
  return internalFormat == GL_SRGB || internalFormat == GL_SRGB8 ||
         internalFormat == GL_SRGB_ALPHA || internalFormat == GL_SRGB8_ALPHA8;
}

GLenum SampleAttachment(GLint depthSize, GLint stencilSize) {
  if (depthSize > 0 && stencilSize > 0) {
    return GL_DEPTH_STENCIL_ATTACHMENT;
  }
  if (depthSize > 0) {
    return GL_DEPTH_ATTACHMENT;
  }
  return stencilSize > 0 ? GL_STENCIL_ATTACHMENT : GL_COLOR_ATTACHMENT0;
}

// Copies sample `sampleIndex` of layer `layer` of the multisample texture on unit 0 to the color
// output.
std::string SampleReadShader(SampleAspect aspect, GLint componentType, bool array) {
  const auto [prefix, output] = aspect == SampleAspect::Depth     ? std::pair{"", "float"}
                                : aspect == SampleAspect::Stencil ? std::pair{"u", "uint"}
                                                                  : ColorShaderTypes(componentType);
  std::ostringstream source;
  source << "#version 150\n"
         << "uniform " << prefix << "sampler2DMS" << (array ? "Array" : "") << " source;\n"
         << "uniform int layer;\n"
         << "uniform int sampleIndex;\n"
         << "out " << output << " value;\n"
         << "void main() {\n"
         << "  value = texelFetch(source, "
         << (array ? "ivec3(ivec2(gl_FragCoord.xy), layer)" : "ivec2(gl_FragCoord.xy)")
         << ", sampleIndex)" << (aspect == SampleAspect::Color ? "" : ".r") << ";\n"
         << "}\n";
  return source.str();
}

// Writes the single-sample texture on unit 0: color as the color output, depth as the fragment
// depth, and stencil bit `stencilBit` by discarding the fragments without it.
std::string SampleWriteShader(SampleAspect aspect, GLint componentType, GLint stencilBit) {
  std::ostringstream source;
  source << "#version 150\n";
  if (aspect == SampleAspect::Color) {
    const auto [prefix, output] = ColorShaderTypes(componentType);
    source << "uniform " << prefix << "sampler2D source;\n"
           << "out " << output << " value;\n"
           << "void main() {\n"
           << "  value = texelFetch(source, ivec2(gl_FragCoord.xy), 0);\n"
           << "}\n";
  } else if (aspect == SampleAspect::Depth) {
    source << "uniform sampler2D source;\n"
           << "void main() {\n"
           << "  gl_FragDepth = texelFetch(source, ivec2(gl_FragCoord.xy), 0).r;\n"
           << "}\n";
  } else {
    source << "uniform usampler2D source;\n"
           << "void main() {\n"
           << "  if ((texelFetch(source, ivec2(gl_FragCoord.xy), 0).r & " << (1u << stencilBit)
           << "u) == 0u) {\n"
           << "    discard;\n"
           << "  }\n"
           << "}\n";
  }
  return source.str();
}

// The replay driver state the readback of multisample samples draws with, set for it and put
// back, with its temporary objects deleted, once done, as the replay continues after it.
class SampleReadback {
public:
  SampleReadback(bool compatibility,
                 bool viewportArrays,
                 bool separateShaderObjects,
                 bool pauseTransformFeedback,
                 const std::vector<GLenum>& subroutineStages)
      : m_Compatibility(compatibility),
        m_ViewportArrays(viewportArrays),
        m_PauseTransformFeedback(pauseTransformFeedback) {
    // A program cannot be used while transform feedback is active and not paused.
    if (m_PauseTransformFeedback) {
      GL_FN(glPauseTransformFeedback)();
    }
    m_Program = GetName(GL_CURRENT_PROGRAM);
    m_Pipeline = separateShaderObjects ? GetName(GL_PROGRAM_PIPELINE_BINDING) : 0;
    KeepProgram(separateShaderObjects);
    SaveSubroutines(subroutineStages);
    m_VertexArray = GetName(GL_VERTEX_ARRAY_BINDING);
    m_DrawFramebuffer = GetName(GL_DRAW_FRAMEBUFFER_BINDING);
    m_ReadFramebuffer = GetName(GL_READ_FRAMEBUFFER_BINDING);
    m_UnpackBuffer = GetName(GL_PIXEL_UNPACK_BUFFER_BINDING);
    m_ActiveTexture = GetInteger(GL_ACTIVE_TEXTURE);
    GL_FN(glActiveTexture)(GL_TEXTURE0);
    for (const auto& [target, binding] : SAMPLE_TEXTURE_BINDINGS) {
      m_Textures.emplace_back(target, GetName(binding));
    }
    // Only viewport and scissor rectangle 0 are drawn with.
    if (m_ViewportArrays) {
      GL_FN(glGetFloati_v)(GL_VIEWPORT, 0, m_Viewport);
      m_Scissor = GL_FN(glIsEnabledi)(GL_SCISSOR_TEST, 0) != GL_FALSE;
      GL_FN(glDisablei)(GL_SCISSOR_TEST, 0);
    } else {
      GL_FN(glGetFloatv)(GL_VIEWPORT, m_Viewport);
      m_Scissor = GL_FN(glIsEnabled)(GL_SCISSOR_TEST) != GL_FALSE;
      GL_FN(glDisable)(GL_SCISSOR_TEST);
    }
    m_Blend = GL_FN(glIsEnabledi)(GL_BLEND, 0) != GL_FALSE;
    GL_FN(glDisablei)(GL_BLEND, 0);
    GL_FN(glGetBooleani_v)(GL_COLOR_WRITEMASK, 0, m_ColorMask);
    GL_FN(glColorMaski)(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GL_FN(glGetIntegerv)(GL_POLYGON_MODE, m_PolygonModes);
    GL_FN(glPolygonMode)(GL_FRONT_AND_BACK, GL_FILL);

    std::vector<GLenum> capabilities{GL_CULL_FACE,          GL_DITHER,
                                     GL_COLOR_LOGIC_OP,     GL_POLYGON_SMOOTH,
                                     GL_RASTERIZER_DISCARD, GL_FRAMEBUFFER_SRGB};
    if (m_Compatibility) {
      capabilities.insert(capabilities.end(), {GL_ALPHA_TEST, GL_POLYGON_STIPPLE});
      m_ClampFragmentColor = GetInteger(GL_CLAMP_FRAGMENT_COLOR);
      GL_FN(glClampColor)(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    }
    // The covering triangle writes no clip distances.
    const GLint clipDistances = GetInteger(GL_MAX_CLIP_DISTANCES);
    for (GLint i = 0; i < clipDistances; ++i) {
      capabilities.push_back(GL_CLIP_DISTANCE0 + static_cast<GLenum>(i));
    }
    for (GLenum cap : capabilities) {
      m_Capabilities.emplace_back(cap, GL_FN(glIsEnabled)(cap) != GL_FALSE);
      GL_FN(glDisable)(cap);
    }

    GL_FN(glBindBuffer)(GL_PIXEL_UNPACK_BUFFER, 0);
    GL_FN(glGenVertexArrays)(1, &m_DrawVertexArray);
    GL_FN(glBindVertexArray)(m_DrawVertexArray);
  }

  SampleReadback(const SampleReadback&) = delete;
  SampleReadback& operator=(const SampleReadback&) = delete;

  ~SampleReadback() {
    GL_FN(glUseProgram)(m_Program);
    for (const auto& [stage, indices] : m_Subroutines) {
      GL_FN(glUniformSubroutinesuiv)(stage, static_cast<GLsizei>(indices.size()), indices.data());
    }
    if (m_KeepingPipeline != 0) {
      GL_FN(glDeleteProgramPipelines)(1, &m_KeepingPipeline);
    }
    for (GLuint program : m_Programs) {
      GL_FN(glDeleteProgram)(program);
    }
    GL_FN(glBindVertexArray)(m_VertexArray);
    GL_FN(glDeleteVertexArrays)(1, &m_DrawVertexArray);
    if (!m_Framebuffers.empty()) {
      GL_FN(glDeleteFramebuffers)
      (static_cast<GLsizei>(m_Framebuffers.size()), m_Framebuffers.data());
    }
    GL_FN(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, m_DrawFramebuffer);
    GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, m_ReadFramebuffer);
    if (m_SwizzledTexture != 0) {
      GL_FN(glBindTexture)(m_SwizzledTarget, m_SwizzledTexture);
      GL_FN(glTexParameteriv)(m_SwizzledTarget, GL_TEXTURE_SWIZZLE_RGBA, m_Swizzle);
    }
    if (!m_TemporaryTextures.empty()) {
      GL_FN(glDeleteTextures)
      (static_cast<GLsizei>(m_TemporaryTextures.size()), m_TemporaryTextures.data());
    }
    for (const auto& [target, texture] : m_Textures) {
      GL_FN(glBindTexture)(target, texture);
    }
    GL_FN(glActiveTexture)(static_cast<GLenum>(m_ActiveTexture));
    GL_FN(glBindBuffer)(GL_PIXEL_UNPACK_BUFFER, m_UnpackBuffer);

    if (m_ViewportArrays) {
      GL_FN(glViewportIndexedfv)(0, m_Viewport);
      SetIndexed(GL_SCISSOR_TEST, m_Scissor);
    } else {
      GL_FN(glViewport)
      (static_cast<GLint>(m_Viewport[0]), static_cast<GLint>(m_Viewport[1]),
       static_cast<GLsizei>(m_Viewport[2]), static_cast<GLsizei>(m_Viewport[3]));
      Set(GL_SCISSOR_TEST, m_Scissor);
    }
    SetIndexed(GL_BLEND, m_Blend);
    GL_FN(glColorMaski)(0, m_ColorMask[0], m_ColorMask[1], m_ColorMask[2], m_ColorMask[3]);
    if (m_Compatibility && m_PolygonModes[0] != m_PolygonModes[1]) {
      GL_FN(glPolygonMode)(GL_FRONT, static_cast<GLenum>(m_PolygonModes[0]));
      GL_FN(glPolygonMode)(GL_BACK, static_cast<GLenum>(m_PolygonModes[1]));
    } else {
      GL_FN(glPolygonMode)(GL_FRONT_AND_BACK, static_cast<GLenum>(m_PolygonModes[0]));
    }
    for (const auto& [cap, enabled] : m_Capabilities) {
      Set(cap, enabled);
    }
    if (m_Compatibility) {
      GL_FN(glClampColor)(GL_CLAMP_FRAGMENT_COLOR, static_cast<GLenum>(m_ClampFragmentColor));
    }
    if (m_PauseTransformFeedback) {
      GL_FN(glResumeTransformFeedback)();
    }
  }

  // Whether the program in use survives the readback switching programs.
  bool KeepsProgram() const {
    return m_ProgramKept;
  }

  // Samples `texture`, bound to `target` on unit 0, with the identity swizzle instead of
  // `swizzle`, which texelFetch applies.
  void Unswizzle(GLenum target, GLuint texture, const GLint (&swizzle)[4]) {
    m_SwizzledTarget = target;
    m_SwizzledTexture = texture;
    std::copy(std::begin(swizzle), std::end(swizzle), m_Swizzle);
    const GLint identity[4]{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    GL_FN(glTexParameteriv)(target, GL_TEXTURE_SWIZZLE_RGBA, identity);
  }

  GLuint Texture() {
    GLuint texture = 0;
    GL_FN(glGenTextures)(1, &texture);
    m_TemporaryTextures.push_back(texture);
    return texture;
  }

  GLuint Framebuffer() {
    GLuint framebuffer = 0;
    GL_FN(glGenFramebuffers)(1, &framebuffer);
    m_Framebuffers.push_back(framebuffer);
    return framebuffer;
  }

  // A program drawing the covering triangle with `fragmentSource`; 0 if it does not link.
  GLuint Program(const std::string& fragmentSource) {
    const GLuint program = GL_FN(glCreateProgram)();
    const std::pair<GLenum, const char*> stages[] = {
        {GL_VERTEX_SHADER, COVERING_TRIANGLE_SHADER},
        {GL_FRAGMENT_SHADER, fragmentSource.c_str()},
    };
    for (const auto& [type, text] : stages) {
      const GLuint shader = GL_FN(glCreateShader)(type);
      GL_FN(glShaderSource)(shader, 1, &text, nullptr);
      GL_FN(glCompileShader)(shader);
      GL_FN(glAttachShader)(program, shader);
      GL_FN(glDeleteShader)(shader);
    }
    GL_FN(glLinkProgram)(program);
    GLint linked = GL_FALSE;
    GL_FN(glGetProgramiv)(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      GL_FN(glDeleteProgram)(program);
      return 0;
    }
    m_Programs.push_back(program);
    return program;
  }

  void SetViewport(GLsizei width, GLsizei height) const {
    if (m_ViewportArrays) {
      GL_FN(glViewportIndexedf)
      (0, 0.0f, 0.0f, static_cast<GLfloat>(width), static_cast<GLfloat>(height));
    } else {
      GL_FN(glViewport)(0, 0, width, height);
    }
  }

private:
  // A program flagged for deletion is deleted once no longer in use, unless a program pipeline
  // holds it as its active program, which takes the program in use to be linked.
  void KeepProgram(bool separateShaderObjects) {
    GLint deleted = GL_FALSE;
    if (m_Program != 0) {
      GL_FN(glGetProgramiv)(m_Program, GL_DELETE_STATUS, &deleted);
    }
    if (deleted == GL_FALSE) {
      return;
    }
    if (!separateShaderObjects) {
      m_ProgramKept = false;
      return;
    }
    GL_FN(glGenProgramPipelines)(1, &m_KeepingPipeline);
    GL_FN(glActiveShaderProgram)(m_KeepingPipeline, m_Program);
    GLint active = 0;
    GL_FN(glGetProgramPipelineiv)(m_KeepingPipeline, GL_ACTIVE_PROGRAM, &active);
    m_ProgramKept = static_cast<GLuint>(active) == m_Program;
  }

  // Using a program resets the subroutine selections, so those of the programs in use for
  // `stages` are set again after the readback.
  void SaveSubroutines(const std::vector<GLenum>& stages) {
    for (GLenum stage : stages) {
      GLint program = static_cast<GLint>(m_Program);
      if (program == 0 && m_Pipeline != 0) {
        GL_FN(glGetProgramPipelineiv)(m_Pipeline, stage, &program);
      }
      if (program == 0) {
        continue;
      }
      // The locations query can fail for a stage the program has no shader for, which has no
      // subroutine uniforms.
      const auto name = static_cast<GLuint>(program);
      GLint uniforms = 0;
      GL_FN(glGetProgramStageiv)(name, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &uniforms);
      GLint locations = 0;
      if (uniforms > 0) {
        GL_FN(glGetProgramStageiv)
        (name, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &locations);
      }
      if (locations <= 0) {
        continue;
      }
      std::vector<GLuint> indices(static_cast<size_t>(locations));
      for (GLint location = 0; location < locations; ++location) {
        GL_FN(glGetUniformSubroutineuiv)(stage, location, &indices[static_cast<size_t>(location)]);
      }
      m_Subroutines.emplace_back(stage, std::move(indices));
    }
  }

  static void Set(GLenum cap, bool enabled) {
    if (enabled) {
      GL_FN(glEnable)(cap);
    } else {
      GL_FN(glDisable)(cap);
    }
  }
  static void SetIndexed(GLenum cap, bool enabled) {
    if (enabled) {
      GL_FN(glEnablei)(cap, 0);
    } else {
      GL_FN(glDisablei)(cap, 0);
    }
  }

  bool m_Compatibility{};
  bool m_ViewportArrays{};
  bool m_PauseTransformFeedback{};
  GLuint m_Program{};
  GLuint m_Pipeline{};
  bool m_ProgramKept{true};
  GLuint m_KeepingPipeline{};
  std::vector<std::pair<GLenum, std::vector<GLuint>>> m_Subroutines;
  GLenum m_SwizzledTarget{};
  GLuint m_SwizzledTexture{};
  GLint m_Swizzle[4]{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
  GLuint m_VertexArray{};
  GLuint m_DrawFramebuffer{};
  GLuint m_ReadFramebuffer{};
  GLuint m_UnpackBuffer{};
  GLint m_ActiveTexture{};
  std::vector<std::pair<GLenum, GLuint>> m_Textures;
  GLfloat m_Viewport[4]{};
  bool m_Scissor{};
  bool m_Blend{};
  GLboolean m_ColorMask[4]{GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
  GLint m_PolygonModes[2]{GL_FILL, GL_FILL};
  GLint m_ClampFragmentColor{};
  std::vector<std::pair<GLenum, bool>> m_Capabilities;
  GLuint m_DrawVertexArray{};
  std::vector<GLuint> m_Programs;
  std::vector<GLuint> m_Framebuffers;
  std::vector<GLuint> m_TemporaryTextures;
};

} // namespace

GITSKey StateRestorer::EmitSampleProgram(const std::string& fragmentSource) {
  const GITSKey program = m_Recorder.CreateStateRestoreObjectKey();
  glCreateProgramCommand create;
  create.m_Return.Key = program;
  Emit<glCreateProgramCommandSerializer>(create);
  const std::pair<GLenum, std::string> stages[] = {
      {GL_VERTEX_SHADER, COVERING_TRIANGLE_SHADER},
      {GL_FRAGMENT_SHADER, fragmentSource},
  };
  for (const auto& [type, source] : stages) {
    const GITSKey shader = m_Recorder.CreateStateRestoreObjectKey();
    EmitShader(shader, type, source, true);
    glAttachShaderCommand attach;
    attach.m_program.Key = program;
    attach.m_shader.Key = shader;
    Emit<glAttachShaderCommandSerializer>(attach);
    glDeleteShaderCommand remove;
    remove.m_shader.Key = shader;
    Emit<glDeleteShaderCommandSerializer>(remove);
  }
  glLinkProgramCommand link;
  link.m_program.Key = program;
  Emit<glLinkProgramCommandSerializer>(link);
  return program;
}

// Multisample contents have no readback nor upload. Every sample is drawn on the replay driver
// into a single-sample texture and read back, then drawn in the subcapture from a texture into
// the restored storage, with the sample mask selecting the sample. A renderbuffer is read through
// a multisample texture it is blitted to. Depth is written as the fragment depth, and stencil one
// bit per draw. The context state restore sets the state these draws change again. Samples that
// cannot be read stop the subcapture.
void StateRestorer::RestoreMultisampleContents(GITSKey key, GLenum target, GLuint name) {
  if (!m_Version.AtLeast(3, 2)) {
    FailSamples(key, "they are read with GL 3.2; record the subcapture on a system with it.");
  }
  const auto context = m_State.Contexts().find(m_Context);
  const ContextState* tracked = context != m_State.Contexts().end() ? &context->second : nullptr;
  // Conditional rendering can discard the draws and blits reading the samples without an error.
  if (tracked != nullptr && tracked->ConditionalRenderActive) {
    FailSamples(key, "conditional rendering is active, and could discard the draws reading them; "
                     "start the subcapture range outside of it.");
  }
  // A program whose relink failed stays in use, but cannot be used again once switched from.
  if (const GLuint program = GetName(GL_CURRENT_PROGRAM); program != 0) {
    GLint linked = GL_FALSE;
    GL_FN(glGetProgramiv)(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      FailSamples(key, "the program in use failed its last link, so it could not be used again "
                       "after the draws reading them; start the subcapture range where a linked "
                       "program is in use.");
    }
  }
  // Transform feedback that is active cannot be paused, nor asked about, before GL 4.0.
  bool transformFeedback = false;
  if (Supports(4, 0, "GL_ARB_transform_feedback2")) {
    transformFeedback =
        GetInteger(GL_TRANSFORM_FEEDBACK_ACTIVE) && !GetInteger(GL_TRANSFORM_FEEDBACK_PAUSED);
  } else if (tracked != nullptr && tracked->TransformFeedbackActive) {
    FailSamples(key, "transform feedback is active, and pausing it while they are read takes "
                     "GL 4.0 or GL_ARB_transform_feedback2; record the subcapture on a system "
                     "with it.");
  }
  // A query result cannot be kept apart from the draws reading the samples.
  if (const GLenum query = ActiveDrawQuery(); query != GL_NONE) {
    std::ostringstream reason;
    reason << "a query of target 0x" << std::hex << query << std::dec
           << " is active, and the draws reading them would count in it; start the subcapture "
              "range where no such query is active.";
    FailSamples(key, reason.str());
  }
  SampleReadback live(m_Version.Compatibility, Supports(4, 1, "GL_ARB_viewport_array"),
                      Supports(4, 1, "GL_ARB_separate_shader_objects"), transformFeedback,
                      SubroutineStages());
  if (!live.KeepsProgram()) {
    FailSamples(key, "the program in use is flagged for deletion, and keeping it while they are "
                     "read takes GL 4.1 or GL_ARB_separate_shader_objects; record the subcapture "
                     "on a system with it.");
  }

  GLenum textureTarget = target;
  if (target == GL_RENDERBUFFER) {
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
    GL_FN(glGetRenderbufferParameteriv)
    (GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &stencilSize);
    GL_FN(glBindRenderbuffer)(GL_RENDERBUFFER, bound);
    if (depthSize == 0 && stencilSize > 0 && !Supports(4, 4, "GL_ARB_texture_stencil8")) {
      FailSamples(key, "stencil-only samples are read with GL 4.4 or GL_ARB_texture_stencil8; "
                       "record the subcapture on a system with it.");
    }

    textureTarget = GL_TEXTURE_2D_MULTISAMPLE;
    const GLuint copy = live.Texture();
    GL_FN(glBindTexture)(textureTarget, copy);
    m_RaisedErrors += TakeErrors();
    GL_FN(glTexImage2DMultisample)
    (textureTarget, samples, static_cast<GLenum>(internalFormat), width, height, GL_TRUE);
    if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
      FailSampleRead(key, "allocating a multisample texture", error);
    }
    // A blit between multisample framebuffers needs the same sample count.
    GLint copySamples = 0;
    GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, GL_TEXTURE_SAMPLES, &copySamples);
    if (copySamples != samples) {
      std::ostringstream reason;
      reason << "no multisample texture of format 0x" << std::hex << internalFormat << std::dec
             << " has " << samples << " samples on this system to read them through.";
      FailSamples(key, reason.str());
    }
    const GLenum attachment = SampleAttachment(depthSize, stencilSize);
    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    if (attachment != GL_COLOR_ATTACHMENT0) {
      mask =
          (depthSize > 0 ? GL_DEPTH_BUFFER_BIT : 0) | (stencilSize > 0 ? GL_STENCIL_BUFFER_BIT : 0);
    }
    // Before GL 4.1, a framebuffer whose draw or read buffer selects a missing color attachment is
    // incomplete, so a depth or stencil one selects no color buffer for both.
    const auto selectBuffers = [attachment] {
      if (attachment != GL_COLOR_ATTACHMENT0) {
        GL_FN(glDrawBuffer)(GL_NONE);
        GL_FN(glReadBuffer)(GL_NONE);
      }
    };
    const GLuint source = live.Framebuffer();
    GL_FN(glBindFramebuffer)(GL_FRAMEBUFFER, source);
    GL_FN(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, name);
    selectBuffers();
    GL_FN(glBindFramebuffer)(GL_FRAMEBUFFER, live.Framebuffer());
    GL_FN(glFramebufferTexture2D)(GL_FRAMEBUFFER, attachment, textureTarget, copy, 0);
    selectBuffers();
    GL_FN(glBindFramebuffer)(GL_READ_FRAMEBUFFER, source);
    GL_FN(glBlitFramebuffer)(0, 0, width, height, 0, 0, width, height, mask, GL_NEAREST);
    if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
      FailSampleRead(key, "blitting the renderbuffer", error);
    }
  } else {
    GL_FN(glBindTexture)(target, name);
    // texelFetch applies the swizzle, and stencil is sampled by the depth stencil texture mode. A
    // handle makes the parameters of its texture immutable, so the samples of a texture with one
    // are read from a copy of its storage when they need changing.
    GLint swizzle[4]{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    if (Supports(3, 3, "GL_ARB_texture_swizzle")) {
      GL_FN(glGetTexParameteriv)(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }
    const bool swizzled = swizzle[0] != GL_RED || swizzle[1] != GL_GREEN || swizzle[2] != GL_BLUE ||
                          swizzle[3] != GL_ALPHA;
    GLint depthSize = 0;
    GLint stencilSize = 0;
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_DEPTH_SIZE, &depthSize);
    GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_STENCIL_SIZE, &stencilSize);
    const bool depthStencil =
        depthSize > 0 && stencilSize > 0 && Supports(4, 3, "GL_ARB_stencil_texturing");
    if (m_State.HandleCreated(key) && (swizzled || depthStencil)) {
      if (!Supports(4, 3, "GL_ARB_copy_image")) {
        FailSamples(key, "they are read through parameters a handle makes immutable, and copying "
                         "them takes GL 4.3 or GL_ARB_copy_image; record the subcapture on a "
                         "system with it.");
      }
      const TextureLevel source = QueryLevel(target, 0);
      GLint samples = 0;
      GLint fixed = GL_TRUE;
      GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_SAMPLES, &samples);
      GL_FN(glGetTexLevelParameteriv)(target, 0, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, &fixed);
      const auto format = static_cast<GLenum>(source.InternalFormat);
      const auto fixedLocations = static_cast<GLboolean>(fixed);
      const GLsizei layers = std::max(source.Depth, 1);
      const GLuint copy = live.Texture();
      GL_FN(glBindTexture)(target, copy);
      m_RaisedErrors += TakeErrors();
      if (target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        GL_FN(glTexImage3DMultisample)
        (target, samples, format, source.Width, source.Height, layers, fixedLocations);
      } else {
        GL_FN(glTexImage2DMultisample)
        (target, samples, format, source.Width, source.Height, fixedLocations);
      }
      if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
        FailSampleRead(key, "allocating a multisample texture", error);
      }
      GL_FN(glCopyImageSubData)
      (name, target, 0, 0, 0, 0, copy, target, 0, 0, 0, 0, source.Width, source.Height, layers);
      if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
        FailSampleRead(key, "copying a multisample texture", error);
      }
    } else if (swizzled) {
      live.Unswizzle(target, name, swizzle);
    }
  }

  const TextureLevel level = QueryLevel(textureTarget, 0);
  GLint samples = 0;
  GLint depthSize = 0;
  GLint stencilSize = 0;
  GLint componentType = GL_NONE;
  GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, GL_TEXTURE_SAMPLES, &samples);
  GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, GL_TEXTURE_DEPTH_SIZE, &depthSize);
  GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, GL_TEXTURE_STENCIL_SIZE, &stencilSize);
  for (GLenum pname : {GL_TEXTURE_RED_TYPE, GL_TEXTURE_ALPHA_TYPE}) {
    GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, pname, &componentType);
    if (componentType != GL_NONE) {
      break;
    }
  }
  if (level.Width == 0 || level.Height == 0 || samples == 0) {
    return;
  }
  // Depth is read as a 32-bit float, which holds normalized depth of up to 24 bits exactly.
  if (depthSize > 24) {
    GLint depthType = GL_NONE;
    GL_FN(glGetTexLevelParameteriv)(textureTarget, 0, GL_TEXTURE_DEPTH_TYPE, &depthType);
    if (depthType == GL_UNSIGNED_NORMALIZED) {
      FailSamples(key, "their depth has more than 24 normalized bits, which the floating-point "
                       "texture they are read through cannot hold exactly.");
    }
  }
  // GL clamps a larger viewport without an error, so the draws would not cover every sample.
  GLint maxViewport[2]{};
  GL_FN(glGetIntegerv)(GL_MAX_VIEWPORT_DIMS, maxViewport);
  if (level.Width > maxViewport[0] || level.Height > maxViewport[1]) {
    std::ostringstream reason;
    reason << "they are " << level.Width << "x" << level.Height
           << ", larger than the viewports the draws reading and writing them can have ("
           << maxViewport[0] << "x" << maxViewport[1]
           << "); record the subcapture on a system with larger viewport limits.";
    FailSamples(key, reason.str());
  }
  const auto internalFormat = static_cast<GLenum>(level.InternalFormat);
  const bool array = textureTarget == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
  const GLint layers = array ? level.Depth : 1;
  const GLenum attachment = SampleAttachment(depthSize, stencilSize);
  const bool srgb = SrgbFormat(internalFormat);

  std::vector<SampleAspect> aspects;
  if (depthSize > 0) {
    aspects.push_back(SampleAspect::Depth);
  }
  const bool stencilTexturing = Supports(4, 3, "GL_ARB_stencil_texturing");
  if (stencilSize > 0 && stencilTexturing) {
    aspects.push_back(SampleAspect::Stencil);
  } else if (stencilSize > 0) {
    FailSamples(key, "stencil samples are read with GL 4.3 or GL_ARB_stencil_texturing; record "
                     "the subcapture on a system with it.");
  }
  if (depthSize == 0 && stencilSize == 0) {
    aspects.push_back(SampleAspect::Color);
  }
  // Depth and stencil of one texture are sampled by its depth stencil texture mode.
  const bool depthStencilMode = stencilTexturing && depthSize > 0 && stencilSize > 0;
  GLint savedMode = GL_DEPTH_COMPONENT;
  if (depthStencilMode) {
    GL_FN(glGetTexParameteriv)(textureTarget, GL_DEPTH_STENCIL_TEXTURE_MODE, &savedMode);
  }

  const GITSKey vertexArray = m_Recorder.CreateStateRestoreObjectKey();
  glGenVertexArraysCommand genVertexArray;
  genVertexArray.m_n.Value = 1;
  genVertexArray.m_arrays.Key = {vertexArray};
  genVertexArray.m_arrays.Value = {0};
  genVertexArray.m_arrays.HasValue = true;
  Emit<glGenVertexArraysCommandSerializer>(genVertexArray);
  glBindVertexArrayCommand bindVertexArray;
  bindVertexArray.m_array.Key = vertexArray;
  Emit<glBindVertexArrayCommandSerializer>(bindVertexArray);
  const GITSKey framebuffer = m_Recorder.CreateStateRestoreObjectKey();
  glGenFramebuffersCommand genFramebuffer;
  genFramebuffer.m_n.Value = 1;
  genFramebuffer.m_framebuffers.Key = {framebuffer};
  genFramebuffer.m_framebuffers.Value = {0};
  genFramebuffer.m_framebuffers.HasValue = true;
  Emit<glGenFramebuffersCommandSerializer>(genFramebuffer);
  glBindFramebufferCommand bindFramebuffer;
  bindFramebuffer.m_target.Value = GL_FRAMEBUFFER;
  bindFramebuffer.m_framebuffer.Key = framebuffer;
  Emit<glBindFramebufferCommandSerializer>(bindFramebuffer);
  if (target == GL_RENDERBUFFER) {
    glFramebufferRenderbufferCommand attach;
    attach.m_target.Value = GL_DRAW_FRAMEBUFFER;
    attach.m_attachment.Value = attachment;
    attach.m_renderbuffertarget.Value = GL_RENDERBUFFER;
    attach.m_renderbuffer.Key = key;
    Emit<glFramebufferRenderbufferCommandSerializer>(attach);
  } else if (!array) {
    glFramebufferTexture2DCommand attach;
    attach.m_target.Value = GL_DRAW_FRAMEBUFFER;
    attach.m_attachment.Value = attachment;
    attach.m_textarget.Value = target;
    attach.m_texture.Key = key;
    Emit<glFramebufferTexture2DCommandSerializer>(attach);
  }
  // Before GL 4.1, a depth or stencil framebuffer is complete with no color draw and read buffer.
  glDrawBufferCommand drawBuffer;
  drawBuffer.m_buf.Value = attachment == GL_COLOR_ATTACHMENT0 ? GL_COLOR_ATTACHMENT0 : GL_NONE;
  Emit<glDrawBufferCommandSerializer>(drawBuffer);
  glReadBufferCommand readBuffer;
  readBuffer.m_src.Value = drawBuffer.m_buf.Value;
  Emit<glReadBufferCommandSerializer>(readBuffer);
  glViewportCommand viewport;
  viewport.m_width.Value = level.Width;
  viewport.m_height.Value = level.Height;
  Emit<glViewportCommandSerializer>(viewport);
  std::vector<GLenum> disabled{GL_BLEND,
                               GL_CULL_FACE,
                               GL_SCISSOR_TEST,
                               GL_DITHER,
                               GL_COLOR_LOGIC_OP,
                               GL_POLYGON_SMOOTH,
                               GL_RASTERIZER_DISCARD,
                               GL_SAMPLE_ALPHA_TO_COVERAGE,
                               GL_SAMPLE_ALPHA_TO_ONE,
                               GL_SAMPLE_COVERAGE};
  if (Supports(4, 0, "GL_ARB_sample_shading")) {
    disabled.push_back(GL_SAMPLE_SHADING);
  }
  if (m_Version.Compatibility) {
    disabled.insert(disabled.end(), {GL_ALPHA_TEST, GL_POLYGON_STIPPLE});
  }
  for (GLenum cap : disabled) {
    EmitCapability(cap, false);
  }
  EmitCapability(GL_MULTISAMPLE, true);
  EmitCapability(GL_SAMPLE_MASK, true);
  EmitCapability(GL_FRAMEBUFFER_SRGB, srgb);
  glPolygonModeCommand polygonMode;
  polygonMode.m_face.Value = GL_FRONT_AND_BACK;
  polygonMode.m_mode.Value = GL_FILL;
  Emit<glPolygonModeCommandSerializer>(polygonMode);
  glColorMaskCommand colorMask;
  colorMask.m_red.Value = GL_TRUE;
  colorMask.m_green.Value = GL_TRUE;
  colorMask.m_blue.Value = GL_TRUE;
  colorMask.m_alpha.Value = GL_TRUE;
  Emit<glColorMaskCommandSerializer>(colorMask);
  glActiveTextureCommand activeTexture;
  activeTexture.m_texture.Value = GL_TEXTURE0;
  Emit<glActiveTextureCommandSerializer>(activeTexture);
  const GITSKey upload = m_Recorder.CreateStateRestoreObjectKey();
  glGenTexturesCommand genTexture;
  genTexture.m_n.Value = 1;
  genTexture.m_textures.Key = {upload};
  genTexture.m_textures.Value = {0};
  genTexture.m_textures.HasValue = true;
  Emit<glGenTexturesCommandSerializer>(genTexture);
  glBindTextureCommand bindTexture;
  bindTexture.m_target.Value = GL_TEXTURE_2D;
  bindTexture.m_texture.Key = upload;
  Emit<glBindTextureCommandSerializer>(bindTexture);
  for (GLenum pname : {GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER}) {
    glTexParameteriCommand parameter;
    parameter.m_target.Value = GL_TEXTURE_2D;
    parameter.m_pname.Value = pname;
    parameter.m_param.Value = GL_NEAREST;
    Emit<glTexParameteriCommandSerializer>(parameter);
  }

  const auto emitUseProgram = [this](GITSKey program) {
    glUseProgramCommand use;
    use.m_program.Key = program;
    Emit<glUseProgramCommandSerializer>(use);
  };
  const auto emitDraw = [this] {
    glDrawArraysCommand draw;
    draw.m_mode.Value = GL_TRIANGLES;
    draw.m_count.Value = 3;
    Emit<glDrawArraysCommandSerializer>(draw);
  };
  const auto words = static_cast<GLuint>((samples + 31) / 32);
  const auto emitSampleMask = [this, words](GLuint sample) {
    for (GLuint word = 0; word < words; ++word) {
      glSampleMaskiCommand command;
      command.m_maskNumber.Value = word;
      command.m_mask.Value = word == sample / 32 ? 1u << (sample % 32) : 0u;
      Emit<glSampleMaskiCommandSerializer>(command);
    }
  };

  std::vector<GITSKey> programs;
  const GLuint resolvedFramebuffer = live.Framebuffer();
  live.SetViewport(level.Width, level.Height);
  // An unsized format of the multisample storage is given the single-sample texture with the
  // type it is read back with, which glTexStorage2D could not take.
  const PixelFormat colorFormat = ReadbackFormat(textureTarget, 0, m_Version.Compatibility);
  for (SampleAspect aspect : aspects) {
    const GLenum sampleFormat = SampleTextureFormat(aspect, internalFormat);
    const PixelFormat allocation = aspect == SampleAspect::Depth ? PixelFormat{GL_RED, GL_FLOAT, 4}
                                   : aspect == SampleAspect::Stencil
                                       ? PixelFormat{GL_RED_INTEGER, GL_UNSIGNED_INT, 4}
                                       : colorFormat;
    const GLuint readProgram = live.Program(SampleReadShader(aspect, componentType, array));
    const GLuint resolved = live.Texture();
    GL_FN(glBindTexture)(GL_TEXTURE_2D, resolved);
    m_RaisedErrors += TakeErrors();
    GL_FN(glTexImage2D)
    (GL_TEXTURE_2D, 0, static_cast<GLint>(sampleFormat), level.Width, level.Height, 0,
     allocation.Format, allocation.Type, nullptr);
    if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
      FailSampleRead(key, "allocating a texture", error);
    }
    GL_FN(glBindFramebuffer)(GL_DRAW_FRAMEBUFFER, resolvedFramebuffer);
    GL_FN(glFramebufferTexture2D)
    (GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolved, 0);
    if (readProgram == 0) {
      FailSamples(key, "the shader reading them does not link on this system.");
    }
    if (const GLenum status = GL_FN(glCheckFramebufferStatus)(GL_DRAW_FRAMEBUFFER);
        status != GL_FRAMEBUFFER_COMPLETE) {
      std::ostringstream reason;
      reason << "a texture of format 0x" << std::hex << sampleFormat
             << " to draw them into is not renderable on this system (framebuffer status 0x"
             << status << std::dec << ").";
      FailSamples(key, reason.str());
    }
    GL_FN(glUseProgram)(readProgram);
    const GLint layerLocation = GL_FN(glGetUniformLocation)(readProgram, "layer");
    const GLint sampleLocation = GL_FN(glGetUniformLocation)(readProgram, "sampleIndex");
    if (aspect == SampleAspect::Color && srgb) {
      GL_FN(glEnable)(GL_FRAMEBUFFER_SRGB);
    } else {
      GL_FN(glDisable)(GL_FRAMEBUFFER_SRGB);
    }
    if (depthStencilMode) {
      GL_FN(glTexParameteri)
      (textureTarget, GL_DEPTH_STENCIL_TEXTURE_MODE,
       aspect == SampleAspect::Stencil ? GL_STENCIL_INDEX : GL_DEPTH_COMPONENT);
    }
    const PixelFormat format = ReadbackFormat(GL_TEXTURE_2D, 0, m_Version.Compatibility);

    std::vector<GITSKey> writePrograms;
    if (aspect == SampleAspect::Stencil) {
      for (GLint bit = 0; bit < stencilSize; ++bit) {
        writePrograms.push_back(EmitSampleProgram(SampleWriteShader(aspect, componentType, bit)));
      }
    } else {
      writePrograms.push_back(EmitSampleProgram(SampleWriteShader(aspect, componentType, 0)));
      emitUseProgram(writePrograms.front());
    }
    programs.insert(programs.end(), writePrograms.begin(), writePrograms.end());
    EmitCapability(GL_DEPTH_TEST, aspect == SampleAspect::Depth);
    EmitCapability(GL_STENCIL_TEST, aspect == SampleAspect::Stencil);
    if (aspect == SampleAspect::Depth) {
      glDepthFuncCommand depthFunc;
      depthFunc.m_func.Value = GL_ALWAYS;
      Emit<glDepthFuncCommandSerializer>(depthFunc);
      glDepthMaskCommand depthMask;
      depthMask.m_flag.Value = GL_TRUE;
      Emit<glDepthMaskCommandSerializer>(depthMask);
    } else if (aspect == SampleAspect::Stencil) {
      glStencilFuncCommand stencilFunc;
      stencilFunc.m_func.Value = GL_ALWAYS;
      stencilFunc.m_ref.Value = 0xFF;
      stencilFunc.m_mask.Value = 0xFF;
      Emit<glStencilFuncCommandSerializer>(stencilFunc);
      glStencilOpCommand stencilOp;
      stencilOp.m_fail.Value = GL_REPLACE;
      stencilOp.m_zfail.Value = GL_REPLACE;
      stencilOp.m_zpass.Value = GL_REPLACE;
      Emit<glStencilOpCommandSerializer>(stencilOp);
      glClearStencilCommand clearStencil;
      Emit<glClearStencilCommandSerializer>(clearStencil);
    }

    m_RaisedErrors += TakeErrors();
    for (GLint layer = 0; layer < layers; ++layer) {
      if (array) {
        glFramebufferTextureLayerCommand attach;
        attach.m_target.Value = GL_DRAW_FRAMEBUFFER;
        attach.m_attachment.Value = attachment;
        attach.m_texture.Key = key;
        attach.m_layer.Value = layer;
        Emit<glFramebufferTextureLayerCommandSerializer>(attach);
      }
      // Stencil bits are only set, from cleared stencil.
      if (aspect == SampleAspect::Stencil) {
        glStencilMaskCommand stencilMask;
        stencilMask.m_mask.Value = 0xFF;
        Emit<glStencilMaskCommandSerializer>(stencilMask);
        glClearCommand clear;
        clear.m_mask.Value = GL_STENCIL_BUFFER_BIT;
        Emit<glClearCommandSerializer>(clear);
      }
      for (GLint sample = 0; sample < samples; ++sample) {
        GL_FN(glUniform1i)(layerLocation, layer);
        GL_FN(glUniform1i)(sampleLocation, sample);
        GL_FN(glDrawArrays)(GL_TRIANGLES, 0, 3);
        if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
          FailSampleRead(key, "drawing a sample", error);
        }
        std::vector<uint8_t> pixels(static_cast<size_t>(level.Width) *
                                    static_cast<size_t>(level.Height) * format.PixelSize);
        GL_FN(glGetTexImage)(GL_TEXTURE_2D, 0, format.Format, format.Type, pixels.data());
        if (const GLenum error = GL_FN(glGetError)(); error != GL_NO_ERROR) {
          FailSampleRead(key, "reading a sample back", error);
        }

        glTexImage2DCommand image;
        image.m_target.Value = GL_TEXTURE_2D;
        image.m_internalformat.Value = static_cast<GLint>(sampleFormat);
        image.m_width.Value = level.Width;
        image.m_height.Value = level.Height;
        image.m_format.Value = format.Format;
        image.m_type.Value = format.Type;
        image.m_pixels.Data = std::move(pixels);
        image.m_pixels.HasValue = true;
        Emit<glTexImage2DCommandSerializer>(image);
        emitSampleMask(static_cast<GLuint>(sample));
        if (aspect != SampleAspect::Stencil) {
          emitDraw();
          continue;
        }
        for (GLint bit = 0; bit < stencilSize; ++bit) {
          emitUseProgram(writePrograms[static_cast<size_t>(bit)]);
          glStencilMaskCommand stencilMask;
          stencilMask.m_mask.Value = 1u << bit;
          Emit<glStencilMaskCommandSerializer>(stencilMask);
          emitDraw();
        }
      }
    }
  }
  if (depthStencilMode) {
    GL_FN(glTexParameteri)(textureTarget, GL_DEPTH_STENCIL_TEXTURE_MODE, savedMode);
  }

  emitUseProgram(0);
  for (GITSKey program : programs) {
    glDeleteProgramCommand remove;
    remove.m_program.Key = program;
    Emit<glDeleteProgramCommandSerializer>(remove);
  }
  glBindVertexArrayCommand unbindVertexArray;
  Emit<glBindVertexArrayCommandSerializer>(unbindVertexArray);
  glDeleteVertexArraysCommand deleteVertexArray;
  deleteVertexArray.m_n.Value = 1;
  deleteVertexArray.m_arrays.Key = {vertexArray};
  deleteVertexArray.m_arrays.Value = {0};
  deleteVertexArray.m_arrays.HasValue = true;
  Emit<glDeleteVertexArraysCommandSerializer>(deleteVertexArray);
  glBindFramebufferCommand unbindFramebuffer;
  unbindFramebuffer.m_target.Value = GL_FRAMEBUFFER;
  Emit<glBindFramebufferCommandSerializer>(unbindFramebuffer);
  glDeleteFramebuffersCommand deleteFramebuffer;
  deleteFramebuffer.m_n.Value = 1;
  deleteFramebuffer.m_framebuffers.Key = {framebuffer};
  deleteFramebuffer.m_framebuffers.Value = {0};
  deleteFramebuffer.m_framebuffers.HasValue = true;
  Emit<glDeleteFramebuffersCommandSerializer>(deleteFramebuffer);
  glDeleteTexturesCommand deleteTexture;
  deleteTexture.m_n.Value = 1;
  deleteTexture.m_textures.Key = {upload};
  deleteTexture.m_textures.Value = {0};
  deleteTexture.m_textures.HasValue = true;
  Emit<glDeleteTexturesCommandSerializer>(deleteTexture);
  // The blits restoring the contents of later renderbuffers write samples unmasked and unconverted.
  EmitCapability(GL_SAMPLE_MASK, false);
  EmitCapability(GL_FRAMEBUFFER_SRGB, false);
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
