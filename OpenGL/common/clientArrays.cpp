// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "clientArrays.h"

/* ************************* ClientArraysUpdate *********************************** */
gits::OpenGL::ClientArraysUpdate::ClientArraysUpdate(GLuint index) {
  // VAO check
  GLint vaoBinding = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
  if (vaoBinding > 0) {
    return;
  }

  DumpAttribsUpdate(index, index, 0, 0);
}

gits::OpenGL::ClientArraysUpdate::ClientArraysUpdate(GLuint start,
                                                     GLuint count,
                                                     GLuint instances,
                                                     GLuint baseinstance) {
  if (count == 0) {
    return;
  }

  // VAO check
  GLint vaoBinding = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
  if (vaoBinding > 0) {
    return;
  }

  DumpAttribsUpdate(start, start + count - 1, instances, baseinstance);
}

gits::OpenGL::ClientArraysUpdate::ClientArraysUpdate(const GLsizei start[],
                                                     const GLsizei count[],
                                                     GLuint primcount) {
  // VAO check
  GLint vaoBinding = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
  if (vaoBinding > 0) {
    return;
  }

  for (unsigned i = 0; i < primcount; i++) {
    if (count[i] > 0) {
      DumpAttribsUpdate(start[i], start[i] + count[i] - 1, 0, 0);
    }
  }
}

gits::OpenGL::ClientArraysUpdate::ClientArraysUpdate(GLenum type,
                                                     const GLsizei* count,
                                                     const void* const* indices,
                                                     GLuint primcount) {
  // VAO check
  GLint vaoBinding = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
  if (vaoBinding > 0) {
    return;
  }

  // Indices buffer binding check
  if (CGits::Instance().IsStateRestoration()) {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  GLint buff = 0;
  drv.gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buff);

  // Return if indices and all attribs stored in buffers
  bool clientAttribs = CheckClientAttribs();
  if (buff != 0 && !clientAttribs) {
    return;
  }

  // Dump indices and get sorted list of indices
  boost::container::flat_set<GLuint> indicesSorted;

  int sum_count = 0;
  for (unsigned i = 0; i < primcount; i++) {
    sum_count += count[i];
  }
  indicesSorted.reserve(sum_count);

  for (unsigned i = 0; i < primcount; i++) {
    if (count[i] > 0) {
      DumpIndicesUpdate(buff, type, count[i], indices[i], 0, indicesSorted);
    }
  }

  // return if attribs stored in buffers
  if (!clientAttribs) {
    return;
  }

  // Dump attribs updates in optimized ranges
  DumpAttribsUpdateOptimized(std::move(indicesSorted), 0, 0);
}

gits::OpenGL::ClientArraysUpdate::ClientArraysUpdate(GLsizei count,
                                                     GLenum type,
                                                     const GLvoid* indices,
                                                     GLuint instances,
                                                     GLuint baseinstance,
                                                     GLuint basevertex) {
  if (count == 0) {
    return;
  }

  // VAO check
  GLint vaoBinding = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
  if (vaoBinding > 0) {
    return;
  }

  // Indices buffer binding check
  if (CGits::Instance().IsStateRestoration()) {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  GLint buff = 0;
  drv.gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buff);

  // Return if indices and all attribs stored in buffers
  bool clientAttribs = CheckClientAttribs();
  if (buff != 0 && !clientAttribs) {
    return;
  }

  // Dump indices and get sorted list of indices
  boost::container::flat_set<GLuint> indicesSorted;

  indicesSorted.reserve(count);

  DumpIndicesUpdate(buff, type, count, indices, basevertex, indicesSorted);

  // return if attribs stored in buffers
  if (!clientAttribs) {
    return;
  }

  // Dump attribs updates in optimized ranges
  DumpAttribsUpdateOptimized(std::move(indicesSorted), instances, baseinstance);
}

void gits::OpenGL::ClientArraysUpdate::DumpIndicesUpdate(
    GLuint buff,
    GLenum type,
    GLuint count,
    const GLvoid* indices,
    GLuint basevertex,
    boost::container::flat_set<GLuint>& sorted) {
  // Get pointer to indices data from client side or mapped buffer
  const GLvoid* indicesPtr;
  std::shared_ptr<MapBuffer> buffMap;
  if (buff != 0) {
    buffMap.reset(new MapBuffer(GL_ELEMENT_ARRAY_BUFFER, buff));
    indicesPtr = (GLvoid*)((uintptr_t)buffMap->Data() + (uintptr_t)indices);
  } else {
    indicesPtr = indices;
  }

  // Support for GL_PRIMITIVE_RESTART and STRIP INDEX options
  GLuint stripIndex = Config::Get().recorder.openGL.utilities.stripIndicesValues;
  GLuint restartIndex = 4294967295u;
  if (drv.gl.glIsEnabled(GL_PRIMITIVE_RESTART)) {
    restartIndex = SD().GetCurrentContextStateData().restartIndexValue;
  }

  // Read indices
  uint64_t indicesDataSize;
  switch (type) {
  case GL_UNSIGNED_BYTE: {
    indicesDataSize = sizeof(GLubyte) * count;

    auto indexPtr = (GLubyte*)indicesPtr;
    for (unsigned i = 0; i < count; i++, indexPtr++) {
      GLubyte index = *indexPtr + (GLubyte)basevertex;
      if ((GLubyte)restartIndex == index || (GLubyte)stripIndex == index) {
        continue;
      }
      sorted.insert((GLuint)index);
    }
    break;
  }
  case GL_UNSIGNED_SHORT: {
    indicesDataSize = sizeof(GLushort) * count;

    auto indexPtr = (GLushort*)indicesPtr;
    for (unsigned i = 0; i < count; i++, indexPtr++) {
      GLushort index = *indexPtr + (GLushort)basevertex;
      if ((GLushort)restartIndex == index || (GLushort)stripIndex == index) {
        continue;
      }
      sorted.insert((GLuint)index);
    }
    break;
  }
  case GL_UNSIGNED_INT: {
    indicesDataSize = sizeof(GLuint) * count;

    auto indexPtr = (GLuint*)indicesPtr;
    for (unsigned i = 0; i < count; i++, indexPtr++) {
      GLuint index = *indexPtr + basevertex;
      if ((GLuint)restartIndex == index || (GLuint)stripIndex == index) {
        continue;
      }
      sorted.insert((GLuint)index);
    }
    break;
  }
  default:
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  // Dump indices data diff
  if (buff == 0) {
    _update.Diff((uint64_t)indicesPtr, (uint64_t)indicesPtr, indicesDataSize);
  }
}

// Returns true if any attrib comes from client side
bool gits::OpenGL::ClientArraysUpdate::CheckClientAttribs() {
  auto& cArrays = SD().GetCurrentContextStateData().ClientArrays();
  if (cArrays.VertexArray().Enabled() && !cArrays.VertexArray().IsBuffArray()) {
    return true;
  }
  if (cArrays.ColorArray().Enabled() && !cArrays.ColorArray().IsBuffArray()) {
    return true;
  }
  if (cArrays.SecondaryColorArray().Enabled() && !cArrays.SecondaryColorArray().IsBuffArray()) {
    return true;
  }
  if (cArrays.NormalArray().Enabled() && !cArrays.NormalArray().IsBuffArray()) {
    return true;
  }

  for (auto& texAttrib : cArrays.TexCoordArrayMap()) {
    // WA for push attrib
    bool enabled = false;
    CContextStateDynamic& contexState = SD().GetCurrentContextStateData();
    if (contexState.pushUsed && curctx::IsOgl()) {
      enabled = drv.gl.glIsEnabled(GL_TEXTURE_COORD_ARRAY);
    }

    if ((enabled || texAttrib.second.Enabled()) && !texAttrib.second.IsBuffArray()) {
      return true;
    }
  }
  for (auto& genericAttrib : cArrays.VertexAttribArrayMap()) {
    if (genericAttrib.second.Enabled() && !genericAttrib.second.IsBuffArray()) {
      return true;
    }
  }
  return false;
}

namespace {
using namespace gits;
using namespace OpenGL;

// Function used to get memory range from attrib array range pointed by indices
std::pair<uint64_t, uint64_t> GetAttribMemRange(int frontindex,
                                                int backindex,
                                                const CClientArrayStateObj& carraystate) {
  auto& attrParams = carraystate.Data().track;

  // Evaluate data begin ptr
  uint64_t dataBegin;
  if (frontindex == 0) {
    dataBegin = (uint64_t)attrParams.ptr;
  } else {
    if (attrParams.stride > 0) {
      dataBegin = (uint64_t)attrParams.ptr + frontindex * attrParams.stride;
    } else {
      dataBegin =
          (uint64_t)attrParams.ptr + frontindex * attrParams.size * DataTypeSize(attrParams.type);
    }
  }

  // Evaluate data size
  uint64_t dataSize;
  unsigned count = backindex - frontindex + 1;
  if (attrParams.stride > 0) {
    dataSize = attrParams.stride * (count - 1) + attrParams.size * DataTypeSize(attrParams.type);
  } else {
    dataSize = attrParams.size * DataTypeSize(attrParams.type) * count;
  }
  uint64_t dataEnd = dataBegin + dataSize;

  return std::pair<uint64_t, uint64_t>(dataBegin, dataEnd);
}

std::pair<GLuint, GLuint> GetInstancedIndicesRange(GLuint divisor,
                                                   GLuint instances,
                                                   GLuint baseinstance) {
  if (divisor == 0) {
    return std::pair<GLuint, GLuint>(0, 0);
  }
  return std::pair<GLuint, GLuint>(baseinstance, (instances - 1) / divisor + baseinstance);
}
} // namespace

// Dump attribs data memory updates
void gits::OpenGL::ClientArraysUpdate::DumpAttribsUpdate(GLuint frontindex,
                                                         GLuint backindex,
                                                         GLuint instances,
                                                         GLuint baseinstance) {
  auto& cArrays = SD().GetCurrentContextStateData().ClientArrays();
  struct DataRange {
    uint64_t attrPtr;
    uint64_t dataBegin;
    uint64_t dataEnd;
    DataRange(uint64_t attrptr, uint64_t databegin, uint64_t dataend)
        : attrPtr(attrptr), dataBegin(databegin), dataEnd(dataend) {}
  };
  std::list<DataRange> memRanges; // update beginPtr, endPtr

  // Get attribs memory ranges

  auto& interleavedArray = cArrays.InterleavedArray();
  // Interleaved attributes
  if (interleavedArray.Enabled()) {
    if (interleavedArray.Enabled() && !interleavedArray.IsBuffArray()) {
      auto memRange = GetAttribMemRange(frontindex, backindex, interleavedArray);
      memRanges.push_back(
          DataRange((uint64_t)interleavedArray.Pointer(), memRange.first, memRange.second));
    }
  } else { // Non interleaved attributes
    {
      auto& vertexAttrib = cArrays.VertexArray();
      if (vertexAttrib.Enabled() && !vertexAttrib.IsBuffArray()) {
        auto memRange = GetAttribMemRange(frontindex, backindex, vertexAttrib);
        memRanges.push_back(
            DataRange((uint64_t)vertexAttrib.Pointer(), memRange.first, memRange.second));
      }
    }
    {
      auto& colorAttrib = cArrays.ColorArray();
      if (colorAttrib.Enabled() && !colorAttrib.IsBuffArray()) {
        auto memRange = GetAttribMemRange(frontindex, backindex, colorAttrib);
        memRanges.push_back(
            DataRange((uint64_t)colorAttrib.Pointer(), memRange.first, memRange.second));
      }
    }
    {
      auto& secondColorAttrib = cArrays.SecondaryColorArray();
      if (secondColorAttrib.Enabled() && !secondColorAttrib.IsBuffArray()) {
        auto memRange = GetAttribMemRange(frontindex, backindex, secondColorAttrib);
        memRanges.push_back(
            DataRange((uint64_t)secondColorAttrib.Pointer(), memRange.first, memRange.second));
      }
    }
    {
      auto& normalAttrib = cArrays.NormalArray();
      if (normalAttrib.Enabled() && !normalAttrib.IsBuffArray()) {
        auto memRange = GetAttribMemRange(frontindex, backindex, normalAttrib);
        memRanges.push_back(
            DataRange((uint64_t)normalAttrib.Pointer(), memRange.first, memRange.second));
      }
    }

    for (auto& texAttrib : cArrays.TexCoordArrayMap()) {
      // WA for push attrib
      bool enabled = false;
      CContextStateDynamic& contexState = SD().GetCurrentContextStateData();
      if (contexState.pushUsed && curctx::IsOgl()) {
        GLint clientActiveTexUnit = 0;
        drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexUnit);
        drv.gl.glClientActiveTexture(texAttrib.first);
        enabled = drv.gl.glIsEnabled(GL_TEXTURE_COORD_ARRAY);
        drv.gl.glClientActiveTexture(clientActiveTexUnit);
      }

      if ((enabled || texAttrib.second.Enabled()) && !texAttrib.second.IsBuffArray()) {
        auto memRange = GetAttribMemRange(frontindex, backindex, texAttrib.second);
        memRanges.push_back(
            DataRange((uint64_t)texAttrib.second.Pointer(), memRange.first, memRange.second));
      }
    }

    for (auto& genericAttrib : cArrays.VertexAttribArrayMap()) {
      if (genericAttrib.second.Enabled() && !genericAttrib.second.IsBuffArray()) {

        std::pair<uint64_t, uint64_t> memRange;
        const GLuint divisor = genericAttrib.second.Data().track.divisor;
        if (divisor == 0) {
          // Non instanced attributes
          memRange = GetAttribMemRange(frontindex, backindex, genericAttrib.second);
        } else {
          // Instanced attributes
          auto instIndicesRange = GetInstancedIndicesRange(divisor, instances, baseinstance);
          memRange = GetAttribMemRange(instIndicesRange.first, instIndicesRange.second,
                                       genericAttrib.second);
        }

        memRanges.push_back(
            DataRange((uint64_t)genericAttrib.second.Pointer(), memRange.first, memRange.second));
      }
    }
  }

  // Optimize memory ranges (melt interfering ranges)
  for (auto range = memRanges.begin(); range != memRanges.end(); range++) {
    uint64_t& attrptr = range->attrPtr;
    uint64_t& begin = range->dataBegin;
    uint64_t& end = range->dataEnd;
    auto cmprange = range;
    cmprange++;
    while (cmprange != memRanges.end()) {
      const uint64_t cmpattrptr = cmprange->attrPtr;
      const uint64_t cmpbegin = cmprange->dataBegin;
      const uint64_t cmpend = cmprange->dataEnd;
      // (GetAreaPtr(attrptr) == GetAreaPtr(cmpattrptr) - This if is looking for
      // attribs with pointer at the same area (page) and interfering data range
      if (GetAreaPtr(attrptr) == GetAreaPtr(cmpattrptr) &&
          ((begin <= cmpbegin && cmpbegin <= end) || (begin <= cmpend && cmpend <= end) ||
           (cmpbegin <= begin && begin <= cmpend) || (cmpbegin <= end && end <= cmpend))) {
        if (cmpbegin < begin) {
          begin = cmpbegin;
        }
        if (cmpend > end) {
          end = cmpend;
        }
        if (cmpattrptr < attrptr) {
          attrptr = cmpattrptr;
        }

        auto itercpy = cmprange;
        cmprange++;
        memRanges.erase(itercpy, itercpy);
      } else {
        cmprange++;
      }
    }
  }

  // Create diffs
  for (auto& range : memRanges) {
    _update.Diff(range.attrPtr, range.dataBegin, range.dataEnd - range.dataBegin);
  }
}

// Dump attribs data memory updates optimized
void gits::OpenGL::ClientArraysUpdate::DumpAttribsUpdateOptimized(
    boost::container::flat_set<GLuint> indices, GLuint instances, GLuint baseinstance) {
  // Get continuous indices ranges and dump attribs data diff for those ranges
  GLuint rangeBegin = 0;
  GLuint rangeEnd = 0;
  for (auto index = indices.begin(); index != indices.end(); index++) {
    if (index == indices.begin()) {
      rangeBegin = *index;
      rangeEnd = *index;
    } else {
      // Check range end - dump indices data
      if (rangeEnd < *index - 1) {
        DumpAttribsUpdate(rangeBegin, rangeEnd, instances, baseinstance);
        rangeBegin = *index;
      }

      // Check indices end - dump attribs data
      if (*index == *indices.rbegin()) {
        DumpAttribsUpdate(rangeBegin, *index, instances, baseinstance);
      }

      rangeEnd = *index;
    }
  }
}
