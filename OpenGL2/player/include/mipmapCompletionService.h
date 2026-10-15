// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Defensive replay-time fixup for one texture-completeness edge case.
//
// Per the GL spec (4.6 section 8.17, "Texture Completeness"), GL_TEXTURE_MAX_LEVEL clamps the
// set of mip levels a texture needs for completeness - a GL_TEXTURE_2D whose
// GL_TEXTURE_MAX_LEVEL is 0 is defined to be complete with only level 0 populated, even while
// its own (or an overriding bound sampler object's) GL_TEXTURE_MIN_FILTER names one of the four
// mipmap-family filters (GL_{NEAREST,LINEAR}_MIPMAP_{NEAREST,LINEAR}). Replay should not depend
// on how an implementation handles that edge case.
//
// With GL_TEXTURE_MAX_LEVEL 0 a mipmap filter can only select level 0, so it samples exactly
// like its non-mipmap counterpart (GL_NEAREST_MIPMAP_* like GL_NEAREST, GL_LINEAR_MIPMAP_* like
// GL_LINEAR), which no implementation treats as incomplete. For each sampling command, the player
// switches such a texture's (or its bound sampler's) GL_TEXTURE_MIN_FILTER to that counterpart
// right before the command and restores it right after, so no state the application could later
// observe - its filter, its levels, or completeness after a later max-level change - differs
// from capture. Acts only when level 1 is missing, the case the quirk was found in.

#include "glTypes.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <vector>

namespace gits::opengl2 {

class MipmapCompletionService {
public:
  static MipmapCompletionService& Get();

  // Called from the texture and sampler bind runners (GlBindTextureRunner::Run() and siblings,
  // commandRunnersCustom.cpp) after their real bind call, to mark `unit` as one
  // BeforeSampling() checks. `unit` is a texture unit index (0-based - e.g. unit 3 means
  // GL_TEXTURE3), matching glActiveTexture's/glBindSampler's own argument convention, NOT a raw
  // GL_TEXTUREn enum value.
  void NoteUnit(GLuint unit);

  // Called by every generated runner of a command that can sample textures, right before its
  // real call (see texture_sampling_commands.py). Checks the GL_TEXTURE_2D bound on every noted
  // unit against live GL state only, since the texture and sampler parameters can change after
  // the bind, and overrides the filter where needed. Does nothing while a display list is being
  // compiled, since its state changes would be compiled into the list; executing the list is
  // handled by BeforeCallList(s).
  //
  // Saves and restores GL_ACTIVE_TEXTURE around its own queries, so
  // a call here never observably changes which unit is active for the caller once it returns -
  // mirrors this backend's other "peek at extra live state, then put it back" helpers (e.g.
  // GlUseProgramRunner::Run()'s own GL_CURRENT_PROGRAM query, commandRunnersCustom.cpp).
  void BeforeSampling();

  // Called before glCallList/glCallLists instead of BeforeSampling(). A texture the list binds
  // itself is only bound once the list body runs, which does not re-enter the runners, so it is
  // not reached through the noted units: besides BeforeSampling()'s checks, every GL_TEXTURE_2D
  // bound while the executed lists, or the lists they call, were compiled is checked and fixed
  // directly. Such a texture sampled through a sampler object is not fixed. `lists` is
  // glCallLists' `lists` array of `size` bytes, or null. While a list is being compiled, only
  // records the called lists instead.
  void BeforeCallList(GLuint list);
  void BeforeCallLists(GLsizei n, GLenum type, const uint8_t* lists, size_t size);

  // Called right after glNewList and glEndList: between them, NoteListTexture() and
  // BeforeCallList(s) record what the list binds and calls, replacing what an earlier
  // compilation of the same list recorded.
  void BeginListCompile(GLuint list);
  void EndListCompile();

  // Called from the GL_TEXTURE_2D bind runners after their real bind call: while a list is being
  // compiled, records `texture` as bound by it.
  void NoteListTexture(GLuint texture);

  // Called right after the real call of every sampling command but glBegin: restores the filters
  // BeforeSampling() overrode.
  void AfterSampling();

  // glBegin/glEnd bracket a block in which state queries and changes are errors, but glCallList
  // may still be called: overrides made before glBegin stay until glEnd, and nothing is checked
  // or changed in between.
  void EnterBeginEnd();
  void LeaveBeginEnd();

private:
  MipmapCompletionService() = default;

  struct FilterOverride {
    bool IsSampler{};
    GLuint Name{};
    GLenum Unit{}; // GL_TEXTUREn the texture was bound to; unused for a sampler.
    GLint Original{};
    GLint Substitute{};
  };

  struct ListContents {
    std::set<GLuint> Textures;
    std::set<GLuint> Calls;
  };

  void BeforeSamplingLocked();
  // Overrides the filter of the GL_TEXTURE_2D bound on `unit`, the active unit, if it needs it.
  void OverrideBoundTexture2D(GLenum unit);
  // Checks and fixes every texture the lists bind, directly by name.
  void OverrideListTextures(const std::vector<GLuint>& lists);
  void OverrideListTexture(GLuint texture, GLenum activeUnit);
  void RestoreFilters();

  // Noted units and lists are process-wide rather than per context: the checks read the current
  // context's live state, so a unit or list noted on another context only costs queries that
  // find nothing to fix.
  std::mutex m_Mutex;
  std::set<GLuint> m_Units;
  std::map<GLuint, ListContents> m_Lists;
  std::vector<FilterOverride> m_Overrides;
  bool m_InBeginEnd{};
};

} // namespace gits::opengl2
