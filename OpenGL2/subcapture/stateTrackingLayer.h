// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "analyzerResults.h"
#include "layerAuto.h"
#include "stateTrackingService.h"
#include "subcaptureRange.h"
#include "subcaptureRecorder.h"

namespace gits::opengl2 {

// Feeds StateTrackingService from replayed commands, before the subcapture range starts. Tracks
// what state restore cannot query from the driver: contexts, which objects exist, texture
// targets, program link inputs, syncs, bindless handles, display lists and deleted objects
// containers may still reference. RecordingLayer adds the commands compiled into a display list.
class StateTrackingLayer : public Layer {
public:
  StateTrackingLayer(StateTrackingService& state,
                     SubcaptureRecorder& recorder,
                     const AnalyzerResults& analysis,
                     const SubcaptureRange& range)
      : Layer("StateTracking"),
        m_State(state),
        m_Recorder(recorder),
        m_Analysis(analysis),
        m_Range(range) {}

  // ---- Window geometry and pixel formats --------------------------------
  void Post(WindowGeometryMetaCommand& command) override;
  void Post(PixelFormatMetaCommand& command) override;
  void Post(wglSetPixelFormatCommand& command) override;

  // ---- Contexts ---------------------------------------------------------
  void Post(wglCreateContextCommand& command) override;
  void Post(wglCreateContextAttribsARBCommand& command) override;
  void Post(wglShareListsCommand& command) override;
  void Post(wglDeleteContextCommand& command) override;
  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;

  // ---- Pbuffers ---------------------------------------------------------
  void Post(wglCreatePbufferARBCommand& command) override;
  void Post(wglCreatePbufferEXTCommand& command) override;
  void Post(wglGetPbufferDCARBCommand& command) override;
  void Post(wglGetPbufferDCEXTCommand& command) override;
  void Post(wglReleasePbufferDCARBCommand& command) override;
  void Post(wglReleasePbufferDCEXTCommand& command) override;
  void Post(wglDestroyPbufferARBCommand& command) override;
  void Post(wglDestroyPbufferEXTCommand& command) override;

  // ---- Frames -----------------------------------------------------------
  void Post(wglSwapBuffersCommand& command) override;
  void Post(wglSwapLayerBuffersCommand& command) override;

  // ---- Buffers ----------------------------------------------------------
  void Post(glGenBuffersCommand& command) override;
  void Post(glGenBuffersARBCommand& command) override;
  void Post(glCreateBuffersCommand& command) override;
  void Pre(glBindBufferCommand& command) override;
  void Post(glBindBufferCommand& command) override;
  void Pre(glBindBufferARBCommand& command) override;
  void Post(glBindBufferARBCommand& command) override;
  void Pre(glDeleteBuffersCommand& command) override;
  void Pre(glDeleteBuffersARBCommand& command) override;
  void Post(glFreeObjectBufferATICommand& command) override;

  // ---- Textures ---------------------------------------------------------
  void Post(glGenTexturesCommand& command) override;
  void Post(glGenTexturesEXTCommand& command) override;
  void Post(glCreateTexturesCommand& command) override;
  void Pre(glBindTextureCommand& command) override;
  void Post(glBindTextureCommand& command) override;
  void Pre(glBindTextureEXTCommand& command) override;
  void Post(glBindTextureEXTCommand& command) override;
  void Post(glBindMultiTextureEXTCommand& command) override;
  void Pre(glTextureViewCommand& command) override;
  void Post(glTextureViewCommand& command) override;
  void Pre(glTextureViewEXTCommand& command) override;
  void Post(glTextureViewEXTCommand& command) override;
  void Pre(glTextureViewOESCommand& command) override;
  void Post(glTextureViewOESCommand& command) override;
  void Pre(glDeleteTexturesCommand& command) override;
  void Pre(glDeleteTexturesEXTCommand& command) override;

  // ---- Renderbuffers ----------------------------------------------------
  void Post(glGenRenderbuffersCommand& command) override;
  void Post(glGenRenderbuffersEXTCommand& command) override;
  void Post(glGenRenderbuffersOESCommand& command) override;
  void Post(glCreateRenderbuffersCommand& command) override;
  void Pre(glBindRenderbufferEXTCommand& command) override;
  void Post(glBindRenderbufferEXTCommand& command) override;
  void Pre(glDeleteRenderbuffersCommand& command) override;
  void Pre(glDeleteRenderbuffersEXTCommand& command) override;
  void Pre(glDeleteRenderbuffersOESCommand& command) override;

  // ---- Samplers ---------------------------------------------------------
  void Post(glGenSamplersCommand& command) override;
  void Post(glCreateSamplersCommand& command) override;
  void Pre(glDeleteSamplersCommand& command) override;

  // ---- Bindless handles -------------------------------------------------
  void Pre(glGetTextureHandleARBCommand& command) override;
  void Pre(glGetTextureHandleNVCommand& command) override;
  void Pre(glGetTextureHandleIMGCommand& command) override;
  void Pre(glGetTextureSamplerHandleARBCommand& command) override;
  void Pre(glGetTextureSamplerHandleNVCommand& command) override;
  void Pre(glGetTextureSamplerHandleIMGCommand& command) override;
  void Pre(glGetImageHandleARBCommand& command) override;
  void Pre(glGetImageHandleNVCommand& command) override;
  void Post(glMakeTextureHandleResidentARBCommand& command) override;
  void Post(glMakeTextureHandleResidentNVCommand& command) override;
  void Post(glMakeTextureHandleNonResidentARBCommand& command) override;
  void Post(glMakeTextureHandleNonResidentNVCommand& command) override;
  void Post(glMakeImageHandleResidentARBCommand& command) override;
  void Post(glMakeImageHandleResidentNVCommand& command) override;
  void Post(glMakeImageHandleNonResidentARBCommand& command) override;
  void Post(glMakeImageHandleNonResidentNVCommand& command) override;
  void Post(glUniformHandleui64ARBCommand& command) override;
  void Post(glUniformHandleui64NVCommand& command) override;
  void Post(glUniformHandleui64IMGCommand& command) override;
  void Post(glProgramUniformHandleui64ARBCommand& command) override;
  void Post(glProgramUniformHandleui64NVCommand& command) override;
  void Post(glProgramUniformHandleui64IMGCommand& command) override;
  void Post(glUniform1iCommand& command) override;
  void Post(glUniform1iARBCommand& command) override;
  void Post(glUniform1ivCommand& command) override;
  void Post(glUniform1ivARBCommand& command) override;
  void Post(glProgramUniform1iCommand& command) override;
  void Post(glProgramUniform1iEXTCommand& command) override;
  void Post(glProgramUniform1ivCommand& command) override;
  void Post(glProgramUniform1ivEXTCommand& command) override;

  // ---- Shaders and programs ---------------------------------------------
  void Post(glCreateShaderCommand& command) override;
  void Post(glCreateShaderObjectARBCommand& command) override;
  void Post(glDeleteShaderCommand& command) override;
  void Post(glCreateProgramCommand& command) override;
  void Post(glCreateProgramObjectARBCommand& command) override;
  void Post(glCreateShaderProgramvCommand& command) override;
  void Post(glDeleteProgramCommand& command) override;
  void Post(glDeleteObjectARBCommand& command) override;
  void Post(glProgramBinaryCommand& command) override;
  // Link inputs: the attached shaders and the calls that affect the next link.
  void Post(glLinkProgramCommand& command) override;
  void Post(glLinkProgramARBCommand& command) override;
  void Post(glBindFragDataLocationCommand& command) override;
  void Post(glBindFragDataLocationEXTCommand& command) override;
  void Post(glTransformFeedbackVaryingsCommand& command) override;
  void Post(glProgramParameteriCommand& command) override;
  void Post(glProgramParameteriARBCommand& command) override;
  void Post(glProgramParameteriEXTCommand& command) override;

  // ---- ARB assembly programs --------------------------------------------
  void Post(glGenProgramsARBCommand& command) override;
  void Post(glBindProgramARBCommand& command) override;
  void Post(glDeleteProgramsARBCommand& command) override;

  // ---- Display lists ----------------------------------------------------
  void Post(glNewListCommand& command) override;
  void Post(glEndListCommand& command) override;
  void Post(glDeleteListsCommand& command) override;
  void Post(wglUseFontBitmapsACommand& command) override;
  void Post(wglUseFontBitmapsWCommand& command) override;

  // ---- Program pipelines ------------------------------------------------
  void Post(glGenProgramPipelinesCommand& command) override;
  void Post(glGenProgramPipelinesEXTCommand& command) override;
  void Post(glCreateProgramPipelinesCommand& command) override;
  void Post(glDeleteProgramPipelinesCommand& command) override;
  void Post(glDeleteProgramPipelinesEXTCommand& command) override;
  // ---- Vertex arrays ----------------------------------------------------
  void Post(glGenVertexArraysCommand& command) override;
  void Post(glGenVertexArraysAPPLECommand& command) override;
  void Post(glGenVertexArraysOESCommand& command) override;
  void Post(glCreateVertexArraysCommand& command) override;
  void Post(glBindVertexArrayAPPLECommand& command) override;
  void Post(glDeleteVertexArraysCommand& command) override;
  void Post(glDeleteVertexArraysAPPLECommand& command) override;
  void Post(glDeleteVertexArraysOESCommand& command) override;

  // ---- Framebuffers -----------------------------------------------------
  void Post(glGenFramebuffersCommand& command) override;
  void Post(glGenFramebuffersEXTCommand& command) override;
  void Post(glGenFramebuffersOESCommand& command) override;
  void Post(glCreateFramebuffersCommand& command) override;
  void Post(glBindFramebufferEXTCommand& command) override;
  void Post(glDeleteFramebuffersCommand& command) override;
  void Post(glDeleteFramebuffersEXTCommand& command) override;
  void Post(glDeleteFramebuffersOESCommand& command) override;

  // ---- Queries ----------------------------------------------------------
  void Post(glGenQueriesCommand& command) override;
  void Post(glGenQueriesARBCommand& command) override;
  void Post(glGenQueriesEXTCommand& command) override;
  void Post(glCreateQueriesCommand& command) override;
  void Post(glBeginQueryCommand& command) override;
  void Post(glBeginQueryARBCommand& command) override;
  void Post(glBeginQueryIndexedCommand& command) override;
  void Post(glQueryCounterCommand& command) override;
  void Post(glDeleteQueriesCommand& command) override;
  void Post(glDeleteQueriesARBCommand& command) override;
  void Post(glDeleteQueriesEXTCommand& command) override;

  // ---- Transform feedbacks ----------------------------------------------
  void Post(glGenTransformFeedbacksCommand& command) override;
  void Post(glGenTransformFeedbacksNVCommand& command) override;
  void Post(glCreateTransformFeedbacksCommand& command) override;
  void Post(glDeleteTransformFeedbacksCommand& command) override;
  void Post(glDeleteTransformFeedbacksNVCommand& command) override;

  // ---- Syncs ------------------------------------------------------------
  void Post(glFenceSyncCommand& command) override;
  void Post(glDeleteSyncCommand& command) override;

private:
  // A deleted buffer, texture or renderbuffer stays live while a container that is not bound, or
  // a buffer texture, references it, but GL no longer reads it by name. Before the range, it is
  // read back before its deletion.
  void DeleteReadBack(GLObjectKind kind,
                      uint32_t threadId,
                      const HandleArrayArgument<GLuint>& names);
  // GL gives the name of a deleted object to new objects although containers still reference the
  // deleted object by it. Before the range, a new object given the name of a tracked deleted
  // object gets another name from `gen`, so the name keeps identifying the deleted object.
  template <typename GenFn>
  void Rename(GLObjectKind kind, uint32_t threadId, GITSKey key, GLuint& name, GenFn gen);
  template <typename GenFn>
  void Rename(GLObjectKind kind, uint32_t threadId, HandleArrayArgument<GLuint>& names, GenFn gen);
  // Forgets the deleted objects the current context deleted which no container references any
  // more.
  void ForgetDetached(uint32_t threadId);
  // GL creates a view only in a name that is not a texture yet, and leaves the name unused when
  // it rejects the view, so a view is tracked only if its name became a texture.
  template <typename SerializerT, typename CommandT>
  void SetView(const CommandT& command);

  StateTrackingService& m_State;
  SubcaptureRecorder& m_Recorder;
  const AnalyzerResults& m_Analysis;
  const SubcaptureRange& m_Range;
  // Whether the name of the view being created was not a texture before the call.
  bool m_ViewNameUnused{};
};

} // namespace gits::opengl2
