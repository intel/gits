-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================


-- Script removin all draw calls from the stream

-- Drawcall functions definitions

function glArrayElement(i)
end

function glArrayElementEXT(i)
end

function glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
end

function glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
end

function glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
end

function glCallList(list)
end

function glCallLists(n, type_, lists)
end

function glClear(mask)
end

function glClearBufferfv(buffer, drawbuffer, value)
end

function glClearBufferiv(buffer, drawbuffer, value)
end

function glClearBufferuiv(buffer, drawbuffer, value)
end

function glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
end

function glCopyColorSubTable(target, start, x, y, width)
end

function glCopyColorSubTableEXT(target, start, x, y, width)
end

function glCopyColorTable(target, internalformat, x, y, width)
end

function glCopyColorTableSGI(target, internalformat, x, y, width)
end

function glCopyConvolutionFilter1D(target, internalformat, x, y, width)
end

function glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width)
end

function glCopyConvolutionFilter2D(target, internalformat, x, y, width, height)
end

function glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height)
end

function glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
end

function glCopyImageSubDataNV(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
end

function glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border)
end

function glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border)
end

function glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width)
end

function glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height)
end

function glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height)
end

function glCopyPathNV(resultPath, srcPath)
end

function glCopyPixels(x, y, width, height, type)
end

function glCopyTexImage1D(target, level, internalFormat, x, y, width, border)
end

function glCopyTexImage1DEXT(target, level, internalFormat, x, y, width, border)
end

function glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border)
end

function glCopyTexImage2DEXT(target, level, internalFormat, x, y, width, height, border)
end

function glCopyTexSubImage1D(target, level, xoffset, x, y, width)
end

function glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width)
end

function glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height)
end

function glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height)
end

function glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height)
end

function glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height)
end

function glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height)
end

function glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border)
end

function glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border)
end

function glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width)
end

function glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height)
end

function glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height)
end

function glDrawArrays(mode, first, count)
end

function glDrawArraysEXT(mode, first, count)
end

function glDrawArraysIndirect(mode, indirect)
end

function glDrawArraysInstanced(mode, first, count, instancecount)
end

function glDrawArraysInstancedANGLE(mode, first, count, instancecount)
end

function glDrawArraysInstancedARB(mode, first, count, instancecount)
end

function glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
end

function glDrawArraysInstancedEXT(mode, first, count, instancecount)
end

function glDrawElements(mode, count, type_, indices)
end

function glDrawElementsBaseVertex(mode, count, type_, indices, basevertex)
end

function glDrawElementsIndirect(mode, type_, indirect)
end

function glDrawElementsInstanced(mode, count, type_, indices, instancecount)
end

function glDrawElementsInstancedANGLE(mode, count, type_, indices, instancecount)
end

function glDrawElementsInstancedARB(mode, count, type_, indices, instancecount)
end

function glDrawElementsInstancedBaseInstance(mode, count, type_, indices, instancecount, baseinstance)
end

function glDrawElementsInstancedBaseVertex(mode, count, type_, indices, instancecount, basevertex)
end

function glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type_, indices, instancecount, basevertex, baseinstance)
end

function glDrawElementsInstancedEXT(mode, count, type_, indices, instancecount)
end

function glDrawPixels(width, height, format, type_, pixels)
end

function glDrawRangeElements(mode, start, end_, count, type_, indices)
end

function glDrawRangeElementsBaseVertex(mode, start, end_, count, type_, indices, basevertex)
end

function glDrawRangeElementsEXT(mode, start, end_, count, type_, indices)
end

function glDrawTexfOES(x, y, z, width, height)
end

function glDrawTexfvOES(coords)
end

function glDrawTexiOES(x, y, z, width, height)
end

function glDrawTexivOES(coords)
end

function glDrawTexsOES(x, y, z, width, height)
end

function glDrawTexsvOES(coords)
end

function glDrawTexxOES(x, y, z, width, height)
end

function glDrawTexxvOES(coords)
end

function glDrawTransformFeedback(mode, id)
end

function glDrawTransformFeedbackInstanced(mode, id, instancecount)
end

function glDrawTransformFeedbackNV(mode, id)
end

function glDrawTransformFeedbackStream(mode, id, stream)
end

function glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
end

function glMultiDrawArrays(mode, first, count, drawcount)
end

function glMultiDrawArraysEXT(mode, first, count, drawcount)
end

function glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
end

function glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
end

function glMultiDrawElements(mode, count, type_, indices, drawcount)
end

function glMultiDrawElementsEXT(mode, count, type_, indices, drawcount)
end

function glMultiDrawElementsIndirect(mode, type_, indirect, drawcount, stride)
end

function glMultiDrawElementsIndirectAMD(mode, type_, indirect, drawcount, stride)
end

function glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size)
end

function glBegin(mode)
end

function glEnd()
end

