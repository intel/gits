-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================


-- Script forcing wire frame view by calling "glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)" before every drawing API.
-- Script is big OpenGL exclusive

-- GL enums
GL_LINE = 0x1B01
GL_FRONT_AND_BACK = 0x0408

-- Pre draw function
function preAction()
  drv.glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
end

-- Post draw function
function postAction()
end


-- Drawcall functions definitions
function glCallList(list)
  preAction()
  drv.glCallList(list)
  postAction()
end
function glCallLists(n, type_, lists)
  preAction()
  drv.glCallLists(n, type_, lists)
  postAction()
end
function glDrawArrays(mode, first, count)
  preAction()
  drv.glDrawArrays(mode, first, count)
  postAction()
end
function glDrawArraysEXT(mode, first, count)
  preAction()
  drv.glDrawArraysEXT(mode, first, count)
  postAction()
end
function glDrawArraysIndirect(mode, indirect)
  preAction()
  drv.glDrawArraysIndirect(mode, indirect)
  postAction()
end
function glDrawArraysInstanced(mode, first, count, instancecount)
  preAction()
  drv.glDrawArraysInstanced(mode, first, count, instancecount)
  postAction()
end
function glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  preAction()
  drv.glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  postAction()
end
function glDrawArraysInstancedARB(mode, first, count, instancecount)
  preAction()
  drv.glDrawArraysInstancedARB(mode, first, count, instancecount)
  postAction()
end
function glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  preAction()
  drv.glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  postAction()
end
function glDrawArraysInstancedEXT(mode, first, count, instancecount)
  preAction()
  drv.glDrawArraysInstancedEXT(mode, first, count, instancecount)
  postAction()
end
function glDrawElements(mode, count, type_, indices)
  preAction()
  drv.glDrawElements(mode, count, type_, indices)
  postAction()
end
function glDrawElementsBaseVertex(mode, count, type_, indices, basevertex)
  preAction()
  drv.glDrawElementsBaseVertex(mode, count, type_, indices, basevertex)
  postAction()
end
function glDrawElementsIndirect(mode, type_, indirect)
  preAction()
  drv.glDrawElementsIndirect(mode, type_, indirect)
  postAction()
end
function glDrawElementsInstanced(mode, count, type_, indices, instancecount)
  preAction()
  drv.glDrawElementsInstanced(mode, count, type_, indices, instancecount)
  postAction()
end
function glDrawElementsInstancedANGLE(mode, count, type_, indices, instancecount)
  preAction()
  drv.glDrawElementsInstancedANGLE(mode, count, type_, indices, instancecount)
  postAction()
end
function glDrawElementsInstancedARB(mode, count, type_, indices, instancecount)
  preAction()
  drv.glDrawElementsInstancedARB(mode, count, type_, indices, instancecount)
  postAction()
end
function glDrawElementsInstancedBaseInstance(mode, count, type_, indices, instancecount, baseinstance)
  preAction()
  drv.glDrawElementsInstancedBaseInstance(mode, count, type_, indices, instancecount, baseinstance)
  postAction()
end
function glDrawElementsInstancedBaseVertex(mode, count, type_, indices, instancecount, basevertex)
  preAction()
  drv.glDrawElementsInstancedBaseVertex(mode, count, type_, indices, instancecount, basevertex)
  postAction()
end
function glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type_, indices, instancecount, basevertex, baseinstance)
  preAction()
  drv.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type_, indices, instancecount, basevertex, baseinstance)
  postAction()
end
function glDrawElementsInstancedEXT(mode, count, type_, indices, instancecount)
  preAction()
  drv.glDrawElementsInstancedEXT(mode, count, type_, indices, instancecount)
  postAction()
end
function glDrawRangeElements(mode, start, end_, count, type_, indices)
  preAction()
  drv.glDrawRangeElements(mode, start, end_, count, type_, indices)
  postAction()
end
function glDrawRangeElementsBaseVertex(mode, start, end_, count, type_, indices, basevertex)
  preAction()
  drv.glDrawRangeElementsBaseVertex(mode, start, end_, count, type_, indices, basevertex)
  postAction()
end
function glDrawRangeElementsEXT(mode, start, end_, count, type_, indices)
  preAction()
  drv.glDrawRangeElementsEXT(mode, start, end_, count, type_, indices)
  postAction()
end
function glDrawTexfOES(x, y, z, width, height)
  preAction()
  drv.glDrawTexfOES(x, y, z, width, height)
  postAction()
end
function glDrawTexfvOES(coords)
  preAction()
  drv.glDrawTexfvOES(coords)
  postAction()
end
function glDrawTexiOES(x, y, z, width, height)
  preAction()
  drv.glDrawTexiOES(x, y, z, width, height)
  postAction()
end
function glDrawTexivOES(coords)
  preAction()
  drv.glDrawTexivOES(coords)
  postAction()
end
function glDrawTexsOES(x, y, z, width, height)
  preAction()
  drv.glDrawTexsOES(x, y, z, width, height)
  postAction()
end
function glDrawTexsvOES(coords)
  preAction()
  drv.glDrawTexsvOES(coords)
  postAction()
end
function glDrawTexxOES(x, y, z, width, height)
  preAction()
  drv.glDrawTexxOES(x, y, z, width, height)
  postAction()
end
function glDrawTexxvOES(coords)
  preAction()
  drv.glDrawTexxvOES(coords)
  postAction()
end
function glDrawTransformFeedback(mode, id)
  preAction()
  drv.glDrawTransformFeedback(mode, id)
  postAction()
end
function glDrawTransformFeedbackInstanced(mode, id, instancecount)
  preAction()
  drv.glDrawTransformFeedbackInstanced(mode, id, instancecount)
  postAction()
end
function glDrawTransformFeedbackNV(mode, id)
  preAction()
  drv.glDrawTransformFeedbackNV(mode, id)
  postAction()
end
function glDrawTransformFeedbackStream(mode, id, stream)
  preAction()
  drv.glDrawTransformFeedbackStream(mode, id, stream)
  postAction()
end
function glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  preAction()
  drv.glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  postAction()
end
function glMultiDrawArrays(mode, first, count, drawcount)
  preAction()
  drv.glMultiDrawArrays(mode, first, count, drawcount)
  postAction()
end
function glMultiDrawArraysEXT(mode, first, count, drawcount)
  preAction()
  drv.glMultiDrawArraysEXT(mode, first, count, drawcount)
  postAction()
end
function glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  preAction()
  drv.glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  postAction()
end
function glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  preAction()
  drv.glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  postAction()
end
function glMultiDrawElements(mode, count, type_, indices, drawcount)
  preAction()
  drv.glMultiDrawElements(mode, count, type_, indices, drawcount)
  postAction()
end
function glMultiDrawElementsEXT(mode, count, type_, indices, drawcount)
  preAction()
  drv.glMultiDrawElementsEXT(mode, count, type_, indices, drawcount)
  postAction()
end
function glMultiDrawElementsIndirect(mode, type_, indirect, drawcount, stride)
  preAction()
  drv.glMultiDrawElementsIndirect(mode, type_, indirect, drawcount, stride)
  postAction()
end
function glMultiDrawElementsIndirectAMD(mode, type_, indirect, drawcount, stride)
  preAction()
  drv.glMultiDrawElementsIndirectAMD(mode, type_, indirect, drawcount, stride)
  postAction()
end
function glBegin(mode)
  preAction()
  drv.glBegin(mode)
end
function glEnd()
  drv.glEnd()
  postAction()
end
