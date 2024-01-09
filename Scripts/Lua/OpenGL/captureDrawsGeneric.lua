-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2024 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================


-- ************************************ description ********************************************
-- This scripts primary goal is to dump framebuffers during recording after/before each drawcall
-- that occurs in the user-specified frame range. Additionally, dumping specified frames
-- as well as tracing of drawcall/frame numbers functionality is also available in this script.

-- *************************************** enums ***********************************************

TRACEV = 0
TRACE  = 1
INFO   = 2
WARN   = 3
ERR    = 4
OFF    = 5

-- ***************************************************************
-- ******************** !!!! SETTINGS !!!! ***********************
-- ***************************************************************

isTrace = false

dumpPostDraw = false
dumpPreDraw  = false
dumpDrawsFromFrames = "-"
dumpFrames = "-"
subpathDraws  = "LuaRecorderDraws"
subpathFrames = "LuaRecorderFrames"


dumpDepth   = false
dumpStencil = false


-- ********************* Globals **************************

FrameRange = gits.newBitRange(dumpFrames)
DrawRange = gits.newBitRange(dumpDrawsFromFrames)

-- **************************** Actions **************************

function preDrawAction(name)
  if(isTrace) then
    gits.log(TRACE,"draw: "..gits.getCurrentDraw().." frame: "..gits.getCurrentFrame().." drawInFrame: "..gits.getCurrentDrawInFrame().."  ")
  end
  if (dumpPreDraw and gits.inBitRange(DrawRange, gits.getCurrentFrame())) then
    final_path = gits.getStreamDir().."/"..subpathDraws.."Pre"
    fname = "drawcall-"..gits.getCurrentDraw().."-pre"
    gits.captureDrawBuffer(final_path, fname, false, dumpDepth, dumpStencil)
  end
end

function postDrawAction(name)
  if (dumpPostDraw and gits.inBitRange(DrawRange, gits.getCurrentFrame())) then
    final_path = gits.getStreamDir().."/"..subpathDraws.."Post"
    fname = "drawcall-"..gits.getCurrentDraw().."-post"
    gits.captureDrawBuffer(final_path, fname, false, dumpDepth, dumpStencil)
  end
end

function preEndFrameAction(name)
  if(isTrace) then
    gits.log(TRACE,"Frame end. Total drawcalls: "..gits.getCurrentDraw())
  end
  if (gits.inBitRange(FrameRange, gits.getCurrentFrame())) then
    final_path = gits.getStreamDir().."/"..subpathFrames
    fname = "frame"..gits.getCurrentFrame()
    gits.captureDrawBuffer(final_path, fname, true, false, false)
  end
end

function postEndFrameAction(name)
end

-- ***************** Functions triggered by GITS *********************

function wglSwapBuffers(hdc)
  preEndFrameAction()
  drv.wglSwapBuffers(hdc)
  postEndFrameAction("wglSwapBuffers")
end

function glXSwapBuffers(dpy, surf)
  preEndFrameAction()
  drv.glXSwapBuffers(dpy, surf)
  postEndFrameAction("glXSwapBuffers")
end

function eglSwapBuffers(dpy, surf)
  preEndFrameAction()
  drv.eglSwapBuffers(dpy, surf)
  postEndFrameAction("eglSwapBuffers")
end


-- Drawcalls
function glArrayElement(i)
  preDrawAction("glArrayElement")
  drv.glArrayElement(i)
  postDrawAction("glArrayElement")
end
function glArrayElementEXT(i)
  preDrawAction("glArrayElementEXT")
  drv.glArrayElementEXT(i)
  postDrawAction("glArrayElementEXT")
end
function glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebuffer")
  drv.glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction("glBlitFramebuffer")
end
function glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebufferANGLE")
  drv.glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction("glBlitFramebufferANGLE")
end
function glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebufferEXT")
  drv.glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction("glBlitFramebufferEXT")
end
function glCallList(list)
  preDrawAction("glCallList")
  drv.glCallList(list)
  postDrawAction("glCallList")
end
function glCallLists(n, type, lists)
  preDrawAction("glCallLists")
  drv.glCallLists(n, type, lists)
  postDrawAction("glCallLists")
end
function glClear(mask)
  preDrawAction("glClear")
  drv.glClear(mask)
  postDrawAction("glClear")
end
function glClearBufferfi(buffer, drawbuffer, depth, stencil)
  preDrawAction("glClearBufferfi")
  drv.glClearBufferfi(buffer, drawbuffer, depth, stencil)
  postDrawAction("glClearBufferfi")
end
function glClearBufferfv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferfv")
  drv.glClearBufferfv(buffer, drawbuffer, value)
  postDrawAction("glClearBufferfv")
end
function glClearBufferiv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferiv")
  drv.glClearBufferiv(buffer, drawbuffer, value)
  postDrawAction("glClearBufferiv")
end
function glClearBufferuiv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferuiv")
  drv.glClearBufferuiv(buffer, drawbuffer, value)
  postDrawAction("glClearBufferuiv")
end
function glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  preDrawAction("glCopyBufferSubData")
  drv.glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  postDrawAction("glCopyBufferSubData")
end
function glCopyColorSubTable(target, start, x, y, width)
  preDrawAction("glCopyColorSubTable")
  drv.glCopyColorSubTable(target, start, x, y, width)
  postDrawAction("glCopyColorSubTable")
end
function glCopyColorSubTableEXT(target, start, x, y, width)
  preDrawAction("glCopyColorSubTableEXT")
  drv.glCopyColorSubTableEXT(target, start, x, y, width)
  postDrawAction("glCopyColorSubTableEXT")
end
function glCopyColorTable(target, internalformat, x, y, width)
  preDrawAction("glCopyColorTable")
  drv.glCopyColorTable(target, internalformat, x, y, width)
  postDrawAction("glCopyColorTable")
end
function glCopyColorTableSGI(target, internalformat, x, y, width)
  preDrawAction("glCopyColorTableSGI")
  drv.glCopyColorTableSGI(target, internalformat, x, y, width)
  postDrawAction("glCopyColorTableSGI")
end
function glCopyConvolutionFilter1D(target, internalformat, x, y, width)
  preDrawAction("glCopyConvolutionFilter")
  drv.glCopyConvolutionFilter1D(target, internalformat, x, y, width)
  postDrawAction("glCopyConvolutionFilter")
end
function glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width)
  preDrawAction("glCopyConvolutionFilter")
  drv.glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width)
  postDrawAction("glCopyConvolutionFilter")
end
function glCopyConvolutionFilter2D(target, internalformat, x, y, width, height)
  preDrawAction("glCopyConvolutionFilter")
  drv.glCopyConvolutionFilter2D(target, internalformat, x, y, width, height)
  postDrawAction("glCopyConvolutionFilter")
end
function glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height)
  preDrawAction("glCopyConvolutionFilter")
  drv.glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height)
  postDrawAction("glCopyConvolutionFilter")
end
function glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  preDrawAction("glCopyImageSubData")
  drv.glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  postDrawAction("glCopyImageSubData")
end
function glCopyImageSubDataNV(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  preDrawAction("glCopyImageSubDataNV")
  drv.glCopyImageSubDataNV(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  postDrawAction("glCopyImageSubDataNV")
end
function glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border)
  preDrawAction("glCopyMultiTexImage")
  drv.glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border)
  postDrawAction("glCopyMultiTexImage")
end
function glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border)
  preDrawAction("glCopyMultiTexImage")
  drv.glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border)
  postDrawAction("glCopyMultiTexImage")
end
function glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width)
  preDrawAction("glCopyMultiTexSubImage")
  drv.glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width)
  postDrawAction("glCopyMultiTexSubImage")
end
function glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyMultiTexSubImage")
  drv.glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction("glCopyMultiTexSubImage")
end
function glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyMultiTexSubImage")
  drv.glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction("glCopyMultiTexSubImage")
end
function glCopyPathNV(resultPath, srcPath)
  preDrawAction("glCopyPathNV")
  drv.glCopyPathNV(resultPath, srcPath)
  postDrawAction("glCopyPathNV")
end
function glCopyPixels(x, y, width, height, type)
  preDrawAction("glCopyPixels")
  drv.glCopyPixels(x, y, width, height, type)
  postDrawAction("glCopyPixels")
end
function glCopyTexImage1D(target, level, internalFormat, x, y, width, border)
  preDrawAction("glCopyTexImage")
  drv.glCopyTexImage1D(target, level, internalFormat, x, y, width, border)
  postDrawAction("glCopyTexImage")
end
function glCopyTexImage1DEXT(target, level, internalFormat, x, y, width, border)
  preDrawAction("glCopyTexImage")
  drv.glCopyTexImage1DEXT(target, level, internalFormat, x, y, width, border)
  postDrawAction("glCopyTexImage")
end
function glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border)
  preDrawAction("glCopyTexImage")
  drv.glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border)
  postDrawAction("glCopyTexImage")
end
function glCopyTexImage2DEXT(target, level, internalFormat, x, y, width, height, border)
  preDrawAction("glCopyTexImage")
  drv.glCopyTexImage2DEXT(target, level, internalFormat, x, y, width, height, border)
  postDrawAction("glCopyTexImage")
end
function glCopyTexSubImage1D(target, level, xoffset, x, y, width)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage1D(target, level, xoffset, x, y, width)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage")
  drv.glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction("glCopyTexSubImage")
end
function glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border)
  preDrawAction("glCopyTextureImage")
  drv.glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border)
  postDrawAction("glCopyTextureImage")
end
function glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border)
  preDrawAction("glCopyTextureImage")
  drv.glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border)
  postDrawAction("glCopyTextureImage")
end
function glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width)
  preDrawAction("glCopyTextureSubImage")
  drv.glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width)
  postDrawAction("glCopyTextureSubImage")
end
function glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTextureSubImage")
  drv.glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction("glCopyTextureSubImage")
end
function glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTextureSubImage")
  drv.glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction("glCopyTextureSubImage")
end
function glDispatchCompute(num_groups_x, num_groups_y, num_groups_z)
  preDrawAction("glDispatchCompute")
  drv.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z)
  postDrawAction("glDispatchCompute")
end
function glDispatchComputeIndirect(indirect)
  preDrawAction("glDispatchComputeIndirect")
  drv.glDispatchComputeIndirect(indirect)
  postDrawAction("glDispatchComputeIndirect")
end
function glDrawArrays(mode, first, count)
  preDrawAction("glDrawArrays")
  drv.glDrawArrays(mode, first, count)
  postDrawAction("glDrawArrays")
end
function glDrawArraysEXT(mode, first, count)
  preDrawAction("glDrawArraysEXT")
  drv.glDrawArraysEXT(mode, first, count)
  postDrawAction("glDrawArraysEXT")
end
function glDrawArraysIndirect(mode, indirect)
  preDrawAction("glDrawArraysIndirect")
  drv.glDrawArraysIndirect(mode, indirect)
  postDrawAction("glDrawArraysIndirect")
end
function glDrawArraysInstanced(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstanced")
  drv.glDrawArraysInstanced(mode, first, count, instancecount)
  postDrawAction("glDrawArraysInstanced")
end
function glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedANGLE")
  drv.glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  postDrawAction("glDrawArraysInstancedANGLE")
end
function glDrawArraysInstancedARB(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedARB")
  drv.glDrawArraysInstancedARB(mode, first, count, instancecount)
  postDrawAction("glDrawArraysInstancedARB")
end
function glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  preDrawAction("glDrawArraysInstancedBaseInstance")
  drv.glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  postDrawAction("glDrawArraysInstancedBaseInstance")
end
function glDrawArraysInstancedEXT(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedEXT")
  drv.glDrawArraysInstancedEXT(mode, first, count, instancecount)
  postDrawAction("glDrawArraysInstancedEXT")
end
function glDrawElements(mode, count, type, indices)
  preDrawAction("glDrawElements")
  drv.glDrawElements(mode, count, type, indices)
  postDrawAction("glDrawElements")
end
function glDrawElementsBaseVertex(mode, count, type, indices, basevertex)
  preDrawAction("glDrawElementsBaseVertex")
  drv.glDrawElementsBaseVertex(mode, count, type, indices, basevertex)
  postDrawAction("glDrawElementsBaseVertex")
end
function glDrawElementsIndirect(mode, type, indirect)
  preDrawAction("glDrawElementsIndirect")
  drv.glDrawElementsIndirect(mode, type, indirect)
  postDrawAction("glDrawElementsIndirect")
end
function glDrawElementsInstanced(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstanced")
  drv.glDrawElementsInstanced(mode, count, type, indices, instancecount)
  postDrawAction("glDrawElementsInstanced")
end
function glDrawElementsInstancedANGLE(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedANGLE")
  drv.glDrawElementsInstancedANGLE(mode, count, type, indices, instancecount)
  postDrawAction("glDrawElementsInstancedANGLE")
end
function glDrawElementsInstancedARB(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedARB")
  drv.glDrawElementsInstancedARB(mode, count, type, indices, instancecount)
  postDrawAction("glDrawElementsInstancedARB")
end
function glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance)
  preDrawAction("glDrawElementsInstancedBaseInstance")
  drv.glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance)
  postDrawAction("glDrawElementsInstancedBaseInstance")
end
function glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex)
  preDrawAction("glDrawElementsInstancedBaseVertex")
  drv.glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex)
  postDrawAction("glDrawElementsInstancedBaseVertex")
end
function glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance)
  preDrawAction("glDrawElementsInstancedBaseVertexBaseInstance")
  drv.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance)
  postDrawAction("glDrawElementsInstancedBaseVertexBaseInstance")
end
function glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedEXT")
  drv.glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
  postDrawAction("glDrawElementsInstancedEXT")
end
function glDrawPixels(width, height, format, type, pixels)
  preDrawAction("glDrawPixels")
  drv.glDrawPixels(width, height, format, type, pixels)
  postDrawAction("glDrawPixels")
end
function glDrawRangeElements(mode, start, end_, count, type, indices)
  preDrawAction("glDrawRangeElements")
  drv.glDrawRangeElements(mode, start, end_, count, type, indices)
  postDrawAction("glDrawRangeElements")
end
function glDrawRangeElementsBaseVertex(mode, start, end_, count, type, indices, basevertex)
  preDrawAction("glDrawRangeElementsBaseVertex")
  drv.glDrawRangeElementsBaseVertex(mode, start, end_, count, type, indices, basevertex)
  postDrawAction("glDrawRangeElementsBaseVertex")
end
function glDrawRangeElementsEXT(mode, start, end_, count, type, indices)
  preDrawAction("glDrawRangeElementsEXT")
  drv.glDrawRangeElementsEXT(mode, start, end_, count, type, indices)
  postDrawAction("glDrawRangeElementsEXT")
end
function glDrawTexfOES(x, y, z, width, height)
  preDrawAction("glDrawTexfOES")
  drv.glDrawTexfOES(x, y, z, width, height)
  postDrawAction("glDrawTexfOES")
end
function glDrawTexfvOES(coords)
  preDrawAction("glDrawTexfvOES")
  drv.glDrawTexfvOES(coords)
  postDrawAction("glDrawTexfvOES")
end
function glDrawTexiOES(x, y, z, width, height)
  preDrawAction("glDrawTexiOES")
  drv.glDrawTexiOES(x, y, z, width, height)
  postDrawAction("glDrawTexiOES")
end
function glDrawTexivOES(coords)
  preDrawAction("glDrawTexivOES")
  drv.glDrawTexivOES(coords)
  postDrawAction("glDrawTexivOES")
end
function glDrawTexsOES(x, y, z, width, height)
  preDrawAction("glDrawTexsOES")
  drv.glDrawTexsOES(x, y, z, width, height)
  postDrawAction("glDrawTexsOES")
end
function glDrawTexsvOES(coords)
  preDrawAction("glDrawTexsvOES")
  drv.glDrawTexsvOES(coords)
  postDrawAction("glDrawTexsvOES")
end
function glDrawTexxOES(x, y, z, width, height)
  preDrawAction("glDrawTexxOES")
  drv.glDrawTexxOES(x, y, z, width, height)
  postDrawAction("glDrawTexxOES")
end
function glDrawTexxvOES(coords)
  preDrawAction("glDrawTexxvOES")
  drv.glDrawTexxvOES(coords)
  postDrawAction("glDrawTexxvOES")
end
function glDrawTransformFeedback(mode, id)
  preDrawAction("glDrawTransformFeedback")
  drv.glDrawTransformFeedback(mode, id)
  postDrawAction("glDrawTransformFeedback")
end
function glDrawTransformFeedbackInstanced(mode, id, instancecount)
  preDrawAction("glDrawTransformFeedbackInstanced")
  drv.glDrawTransformFeedbackInstanced(mode, id, instancecount)
  postDrawAction("glDrawTransformFeedbackInstanced")
end
function glDrawTransformFeedbackNV(mode, id)
  preDrawAction("glDrawTransformFeedbackNV")
  drv.glDrawTransformFeedbackNV(mode, id)
  postDrawAction("glDrawTransformFeedbackNV")
end
function glDrawTransformFeedbackStream(mode, id, stream)
  preDrawAction("glDrawTransformFeedbackStream")
  drv.glDrawTransformFeedbackStream(mode, id, stream)
  postDrawAction("glDrawTransformFeedbackStream")
end
function glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  preDrawAction("glDrawTransformFeedbackStreamInstanced")
  drv.glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  postDrawAction("glDrawTransformFeedbackStreamInstanced")
end
function glMultiDrawArrays(mode, first, count, drawcount)
  preDrawAction("glMultiDrawArrays")
  drv.glMultiDrawArrays(mode, first, count, drawcount)
  postDrawAction("glMultiDrawArrays")
end
function glMultiDrawArraysEXT(mode, first, count, drawcount)
  preDrawAction("glMultiDrawArraysEXT")
  drv.glMultiDrawArraysEXT(mode, first, count, drawcount)
  postDrawAction("glMultiDrawArraysEXT")
end
function glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  preDrawAction("glMultiDrawArraysIndirect")
  drv.glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  postDrawAction("glMultiDrawArraysIndirect")
end
function glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  preDrawAction("glMultiDrawArraysIndirectAMD")
  drv.glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  postDrawAction("glMultiDrawArraysIndirectAMD")
end
function glMultiDrawElements(mode, count, type, indices, drawcount)
  preDrawAction("glMultiDrawElements")
  drv.glMultiDrawElements(mode, count, type, indices, drawcount)
  postDrawAction("glMultiDrawElements")
end
function glMultiDrawElementsEXT(mode, count, type, indices, drawcount)
  preDrawAction("glMultiDrawElementsEXT")
  drv.glMultiDrawElementsEXT(mode, count, type, indices, drawcount)
  postDrawAction("glMultiDrawElementsEXT")
end
function glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride)
  preDrawAction("glMultiDrawElementsIndirect")
  drv.glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride)
  postDrawAction("glMultiDrawElementsIndirect")
end
function glMultiDrawElementsIndirectAMD(mode, type, indirect, drawcount, stride)
  preDrawAction("glMultiDrawElementsIndirectAMD")
  drv.glMultiDrawElementsIndirectAMD(mode, type, indirect, drawcount, stride)
  postDrawAction("glMultiDrawElementsIndirectAMD")
end
function glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size)
  preDrawAction("glNamedCopyBufferSubDataEXT")
  drv.glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size)
  postDrawAction("glNamedCopyBufferSubDataEXT")
end
function glApplyFramebufferAttachmentCMAAINTEL()
  preDrawAction("glApplyFramebufferAttachmentCMAAINTEL")
  drv.glApplyFramebufferAttachmentCMAAINTEL()
  postDrawAction("glApplyFramebufferAttachmentCMAAINTEL")
end
function glBegin(mode)
  preDrawAction("glBegin-glEnd")
  drv.glBegin(mode)
end
function glEnd()
  drv.glEnd()
  postDrawAction("glBegin-glEnd")
end
