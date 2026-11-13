// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"
#include "objectNameMapService.h"
#include "subcaptureRecorder.h"
#include "windowGeometryMetaTypes.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits::opengl {

// A copy of a replayed command, written again under a state-restore key, on its own thread or
// on `threadId`. It keeps the serialized command, so storing commands of thousands of types
// instantiates no code per type.
class StoredCommand {
public:
  explicit StoredCommand(const stream::CommandSerializer& serializer);

  void Record(SubcaptureRecorder& recorder,
              GITSKey key,
              std::optional<uint32_t> threadId = std::nullopt) const;

private:
  unsigned m_Id{};
  std::vector<char> m_Data;
};

std::shared_ptr<StoredCommand> StoreSerialized(const stream::CommandSerializer& serializer);

template <typename SerializerT, typename CommandT>
std::shared_ptr<StoredCommand> Store(const CommandT& command) {
  return StoreSerialized(SerializerT(command));
}

// A recorded command with the window geometry sample the recorder took for it, if any.
struct SampledCommand {
  std::shared_ptr<StoredCommand> Geometry;
  std::shared_ptr<StoredCommand> Command;
};

struct MakeCurrent {
  SampledCommand Call;
  uint32_t ThreadId{};
  GITSKey Context{};
  HDC Dc{};
  // The draw DC, except for a make-context-current call that reads another drawable.
  HDC ReadDc{};
  // The recorded display, draw drawable and read drawable of a GLX make-current call.
  Display* GlxDisplay{};
  GLXDrawable Drawable{};
  GLXDrawable ReadDrawable{};
};

struct ContextState {
  GITSKey ShareGroup{};
  std::shared_ptr<MakeCurrent> LastMakeCurrent;
};

// Commands that create an object again as it was read back, and the objects they reference.
struct ObjectSnapshot {
  std::vector<std::shared_ptr<StoredCommand>> Commands;
  std::vector<GITSKey> References;
  // Level 0 of a texture, or a renderbuffer, which framebuffers it is attached to can still
  // render to.
  GLsizei Width{};
  GLsizei Height{};
  GLenum InternalFormat{};
  GLint Samples{};
  // Instead of the commands, a texture with immutable storage keeps a view of its whole storage,
  // through which it is read when restored, since views, framebuffers and resident image
  // handles can still change it. The view has default parameters, so the texture's own are
  // kept as the commands that set them.
  GLuint Storage{};
  std::vector<std::shared_ptr<StoredCommand>> Parameters;

  // Whether the deleted object is kept to be restored.
  bool Kept() const {
    return !Commands.empty() || Storage != 0;
  }
};

struct ObjectState {
  GLObjectKind Kind{};
  // The share group of shared objects, the context of container objects.
  GITSKey Owner{};
  GLenum Target{};
  // The glTextureView that made a texture a view of `ViewOf`, whose storage it shares.
  std::shared_ptr<StoredCommand> View;
  GITSKey ViewOf{};
  // A shader still attached or a program still in use when deleted stays live, under this name,
  // until GL reclaims it. A deleted buffer, texture or renderbuffer stays live while a container
  // or a buffer texture references it, a texture or sampler while one of its handles is resident,
  // and a texture while a view of it is tracked; it is tracked, under this name, while one does.
  bool DeletePending{};
  GLuint LiveName{};
  // The deleted buffer, texture, renderbuffer or sampler as it was before its deletion, and the
  // context that deleted it.
  ObjectSnapshot Snapshot;
  GITSKey DeletedIn{};
  // Whether a texture or image handle of the texture was ever created.
  bool HandleCreated{};
};

struct ProgramState {
  // Shaders attached at the last link, as (type, source).
  std::vector<std::pair<GLenum, std::string>> LinkedShaders;
  // Commands that affect links: output locations, varyings and parameters.
  std::vector<std::shared_ptr<StoredCommand>> LinkSetup;
  // The number of LinkSetup commands issued before the last link.
  size_t LinkedSetupCount{};
  // The glCreateShaderProgramv call that created the program, until it is linked again.
  std::shared_ptr<StoredCommand> Creation;
  // The glProgramBinary call that loaded the program, until it is linked again.
  std::shared_ptr<StoredCommand> Binary;
  // The glUniformHandle* or glProgramUniformHandle* call that set each sampler or image uniform
  // holding a bindless handle, by recorded location. Reading a uniform back gives only the low
  // 32 bits of its handle.
  std::map<GLint, std::shared_ptr<StoredCommand>> HandleUniforms;
};

// An ARB assembly program. Its names are separate from those of GLSL programs.
struct ArbProgramState {
  GITSKey ShareGroup{};
  GLenum Target{};
};

// Commands that set the vertex array and pixel unpack state of a context, and the buffers they
// reference.
struct ListCompileState {
  std::vector<std::shared_ptr<StoredCommand>> Commands;
  std::vector<GITSKey> Buffers;
};

// The commands from glNewList to glEndList, recorded as compiled. They are preceded by the state
// the list was compiled with, which compiled draws and pixel commands read, and followed by the
// commands that set that state back to its initial values.
struct DisplayList {
  std::vector<std::shared_ptr<StoredCommand>> Commands;
  std::vector<GITSKey> Buffers;
  // Whether glNewList also executed the commands, with GL_COMPILE_AND_EXECUTE.
  bool Executed{};
  // Whether the list can change texture images or calls other lists, which can.
  bool ChangesTextures{};
};

// A bindless texture or image handle of `Texture`, and `Sampler` for a texture sampler handle.
// `Creation` keeps the handle the recording returned.
struct BindlessHandle {
  GITSKey Texture{};
  GITSKey Sampler{};
  std::shared_ptr<StoredCommand> Creation;
};

// The display lists a wglUseFontBitmaps call created, followed by the glDeleteLists that deleted
// some of them since.
struct FontLists {
  GITSKey ShareGroup{};
  GLuint First{};
  GLuint Count{};
  std::vector<std::shared_ptr<StoredCommand>> Commands;
};

// An image of a compressed texture level, as the application uploaded it whole.
struct CompressedImage {
  GLenum InternalFormat{};
  GLsizei Width{};
  GLsizei Height{};
  GLsizei Depth{};
  std::vector<uint8_t> Data;
};

// Tracks what state restore needs from the commands replayed before the subcapture range.
class StateTrackingService {
public:
  void SetGeometry(const WindowGeometryMetaCommand& command);
  // Takes the geometry sample of the command being replayed.
  SampledCommand Sample(std::shared_ptr<StoredCommand> command);

  void AddTopology(SampledCommand command);
  void CreateContext(GITSKey context, GITSKey shareContext);
  void ShareLists(GITSKey context, GITSKey otherContext);
  void DeleteContext(GITSKey context);
  // A destroyed GLX context stays usable until the thread it is current on releases it. One that
  // is still current is kept, and `destroy` is added to the topology and the context deleted
  // once no thread has it current any more.
  void DestroyGlxContext(GITSKey context, SampledCommand destroy);
  // The same for a GLX window, pbuffer or pixmap: `destroy` is added to the topology once no thread
  // has `drawable` current any more.
  void DestroyGlxDrawable(GLXDrawable drawable, SampledCommand destroy);
  void SetCurrent(uint32_t threadId, GITSKey context, HDC dc, HDC readDc, SampledCommand call);
  void SetCurrentGlx(uint32_t threadId,
                     GITSKey context,
                     Display* display,
                     GLXDrawable drawable,
                     GLXDrawable readDrawable,
                     SampledCommand call);
  void CreatePbuffer(GITSKey pbuffer, GLsizei width, GLsizei height);
  void GetPbufferDc(GITSKey pbuffer, HDC dc);
  void ReleasePbufferDc(HDC dc);
  void DestroyPbuffer(GITSKey pbuffer);
  // The size of the pbuffer a recorded DC belongs to; std::nullopt for other DCs.
  std::optional<std::pair<GLsizei, GLsizei>> PbufferSize(HDC dc) const;

  void CreateObject(GLObjectKind kind, uint32_t threadId, GITSKey key);
  // `liveName` is the name of a deleted shader or program GL keeps until it is detached or no
  // longer in use, or of a deleted buffer, texture, renderbuffer or sampler read back into
  // `snapshot` before `context` deleted it; 0 for an object deleted at once.
  void DeleteObject(GLObjectKind kind,
                    GITSKey key,
                    GLuint liveName = 0,
                    ObjectSnapshot snapshot = {},
                    GITSKey context = 0);
  // Forgets the delete-pending shader or program whose name GL gave to the shader or program just
  // created, since GL reclaimed it first.
  void ForgetReclaimed(uint32_t threadId, GLuint name);
  // Forgets the deleted buffer, texture, renderbuffer or sampler nothing keeps alive any more.
  void ForgetDeleted(GITSKey key);
  // Whether `name` is the name of a tracked deleted buffer, texture, renderbuffer or sampler of
  // `shareGroup`.
  bool IsDeletedName(GITSKey shareGroup, GLObjectKind kind, GLuint name) const {
    return m_DeletedNames.contains({shareGroup, kind, name});
  }
  void SetTarget(GITSKey texture, GLenum target);
  void SetView(GITSKey texture, GITSKey original, std::shared_ptr<StoredCommand> view);
  // Whether a tracked texture is a view of `texture`.
  bool Viewed(GITSKey texture) const {
    return m_ViewOriginals.contains(texture);
  }
  ProgramState* Program(GITSKey program);
  // A texture whose levels have different formats can read back its compressed images wrong, so
  // the last whole upload of each compressed image is kept, by share group, texture name, image
  // target and level, until a command can change that image.
  void KeepCompressedImage(
      GITSKey shareGroup, GLuint texture, GLenum imageTarget, GLint level, CompressedImage image);
  // Forgets the kept compressed images of `shareGroup`, only those of `texture`, `imageTarget`
  // and `level` where given.
  void ForgetCompressedImages(GITSKey shareGroup,
                              std::optional<GLuint> texture = std::nullopt,
                              std::optional<GLenum> imageTarget = std::nullopt,
                              std::optional<GLint> level = std::nullopt);
  const CompressedImage* FindCompressedImage(GITSKey shareGroup,
                                             GLuint texture,
                                             GLenum imageTarget,
                                             GLint level) const;

  void CreateSync(uint32_t threadId, GITSKey key);
  void DeleteSync(GITSKey key);

  void CreateArbProgram(uint32_t threadId, GITSKey key);
  void SetArbProgramTarget(GITSKey key, GLenum target);
  void DeleteArbProgram(GITSKey key);

  // Handles are deleted with their texture or sampler. A resident handle keeps a deleted texture
  // or sampler alive, so its handles are kept until it is forgotten.
  void CreateHandle(GLuint64 handle, BindlessHandle state);
  // Residency is per context; `command` makes the handle resident again. GL rejects making a
  // resident handle resident, so the first command is kept until MakeNonResident().
  void MakeResident(uint32_t threadId, GLuint64 handle, std::shared_ptr<StoredCommand> command);
  void MakeNonResident(uint32_t threadId, GLuint64 handle);
  // Whether a handle of the texture or sampler `object` is resident in any context.
  bool HasResidentHandle(GITSKey object) const;
  // Whether a texture or image handle of the texture was ever created, which makes its parameters
  // immutable even after the handle is deleted with its sampler.
  bool HandleCreated(GITSKey texture) const;

  // `newList` is written in place of the glNewList that starts the list.
  void BeginList(uint32_t threadId,
                 GLuint list,
                 bool executed,
                 ListCompileState compileState,
                 std::shared_ptr<StoredCommand> newList);
  bool CompilingList(uint32_t threadId) const {
    return m_CompilingLists.contains(threadId);
  }
  // Whether a list compiles on `threadId` with GL_COMPILE, so its commands do not execute.
  bool CompilingOnly(uint32_t threadId) const {
    const auto compiling = m_CompilingLists.find(threadId);
    return compiling != m_CompilingLists.end() && !compiling->second.second.Executed;
  }
  void MarkListChangesTextures(uint32_t threadId);
  // Whether executing `list`, or any list of `shareGroup` when none is given, can change texture
  // images.
  bool ListChangesTextures(GITSKey shareGroup, std::optional<GLuint> list) const;
  void AddListCommand(uint32_t threadId, std::shared_ptr<StoredCommand> command);
  void EndList(uint32_t threadId, ListCompileState reset);
  void CreateFontLists(uint32_t threadId,
                       GLuint first,
                       GLuint count,
                       std::shared_ptr<StoredCommand> command);
  void DeleteLists(uint32_t threadId,
                   GLuint list,
                   GLsizei range,
                   std::shared_ptr<StoredCommand> command);
  // The share group of the context current on `threadId`, and that context.
  std::pair<GITSKey, GITSKey> CurrentShareGroup(uint32_t threadId) const;

  const std::vector<SampledCommand>& Topology() const {
    return m_Topology;
  }
  const std::map<GITSKey, ContextState>& Contexts() const {
    return m_Contexts;
  }
  const std::map<uint32_t, std::shared_ptr<MakeCurrent>>& Threads() const {
    return m_Threads;
  }
  // The destroy commands of the GLX contexts still current on a thread, by context.
  const std::map<GITSKey, SampledCommand>& PendingGlxDestroys() const {
    return m_PendingGlxDestroys;
  }
  // The destroy commands of the GLX drawables still current on a thread, by drawable.
  const std::map<GLXDrawable, SampledCommand>& PendingGlxDrawableDestroys() const {
    return m_PendingGlxDrawableDestroys;
  }
  const std::map<GITSKey, ObjectState>& Objects() const {
    return m_Objects;
  }
  const ObjectState* Object(GITSKey key) const;
  const ProgramState* FindProgram(GITSKey program) const;
  const std::map<GITSKey, GITSKey>& Syncs() const {
    return m_Syncs;
  }
  const std::map<GITSKey, ArbProgramState>& ArbPrograms() const {
    return m_ArbPrograms;
  }
  // By share group and the handle the recording returned, which is unique only within its share
  // group.
  const std::map<std::pair<GITSKey, GLuint64>, BindlessHandle>& Handles() const {
    return m_Handles;
  }
  // By context and handle.
  const std::map<GITSKey, std::map<GLuint64, std::shared_ptr<StoredCommand>>>& ResidentHandles()
      const {
    return m_ResidentHandles;
  }
  // By share group and list name.
  const std::map<std::pair<GITSKey, GLuint>, DisplayList>& DisplayLists() const {
    return m_DisplayLists;
  }
  // In the order they were created.
  const std::vector<FontLists>& Fonts() const {
    return m_Fonts;
  }

private:
  void SetCurrent(std::shared_ptr<MakeCurrent> current);
  bool IsGlxDrawableCurrent(GLXDrawable drawable) const;
  GITSKey CurrentContext(uint32_t threadId) const;
  GITSKey OwnerFor(GLObjectKind kind, uint32_t threadId) const;
  GITSKey ShareGroupOf(GITSKey context) const;
  // Deletes the handles of the texture or sampler `object`.
  void DeleteHandles(GITSKey object);
  void EraseObject(std::map<GITSKey, ObjectState>::iterator object);

  std::shared_ptr<StoredCommand> m_Geometry;
  std::vector<SampledCommand> m_Topology;
  std::map<GITSKey, ContextState> m_Contexts;
  std::map<uint32_t, std::shared_ptr<MakeCurrent>> m_Threads;
  std::map<GITSKey, SampledCommand> m_PendingGlxDestroys;
  std::map<GLXDrawable, SampledCommand> m_PendingGlxDrawableDestroys;
  std::map<GITSKey, std::pair<GLsizei, GLsizei>> m_PbufferSizes;
  // Recorded pbuffer DC to its pbuffer.
  std::map<HDC, GITSKey> m_PbufferDcs;
  std::map<GITSKey, ObjectState> m_Objects;
  // Share group, kind and name of the tracked deleted buffers, textures, renderbuffers and
  // samplers.
  std::multiset<std::tuple<GITSKey, GLObjectKind, GLuint>> m_DeletedNames;
  // The original of each tracked view.
  std::multiset<GITSKey> m_ViewOriginals;
  std::unordered_map<GITSKey, ProgramState> m_Programs;
  std::map<std::tuple<GITSKey, GLuint, GLenum, GLint>, CompressedImage> m_CompressedImages;
  // Sync object key to its share group.
  std::map<GITSKey, GITSKey> m_Syncs;
  std::map<GITSKey, ArbProgramState> m_ArbPrograms;
  std::map<std::pair<GITSKey, GLuint64>, BindlessHandle> m_Handles;
  std::map<GITSKey, std::map<GLuint64, std::shared_ptr<StoredCommand>>> m_ResidentHandles;
  std::map<std::pair<GITSKey, GLuint>, DisplayList> m_DisplayLists;
  // Thread to the list it compiles and its share group.
  std::map<uint32_t, std::pair<std::pair<GITSKey, GLuint>, DisplayList>> m_CompilingLists;
  std::vector<FontLists> m_Fonts;
};

bool IsContainerKind(GLObjectKind kind);

} // namespace gits::opengl
