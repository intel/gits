// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstddef>

namespace gits::opengl {

// Element counts of DISCARDED_OUTPUT query results that are lists sized by live state, queried
// from the replay driver before the call writes them (pointer_length_derivation.py's
// _DISCARDED_OUTPUT_LIST_SIZERS selects which command uses which). Each returns 0 for a pname
// that is not such a list; those return a fixed count bounded by DiscardedOutputScratch.

// glGet{Boolean,Integer,Integer64,Float,Double,Fixed,UnsignedByte}v list pnames, e.g.
// GL_COMPRESSED_TEXTURE_FORMATS (length GL_NUM_COMPRESSED_TEXTURE_FORMATS).
size_t StateListElements(GLenum pname);

// glGetTexBumpParameter*ATI(GL_BUMP_TEX_UNITS_ATI).
size_t TexBumpListElements(GLenum pname);

// glGetActiveAtomicCounterBufferiv(GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES).
size_t AtomicCounterBufferListElements(GLuint program, GLuint bufferIndex, GLenum pname);

// glGetActiveUniformBlockiv(GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES).
size_t UniformBlockListElements(GLuint program, GLuint uniformBlockIndex, GLenum pname);

// glGetActiveSubroutineUniformiv(GL_COMPATIBLE_SUBROUTINES).
size_t SubroutineUniformListElements(GLuint program, GLenum shadertype, GLuint index, GLenum pname);

} // namespace gits::opengl
