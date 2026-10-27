// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "analyzerResults.h"
#include "stateTrackingService.h"
#include "subcaptureRecorder.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits::opengl2 {

// The format and type pixels are read back with, and the size of one pixel.
struct PixelFormat {
  GLenum Format;
  GLenum Type;
  size_t PixelSize;
};

// Writes commands that recreate the replay's state at the start of the subcapture range. Objects
// keep their stream keys, so the recorded commands of the range resolve them unchanged. Object
// contents and context state are read back from the replay driver. An optimized restore creates
// the objects the range does not need without their contents.
class StateRestorer {
public:
  StateRestorer(const StateTrackingService& state,
                SubcaptureRecorder& recorder,
                const AnalyzerResults& analysis)
      : m_State(state), m_Recorder(recorder), m_Analysis(analysis) {}

  void Restore();

  // The vertex array and pixel unpack state of the context current on `threadId`, which the
  // commands compiled into a display list read. Written for a context with that state initial.
  ListCompileState CaptureListCompileState(uint32_t threadId);
  // Sets the state CaptureListCompileState writes back to its initial values, where the current
  // context has it changed.
  ListCompileState ResetListCompileState();
  // The buffer, texture, renderbuffer or sampler `key` of the share group current on `threadId`,
  // read back before its deletion, or for a texture with immutable storage kept readable; empty
  // for one no container, resident handle nor tracked view keeps once it is deleted, and for one
  // GL has no object for, or cannot read.
  ObjectSnapshot CaptureDeleted(uint32_t threadId, GLObjectKind kind, GITSKey key);
  // The deleted buffers, textures, renderbuffers and samplers the context current on `threadId`
  // deleted, which none of its containers, nor a buffer texture, references any more, and no
  // resident handle nor tracked view keeps.
  std::vector<GITSKey> DetachedDeleted(uint32_t threadId);

private:
  struct Version {
    int Major{};
    int Minor{};
    bool Compatibility{};
    bool AtLeast(int major, int minor) const {
      return Major > major || (Major == major && Minor >= minor);
    }
  };

  template <typename SerializerT, typename CommandT>
  void Emit(CommandT& command) {
    if (m_Collecting) {
      return;
    }
    if (m_Capture != nullptr) {
      m_Capture->push_back(Store<SerializerT>(command));
      return;
    }
    command.m_Key = m_Recorder.CreateStateRestoreKey();
    command.m_ThreadId = m_ThreadId;
    m_Recorder.Record(SerializerT(command));
  }
  void Emit(const SampledCommand& command);

  // Makes `current`'s context current in the subcapture and on the replay driver.
  bool SwitchTo(const MakeCurrent& current);
  // The DC or GLX drawable the current context draws to on the replay driver.
  uint64_t LiveDrawable() const;
  void ReadVersion();
  void ReadExtensions();
  void MapNames(GITSKey shareGroup, GITSKey context);
  // Adds to m_Required the objects the context state at the restore point references, and those
  // the required objects depend on. Runs the restore of that state without writing it.
  void CollectRequired(const std::map<GITSKey, std::shared_ptr<MakeCurrent>>& shareGroups);
  // A deleted object is restored only for the restored objects and bindings that reference it.
  bool Required(GITSKey key) const {
    const ObjectState* object = m_State.Object(key);
    if (object != nullptr && object->Snapshot.Kept()) {
      return m_Required.contains(key);
    }
    return !m_Analysis.Optimize() || m_Required.contains(key);
  }
  GITSKey KeyOf(GLObjectKind kind, GLuint name);
  GITSKey ArbProgramKeyOf(GLuint name) const;
  GLuint NameOf(GLObjectKind kind, GITSKey key) const;
  GLenum TargetOf(GITSKey texture) const;
  bool HasExtension(const char* name) const;
  std::vector<GLenum> ArbProgramTargets() const;
  void BeginReadback();
  void EndReadback();
  void EmitCapability(GLenum cap, bool enabled);
  // Writes `pixels` into `attachment` of the framebuffer bound to GL_DRAW_FRAMEBUFFER in the
  // subcapture, through a temporary texture of `internalFormat` and a blit.
  void EmitBlit(GLenum attachment,
                GLenum internalFormat,
                GLsizei width,
                GLsizei height,
                const PixelFormat& format,
                std::vector<uint8_t> pixels);
  // The format `attachment` of the framebuffer bound to GL_READ_FRAMEBUFFER is read back with.
  static PixelFormat AttachmentReadbackFormat(GLenum attachment);

  void RestoreClientArrayBlocks();
  void RestoreSharedObjects(GITSKey shareGroup);
  void RestoreDeleted(GITSKey key, const ObjectState& object);
  void RestoreBuffer(GITSKey key, GLuint name);
  void RestoreTexture(GITSKey key, GLuint name, GLenum target);
  // `restored` holds the views already restored.
  void RestoreTextureView(GITSKey key, std::set<GITSKey>& restored);
  // A new view of the whole immutable storage of the texture `name`, bound to `target`, which
  // keeps it readable once deleted; 0 if it has mutable storage or GL has no texture views.
  GLuint ViewWholeStorage(GLuint name, GLenum target);
  // The parameters of the texture `key` bound to `target`, as captured at its deletion for one
  // read through a view of its storage.
  void RestoreTextureParameters(GITSKey key, GLenum target);
  void RestoreTextureParameters(GLenum target);
  void RestoreRenderbuffer(GITSKey key, GLuint name);
  void RestoreRenderbufferContents(GITSKey key,
                                   GLuint name,
                                   GLsizei width,
                                   GLsizei height,
                                   GLenum internalFormat,
                                   GLint depthSize,
                                   GLint stencilSize);
  void RestoreSampler(GITSKey key, GLuint name);
  void RestoreShader(GITSKey key, GLuint name);
  void EmitShader(GITSKey key, GLenum type, const std::string& source, bool compile);
  void RestoreProgram(GITSKey key, GLuint name);
  void RestoreUniforms(GITSKey key, GLuint program);
  void EmitUniform(GLuint program, GLint location, GLint capturedLocation, GLenum type);
  void RestoreSubroutines(GLuint program);
  void RestoreArbPrograms(GITSKey shareGroup);
  void RestoreDisplayLists(GITSKey shareGroup);

  template <typename SerializerT, typename CommandT, typename T>
  void EmitUniformValues(GLint capturedLocation, std::vector<T> values) {
    CommandT command;
    command.m_location.Value = capturedLocation;
    command.m_count.Value = 1;
    command.m_value.Data = std::move(values);
    command.m_value.HasValue = true;
    Emit<SerializerT>(command);
  }

  void RestoreContainers(GITSKey context);
  void RestoreVertexArray(GITSKey key, GLuint name);
  void RestoreTransformFeedback(GITSKey key, GLuint name);
  void RestoreIndexedBuffers(bool transformFeedback);
  void RestoreVertexAttributes();
  void RestoreVertexAttributeFormat(GLuint index, bool hasBuffer);
  void RestoreVertexBufferBindings();
  void RestoreClientArrays();
  void ResetClientArrays();
  void RestoreFramebuffer(GITSKey key, GLuint name);
  void RestoreDeletedAttachment(GITSKey framebuffer, GLenum attachment, GITSKey key, GLint level);
  void RestoreProgramPipeline(GITSKey key, GLuint name);
  void RestoreDefaultFramebuffer();
  void RestoreReadDrawable(const MakeCurrent& current);
  void RestoreGlxReadDrawable(const MakeCurrent& current);
  void DeletePendingObjects(const std::map<GITSKey, std::shared_ptr<MakeCurrent>>& shareGroups);
  // Whether a container of the current context `context`, or a buffer texture of `shareGroup`,
  // references the buffer, texture or renderbuffer `name`. A deletion detaches it from the
  // containers bound to the context, which `unboundOnly` skips.
  bool Attached(
      GITSKey shareGroup, GITSKey context, GLObjectKind kind, GLuint name, bool unboundOnly);
  void RestoreContextState();
  void RestoreDrawBufferState();
  void RestoreVertexAttributeValues();
  void RestoreCompatibilityState();
  void RestoreFixedFunctionTransforms();
  void RestoreTextureUnit(GLint unit, bool coordinates, bool environment);
  void RestoreBindings();
  void RestoreActiveQueries();
  void RestoreArbProgramBindings();

  const StateTrackingService& m_State;
  SubcaptureRecorder& m_Recorder;
  const AnalyzerResults& m_Analysis;
  bool m_Collecting{false};
  std::unordered_set<GITSKey> m_Required;
  // Set while capturing commands instead of writing them.
  std::vector<std::shared_ptr<StoredCommand>>* m_Capture{};
  // The share group and context whose names are mapped at the first key lookup.
  std::optional<std::pair<GITSKey, GITSKey>> m_UnmappedNames;
  uint32_t m_ThreadId{};
  // The recorded threads made current on, and the recorded display of those that used GLX.
  std::map<uint32_t, Display*> m_UsedThreads;
  HDC m_LiveDc{};
  HDC m_RecordedDc{};
  // For a context made current through GLX, the drawable it draws to on the replay driver and as
  // recorded, and the recorded display, which a WGL context has none of.
  GLXDrawable m_LiveDrawable{};
  GLXDrawable m_RecordedDrawable{};
  Display* m_RecordedDisplay{};
  // The DCs and GLX drawables whose contents are restored.
  std::set<uint64_t> m_RestoredDrawables;
  Version m_Version;
  std::set<std::string> m_Extensions;
  std::unordered_map<GLObjectKind, std::unordered_map<GLuint, GITSKey>> m_Keys;
  std::unordered_map<GLuint, GITSKey> m_ArbProgramKeys;
  std::vector<std::pair<GLenum, GLint>> m_SavedPackState;
  // Captured locations for uniforms the application never looked up before the range, far
  // above the locations a driver assigns.
  GLint m_NextUnusedLocation{0x40000000};
};

} // namespace gits::opengl2
