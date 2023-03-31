#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

Bind=1
Create=2
Delete=4
Fill=8
Gen=16
Get=32
Map=64
Param=128
Render=256
Resource=512
Query=1024
Copy=2048
Exec=4096
GetEssential=8192

functions_table = []

def Function(**kwargs):

  #print(kwargs.get('arg1').get('name'))
  #print(kwargs)
  functions_table.append(kwargs)


def ArgDef(**kwargs):
  return kwargs

def RetDef(**kwargs):
  return kwargs

def GetFunctions():
  return functions_table

Function(name='glAccum', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='value', type='GLfloat')
)

Function(name='glAccumxOES', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='value', type='GLfixed')
)

Function(name='glAcquireKeyedMutexWin32EXT', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='memory', type='GLuint'),
arg2=ArgDef(name='key', type='GLuint64'),
arg3=ArgDef(name='timeout', type='GLuint')
)

Function(name='glActiveProgramEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glActiveShaderProgram', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline'),
arg2=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glActiveShaderProgramEXT', enabled=True, type=Param, inheritFrom='glActiveShaderProgram')

Function(name='glActiveStencilFaceEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum')
)

Function(name='glActiveTexture', enabled=True, type=Param, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum')
)

Function(name='glActiveTextureARB', enabled=True, type=Param, inheritFrom='glActiveTexture')

Function(name='glActiveVaryingNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glAddSwapHintRectWIN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei')
)

Function(name='glAlphaFragmentOp1ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMod', type='GLuint'),
arg4=ArgDef(name='arg1', type='GLuint'),
arg5=ArgDef(name='arg1Rep', type='GLuint'),
arg6=ArgDef(name='arg1Mod', type='GLuint')
)

Function(name='glAlphaFragmentOp2ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMod', type='GLuint'),
arg4=ArgDef(name='arg1', type='GLuint'),
arg5=ArgDef(name='arg1Rep', type='GLuint'),
arg6=ArgDef(name='arg1Mod', type='GLuint'),
arg7=ArgDef(name='arg2', type='GLuint'),
arg8=ArgDef(name='arg2Rep', type='GLuint'),
arg9=ArgDef(name='arg2Mod', type='GLuint')
)

Function(name='glAlphaFragmentOp3ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMod', type='GLuint'),
arg4=ArgDef(name='arg1', type='GLuint'),
arg5=ArgDef(name='arg1Rep', type='GLuint'),
arg6=ArgDef(name='arg1Mod', type='GLuint'),
arg7=ArgDef(name='arg2', type='GLuint'),
arg8=ArgDef(name='arg2Rep', type='GLuint'),
arg9=ArgDef(name='arg2Mod', type='GLuint'),
arg10=ArgDef(name='arg3', type='GLuint'),
arg11=ArgDef(name='arg3Rep', type='GLuint'),
arg12=ArgDef(name='arg3Mod', type='GLuint')
)

Function(name='glAlphaFunc', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLfloat')
)

Function(name='glAlphaFuncQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLclampf')
)

Function(name='glAlphaFuncx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLfixed')
)

Function(name='glAlphaFuncxOES', enabled=True, type=Param, inheritFrom='glAlphaFuncx')

Function(name='glAlphaToCoverageDitherControlNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glApplyFramebufferAttachmentCMAAINTEL', enabled=True, type=Fill,
retV=RetDef(type='void')
)

Function(name='glApplyFramebufferAttachmentCmaaINTEL', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target',type='GLenum'),
arg2=ArgDef(name='attachment',type='GLenum')
)

Function(name='glApplyTextureEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glAreProgramsResidentNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='programs', type='const GLuint*'),
arg3=ArgDef(name='residences', type='GLboolean*')
)

Function(name='glAreTexturesResident', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='textures', type='const GLuint*'),
arg3=ArgDef(name='residences', type='GLboolean*')
)

Function(name='glAreTexturesResidentEXT', enabled=False, type=None, inheritFrom='glAreTexturesResident')

Function(name='glArrayElement', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(i)',
retV=RetDef(type='void'),
arg1=ArgDef(name='i', type='GLint')
)

Function(name='glArrayElementEXT', enabled=True, type=Render, inheritFrom='glArrayElement')

Function(name='glArrayObjectATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='buffer', type='GLuint'),
arg6=ArgDef(name='offset', type='GLuint')
)

Function(name='glAsyncMarkerSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='marker', type='GLuint')
)

Function(name='glAttachObjectARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='containerObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glAttachShader', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram')
)

Function(name='glBegin', enabled=True, type=Param, runWrap=True, passToken=True, stateTrack=True, prefix="""  // glBegin/glEnd are special, in that flattened playback in one thread
  // will switch contexts during vertex specification, which causes problems -
  // make current fails (at least on Windows). Here we lock, API mutex,
  // which is recursive, and effectively makes glBegin/glEnd block atomic
  // with respect to GL commands in other threads. This will cause big issues
  // if appliaction fails to match glBegin and glEnd everywhere.
  globalMutex.lock();""",
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glBeginConditionalRender', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glBeginConditionalRenderNV', enabled=True, type=Param, inheritFrom='glBeginConditionalRender')

Function(name='glBeginConditionalRenderNVX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glBeginFragmentShaderATI', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glBeginOcclusionQueryNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glBeginPerfMonitorAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='monitor', type='GLuint')
)

Function(name='glBeginPerfQueryINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryHandle', type='GLuint')
)

Function(name='glBeginQuery', enabled=True, type=Param|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint')
)

Function(name='glBeginQueryARB', enabled=True, type=Param|Query, inheritFrom='glBeginQuery')

Function(name='glBeginQueryEXT', enabled=True, type=Param|Query, inheritFrom='glBeginQuery')

Function(name='glBeginQueryIndexed', enabled=True, type=Param|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='id', type='GLuint')
)

Function(name='glBeginTransformFeedback', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='primitiveMode', type='GLenum')
)

Function(name='glBeginTransformFeedbackEXT', enabled=True, type=Param, inheritFrom='glBeginTransformFeedback')

Function(name='glBeginTransformFeedbackNV', enabled=True, type=Param, inheritFrom='glBeginTransformFeedback')

Function(name='glBeginVertexShaderEXT', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glBeginVideoCaptureNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint')
)

Function(name='glBindAttribLocation', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glBindAttribLocationARB', enabled=True, type=Param, inheritFrom='glBindAttribLocation')

Function(name='glBindBuffer', enabled=True, type=Bind, stateTrack=True, recCond='ConditionBufferES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glBindBufferARB', enabled=True, type=Bind, inheritFrom='glBindBuffer')

Function(name='glBindBufferBase', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glBindBufferBaseEXT', enabled=True, type=Bind, inheritFrom='glBindBufferBase')

Function(name='glBindBufferBaseNV', enabled=True, type=Bind, inheritFrom='glBindBufferBase')

Function(name='glBindBufferOffsetEXT', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glBindBufferOffsetNV', enabled=True, type=Bind, inheritFrom='glBindBufferOffsetEXT')

Function(name='glBindBufferRange', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glBindBufferRangeEXT', enabled=True, type=Bind, inheritFrom='glBindBufferRange')

Function(name='glBindBufferRangeNV', enabled=True, type=Bind, inheritFrom='glBindBufferRange')

Function(name='glBindBuffersBase', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='first', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='buffers', type='const GLuint*', wrapType='CGLBuffer::CSArray', wrapParams='count, buffers')
)

Function(name='glBindBuffersRange', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='first', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='buffers', type='const GLuint*'),
arg5=ArgDef(name='offsets', type='const GLintptr*'),
arg6=ArgDef(name='sizes', type='const GLsizeiptr*')
)

Function(name='glBindFragDataLocation', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='color', type='GLuint'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glBindFragDataLocationEXT', enabled=True, type=Bind, inheritFrom='glBindFragDataLocation')

Function(name='glBindFragDataLocationIndexed', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='colorNumber', type='GLuint'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glBindFragDataLocationIndexedEXT', enabled=False, type=Bind, inheritFrom='glBindFragDataLocationIndexed')

Function(name='glBindFragmentShaderATI', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glBindFramebuffer', enabled=True, type=Bind, stateTrack=True, runWrap=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer')
)

Function(name='glBindFramebufferEXT', enabled=True, type=Bind, stateTrack=True, runWrap=False, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebufferEXT')
)

Function(name='glBindFramebufferOES', enabled=True, type=Bind, inheritFrom='glBindFramebufferEXT', runWrap=False, recWrap=True)

Function(name='glBindImageTexture', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='unit', type='GLuint'),
arg2=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='layered', type='GLboolean'),
arg5=ArgDef(name='layer', type='GLint'),
arg6=ArgDef(name='access', type='GLenum'),
arg7=ArgDef(name='format', type='GLenum')
)

Function(name='glBindImageTextureEXT', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='layered', type='GLboolean'),
arg5=ArgDef(name='layer', type='GLint'),
arg6=ArgDef(name='access', type='GLenum'),
arg7=ArgDef(name='format', type='GLint')
)

Function(name='glBindImageTextures', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='textures', type='const GLuint*')
)

Function(name='glBindLightParameterEXT', enabled=True, type=Bind,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='value', type='GLenum')
)

Function(name='glBindMaterialParameterEXT', enabled=True, type=Bind,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='value', type='GLenum')
)

Function(name='glBindMultiTextureEXT', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glBindParameterEXT', enabled=True, type=Bind,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='value', type='GLenum')
)

Function(name='glBindProgramARB', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='program', type='GLuint')
)

Function(name='glBindProgramNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint')
)

Function(name='glBindProgramPipeline', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline')
)

Function(name='glBindProgramPipelineEXT', enabled=True, type=Bind, inheritFrom='glBindProgramPipeline')

Function(name='glBindRenderbuffer', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glBindRenderbufferEXT', enabled=True, type=Bind, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT')
)

Function(name='glBindRenderbufferOES', enabled=True, type=Bind, inheritFrom='glBindRenderbuffer')

Function(name='glBindSampler', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='unit', type='GLuint'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler')
)

Function(name='glBindSamplers', enabled=True, type=Bind, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='samplers', type='const GLuint*', wrapType='CGLSampler::CSArray', wrapParams='count, samplers')
)

Function(name='glBindTexGenParameterEXT', enabled=True, type=Bind,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='unit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='value', type='GLenum')
)

Function(name='glBindTexture', enabled=True, type=Bind, stateTrack=True, recCond='ConditionTextureES(_recorder)', runCond='ConditionCurrentContextZero()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glBindTextureEXT', enabled=True, type=Bind, inheritFrom='glBindTexture', recCond=False, runCond=False)

Function(name='glBindTextureUnit', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='unit', type='GLuint'),
arg2=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glBindTextureUnitParameterEXT', enabled=True, type=Bind,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='unit', type='GLenum'),
arg2=ArgDef(name='value', type='GLenum')
)

Function(name='glBindTextures', enabled=True, type=Bind, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='textures', type='const GLuint*', wrapType='CGLTexture::CSArray', wrapParams='count, textures')
)

Function(name='glBindTransformFeedback', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint', wrapType='CGLTransformFeedback')
)

Function(name='glBindTransformFeedbackNV', enabled=True, type=Bind, inheritFrom='glBindTransformFeedback')

Function(name='glBindVertexArray', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLuint', wrapType='CGLVertexArray')
)

Function(name='glBindVertexArrayAPPLE', enabled=False, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLuint')
)

Function(name='glBindVertexArrayOES', enabled=True, type=Bind, inheritFrom='glBindVertexArray')

Function(name='glBindVertexBuffer', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='bindingindex', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='offset', type='GLintptr'),
arg4=ArgDef(name='stride', type='GLsizei')
)

Function(name='glBindVertexBuffers', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='buffers', type='const GLuint*'),
arg4=ArgDef(name='offsets', type='const GLintptr*'),
arg5=ArgDef(name='strides', type='const GLsizei*')
)

Function(name='glBindVertexShaderEXT', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glBindVideoCaptureStreamBufferNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='frame_region', type='GLenum'),
arg4=ArgDef(name='offset', type='GLintptrARB')
)

Function(name='glBindVideoCaptureStreamTextureNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='frame_region', type='GLenum'),
arg4=ArgDef(name='target', type='GLenum'),
arg5=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glBinormal3bEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bx', type='GLbyte'),
arg2=ArgDef(name='by', type='GLbyte'),
arg3=ArgDef(name='bz', type='GLbyte')
)

Function(name='glBinormal3bvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*')
)

Function(name='glBinormal3dEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bx', type='GLdouble'),
arg2=ArgDef(name='by', type='GLdouble'),
arg3=ArgDef(name='bz', type='GLdouble')
)

Function(name='glBinormal3dvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*')
)

Function(name='glBinormal3fEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bx', type='GLfloat'),
arg2=ArgDef(name='by', type='GLfloat'),
arg3=ArgDef(name='bz', type='GLfloat')
)

Function(name='glBinormal3fvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glBinormal3iEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bx', type='GLint'),
arg2=ArgDef(name='by', type='GLint'),
arg3=ArgDef(name='bz', type='GLint')
)

Function(name='glBinormal3ivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*')
)

Function(name='glBinormal3sEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bx', type='GLshort'),
arg2=ArgDef(name='by', type='GLshort'),
arg3=ArgDef(name='bz', type='GLshort')
)

Function(name='glBinormal3svEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*')
)

Function(name='glBinormalPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glBitmap', enabled=True, type=Resource,
retV=RetDef(type='void'),
arg1=ArgDef(name='width', type='GLsizei'),
arg2=ArgDef(name='height', type='GLsizei'),
arg3=ArgDef(name='xorig', type='GLfloat'),
arg4=ArgDef(name='yorig', type='GLfloat'),
arg5=ArgDef(name='xmove', type='GLfloat'),
arg6=ArgDef(name='ymove', type='GLfloat'),
arg7=ArgDef(name='bitmap', type='const GLubyte*', wrapType='CGLBitmapResource', wrapParams='width, height, bitmap')
)

Function(name='glBitmapxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='width', type='GLsizei'),
arg2=ArgDef(name='height', type='GLsizei'),
arg3=ArgDef(name='xorig', type='GLfixed'),
arg4=ArgDef(name='yorig', type='GLfixed'),
arg5=ArgDef(name='xmove', type='GLfixed'),
arg6=ArgDef(name='ymove', type='GLfixed'),
arg7=ArgDef(name='bitmap', type='const GLubyte*', wrapType='CGLBitmapResource')
)

Function(name='glBlendBarrier', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glBlendBarrierKHR', enabled=True, type=Param, inheritFrom='glBlendBarrier')

Function(name='glBlendBarrierNV', enabled=False, type=None, inheritFrom='glBlendBarrier')

Function(name='glBlendColor', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat'),
arg4=ArgDef(name='alpha', type='GLfloat')
)

Function(name='glBlendColorEXT', enabled=True, type=Param, inheritFrom='glBlendColor')

Function(name='glBlendColorxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfixed'),
arg2=ArgDef(name='green', type='GLfixed'),
arg3=ArgDef(name='blue', type='GLfixed'),
arg4=ArgDef(name='alpha', type='GLfixed')
)

Function(name='glBlendEquation', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glBlendEquationEXT', enabled=True, type=Param, inheritFrom='glBlendEquation')

Function(name='glBlendEquationIndexedAMD', enabled=True, type=Param, inheritFrom='glBlendEquationi')

Function(name='glBlendEquationOES', enabled=True, type=Param, inheritFrom='glBlendEquation')

Function(name='glBlendEquationSeparate', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='modeRGB', type='GLenum'),
arg2=ArgDef(name='modeAlpha', type='GLenum')
)

Function(name='glBlendEquationSeparateATI', enabled=True, type=Param, inheritFrom='glBlendEquationSeparate')

Function(name='glBlendEquationSeparateEXT', enabled=True, type=Param, inheritFrom='glBlendEquationSeparate')

Function(name='glBlendEquationSeparateIndexedAMD', enabled=True, type=Param, inheritFrom='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateOES', enabled=True, type=Param, inheritFrom='glBlendEquationSeparate')

Function(name='glBlendEquationSeparatei', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLuint'),
arg2=ArgDef(name='modeRGB', type='GLenum'),
arg3=ArgDef(name='modeAlpha', type='GLenum')
)

Function(name='glBlendEquationSeparateiARB', enabled=True, type=Param, inheritFrom='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateiEXT', enabled=True, type=Param, inheritFrom='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateiOES', enabled=False, type=None, inheritFrom='glBlendEquationSeparatei')

Function(name='glBlendEquationi', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLuint'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glBlendEquationiARB', enabled=True, type=Param, inheritFrom='glBlendEquationi')

Function(name='glBlendEquationiEXT', enabled=True, type=Param, inheritFrom='glBlendEquationi')

Function(name='glBlendEquationiOES', enabled=False, type=None, inheritFrom='glBlendEquationi')

Function(name='glBlendFunc', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='sfactor', type='GLenum'),
arg2=ArgDef(name='dfactor', type='GLenum')
)

Function(name='glBlendFuncIndexedAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLuint'),
arg2=ArgDef(name='src', type='GLenum'),
arg3=ArgDef(name='dst', type='GLenum')
)

Function(name='glBlendFuncSeparate', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='sfactorRGB', type='GLenum'),
arg2=ArgDef(name='dfactorRGB', type='GLenum'),
arg3=ArgDef(name='sfactorAlpha', type='GLenum'),
arg4=ArgDef(name='dfactorAlpha', type='GLenum')
)

Function(name='glBlendFuncSeparateEXT', enabled=True, type=Param, inheritFrom='glBlendFuncSeparate')

Function(name='glBlendFuncSeparateINGR', enabled=True, type=Param, inheritFrom='glBlendFuncSeparate')

Function(name='glBlendFuncSeparateIndexedAMD', enabled=True, type=Param, inheritFrom='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateOES', enabled=True, type=Param, inheritFrom='glBlendFuncSeparate')

Function(name='glBlendFuncSeparatei', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLuint'),
arg2=ArgDef(name='srcRGB', type='GLenum'),
arg3=ArgDef(name='dstRGB', type='GLenum'),
arg4=ArgDef(name='srcAlpha', type='GLenum'),
arg5=ArgDef(name='dstAlpha', type='GLenum')
)

Function(name='glBlendFuncSeparateiARB', enabled=True, type=Param, inheritFrom='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateiEXT', enabled=True, type=Param, inheritFrom='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateiOES', enabled=False, type=None, inheritFrom='glBlendFuncSeparatei')

Function(name='glBlendFunci', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLuint'),
arg2=ArgDef(name='src', type='GLenum'),
arg3=ArgDef(name='dst', type='GLenum')
)

Function(name='glBlendFunciARB', enabled=True, type=Param, inheritFrom='glBlendFunci')

Function(name='glBlendFunciEXT', enabled=True, type=Param, inheritFrom='glBlendFunci')

Function(name='glBlendFunciOES', enabled=False, type=None, inheritFrom='glBlendFunci')

Function(name='glBlendParameteriNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='value', type='GLint')
)

Function(name='glBlitFramebuffer', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='srcX0', type='GLint'),
arg2=ArgDef(name='srcY0', type='GLint'),
arg3=ArgDef(name='srcX1', type='GLint'),
arg4=ArgDef(name='srcY1', type='GLint'),
arg5=ArgDef(name='dstX0', type='GLint'),
arg6=ArgDef(name='dstY0', type='GLint'),
arg7=ArgDef(name='dstX1', type='GLint'),
arg8=ArgDef(name='dstY1', type='GLint'),
arg9=ArgDef(name='mask', type='GLbitfield'),
arg10=ArgDef(name='filter', type='GLenum')
)

Function(name='glBlitFramebufferANGLE', enabled=True, type=Fill, inheritFrom='glBlitFramebuffer')

Function(name='glBlitFramebufferEXT', enabled=True, type=Fill, inheritFrom='glBlitFramebuffer')

Function(name='glBlitFramebufferNV', enabled=False, type=None, inheritFrom='glBlitFramebuffer')

Function(name='glBlitNamedFramebuffer', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='readFramebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='drawFramebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg3=ArgDef(name='srcX0', type='GLint'),
arg4=ArgDef(name='srcY0', type='GLint'),
arg5=ArgDef(name='srcX1', type='GLint'),
arg6=ArgDef(name='srcY1', type='GLint'),
arg7=ArgDef(name='dstX0', type='GLint'),
arg8=ArgDef(name='dstY0', type='GLint'),
arg9=ArgDef(name='dstX1', type='GLint'),
arg10=ArgDef(name='dstY1', type='GLint'),
arg11=ArgDef(name='mask', type='GLbitfield'),
arg12=ArgDef(name='filter', type='GLenum')
)

Function(name='glBufferAddressRangeNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='address', type='GLuint64EXT'),
arg4=ArgDef(name='length', type='GLsizeiptr')
)

Function(name='glBufferData', enabled=True, type=Resource, stateTrack=True, recCond='ConditionBufferData(_recorder, target, size, data, usage)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size'),
arg4=ArgDef(name='usage', type='GLenum')
)

Function(name='glBufferDataARB', enabled=True, type=Resource, inheritFrom='glBufferData', recCond=False)

Function(name='glBufferPageCommitmentARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='commit', type='GLboolean')
)

Function(name='glBufferParameteriAPPLE', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glBufferStorage', enabled=True, type=Resource, stateTrack=True, recCond='ConditionBufferStorage(_recorder, target, size, data, flags)', interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size'),
arg4=ArgDef(name='flags', type='GLbitfield', wrapType='CBufferStorageFlags')
)

Function(name='glBufferStorageEXT', enabled=False, type=None, inheritFrom='glBufferStorage', interceptorExecOverride=False)

Function(name='glBufferStorageExternalEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='clientBuffer', type='GLeglClientBufferEXT'),
arg5=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glBufferStorageMemEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='memory', type='GLuint'),
arg4=ArgDef(name='offset', type='GLuint64')
)

Function(name='glBufferSubData', enabled=True, type=Resource, stateTrack=True, recCond='ConditionBufferES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size')
)

Function(name='glBufferSubDataARB', enabled=True, type=Resource, inheritFrom='glBufferSubData', recCond=False)

Function(name='glCallCommandListNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint')
)

Function(name='glCallList', enabled=True, type=Render, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint')
)

Function(name='glCallLists', enabled=True, type=Render, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='lists', type='const void*', wrapType='CGLubyte::CSArray', wrapParams='DataTypeSize(type) * n, static_cast<const GLubyte*>(lists)')
)

Function(name='glCheckFramebufferStatus', enabled=True, type=Get,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glCheckFramebufferStatusEXT', enabled=True, type=Get, inheritFrom='glCheckFramebufferStatus', recWrap=True)

Function(name='glCheckFramebufferStatusOES', enabled=True, type=Get, inheritFrom='glCheckFramebufferStatus')

Function(name='glCheckNamedFramebufferStatus', enabled=True, type=Get,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='target', type='GLenum')
)

Function(name='glCheckNamedFramebufferStatusEXT', enabled=True, type=Get,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebufferEXT'),
arg2=ArgDef(name='target', type='GLenum')
)

Function(name='glClampColor', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='clamp', type='GLenum')
)

Function(name='glClampColorARB', enabled=True, type=Param, inheritFrom='glClampColor')

Function(name='glClear', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glClearAccum', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat'),
arg4=ArgDef(name='alpha', type='GLfloat')
)

Function(name='glClearAccumxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfixed'),
arg2=ArgDef(name='green', type='GLfixed'),
arg3=ArgDef(name='blue', type='GLfixed'),
arg4=ArgDef(name='alpha', type='GLfixed')
)

Function(name='glClearBufferData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glClearBufferSubData', enabled=True, type=Resource, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='offset', type='GLintptr'),
arg4=ArgDef(name='size', type='GLsizeiptr'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size')
)

Function(name='glClearBufferfi', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLenum'),
arg2=ArgDef(name='drawbuffer', type='GLint'),
arg3=ArgDef(name='depth', type='GLfloat'),
arg4=ArgDef(name='stencil', type='GLint')
)

Function(name='glClearBufferfv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLenum'),
arg2=ArgDef(name='drawbuffer', type='GLint'),
arg3=ArgDef(name='value', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearBufferiv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLenum'),
arg2=ArgDef(name='drawbuffer', type='GLint'),
arg3=ArgDef(name='value', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearBufferuiv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLenum'),
arg2=ArgDef(name='drawbuffer', type='GLint'),
arg3=ArgDef(name='value', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearColor', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat'),
arg4=ArgDef(name='alpha', type='GLfloat')
)

Function(name='glClearColorIiEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLint'),
arg2=ArgDef(name='green', type='GLint'),
arg3=ArgDef(name='blue', type='GLint'),
arg4=ArgDef(name='alpha', type='GLint')
)

Function(name='glClearColorIuiEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLuint'),
arg2=ArgDef(name='green', type='GLuint'),
arg3=ArgDef(name='blue', type='GLuint'),
arg4=ArgDef(name='alpha', type='GLuint')
)

Function(name='glClearColorx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfixed'),
arg2=ArgDef(name='green', type='GLfixed'),
arg3=ArgDef(name='blue', type='GLfixed'),
arg4=ArgDef(name='alpha', type='GLfixed')
)

Function(name='glClearColorxOES', enabled=True, type=Param, inheritFrom='glClearColorx')

Function(name='glClearDepth', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='depth', type='GLdouble')
)

Function(name='glClearDepthdNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='depth', type='GLdouble')
)

Function(name='glClearDepthf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='d', type='GLfloat')
)

Function(name='glClearDepthfOES', enabled=True, type=Param, inheritFrom='glClearDepthf',
arg1=ArgDef(name='depth', type='GLclampf')
)

Function(name='glClearDepthx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='depth', type='GLfixed')
)

Function(name='glClearDepthxOES', enabled=True, type=Param, inheritFrom='glClearDepthx',
arg1=ArgDef(name='depth', type='GLclampx')
)

Function(name='glClearIndex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLfloat')
)

Function(name='glClearNamedBufferData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glClearNamedBufferDataEXT', enabled=False, type=None, inheritFrom='glClearNamedBufferData')

Function(name='glClearNamedBufferSubData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='offset', type='GLintptr'),
arg4=ArgDef(name='size', type='GLsizeiptr'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glClearNamedBufferSubDataEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='offset', type='GLsizeiptr'),
arg6=ArgDef(name='size', type='GLsizeiptr'),
arg7=ArgDef(name='data', type='const void*')
)

Function(name='glClearNamedFramebufferfi', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLenum'),
arg3=ArgDef(name='drawbuffer', type='GLint'),
arg4=ArgDef(name='depth', type='GLfloat'),
arg5=ArgDef(name='stencil', type='GLint')
)

Function(name='glClearNamedFramebufferfv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLenum'),
arg3=ArgDef(name='drawbuffer', type='GLint'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearNamedFramebufferiv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLenum'),
arg3=ArgDef(name='drawbuffer', type='GLint'),
arg4=ArgDef(name='value', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearNamedFramebufferuiv', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLenum'),
arg3=ArgDef(name='drawbuffer', type='GLint'),
arg4=ArgDef(name='value', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='buffer, value')
)

Function(name='glClearPixelLocalStorageuiEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='offset', type='GLsizei'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='values', type='const GLuint*')
)

Function(name='glClearStencil', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLint')
)

Function(name='glClearTexImage', enabled=True, type=Resource,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='data', type='const void*', wrapType='CGLClearTexResource', wrapParams='data, texture, level, format, type')
)

Function(name='glClearTexImageEXT', enabled=False, type=None, inheritFrom='glClearTexImage')

Function(name='glClearTexSubImage', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glClearTexSubImageEXT', enabled=False, type=None, inheritFrom='glClearTexSubImage')

Function(name='glClientActiveTexture', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum')
)

Function(name='glClientActiveTextureARB', enabled=True, type=Param, inheritFrom='glClientActiveTexture')

Function(name='glClientActiveVertexStreamATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum')
)

Function(name='glClientAttribDefaultEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glClientWaitSync', enabled=True, type=Param, runWrap=True, interceptorExecOverride=True,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='sync', type='GLsync', wrapType='CGLsync'),
arg2=ArgDef(name='flags', type='GLbitfield'),
arg3=ArgDef(name='timeout', type='GLuint64')
)

Function(name='glClientWaitSyncAPPLE', enabled=False, type=Param,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='sync', type='GLsync'),
arg2=ArgDef(name='flags', type='GLbitfield'),
arg3=ArgDef(name='timeout', type='GLuint64')
)

Function(name='glClipControl', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='origin', type='GLenum'),
arg2=ArgDef(name='depth', type='GLenum')
)

Function(name='glClipControlEXT', enabled=False, type=None, inheritFrom='glClipControl')

Function(name='glClipPlane', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='plane', type='GLenum'),
arg2=ArgDef(name='equation', type='const GLdouble*', wrapParams='4, equation')
)

Function(name='glClipPlanef', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='p', type='GLenum'),
arg2=ArgDef(name='eqn', type='const GLfloat*', wrapParams='4, eqn')
)

Function(name='glClipPlanefIMG', enabled=True, type=Param, inheritFrom='glClipPlanef')

Function(name='glClipPlanefOES', enabled=True, type=Param, inheritFrom='glClipPlanef')

Function(name='glClipPlanex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='plane', type='GLenum'),
arg2=ArgDef(name='equation', type='const GLfixed*', wrapParams='4, equation')
)

Function(name='glClipPlanexIMG', enabled=True, type=Param, inheritFrom='glClipPlanex')

Function(name='glClipPlanexOES', enabled=True, type=Param, inheritFrom='glClipPlanex')

Function(name='glColor3b', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLbyte'),
arg2=ArgDef(name='green', type='GLbyte'),
arg3=ArgDef(name='blue', type='GLbyte')
)

Function(name='glColor3bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*', wrapParams='3, v')
)

Function(name='glColor3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLdouble'),
arg2=ArgDef(name='green', type='GLdouble'),
arg3=ArgDef(name='blue', type='GLdouble')
)

Function(name='glColor3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glColor3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat')
)

Function(name='glColor3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='r', type='GLfloat'),
arg2=ArgDef(name='g', type='GLfloat'),
arg3=ArgDef(name='b', type='GLfloat'),
arg4=ArgDef(name='x', type='GLfloat'),
arg5=ArgDef(name='y', type='GLfloat'),
arg6=ArgDef(name='z', type='GLfloat')
)

Function(name='glColor3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLfloat*', wrapParams='3, c'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glColor3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glColor3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLhalfNV'),
arg2=ArgDef(name='green', type='GLhalfNV'),
arg3=ArgDef(name='blue', type='GLhalfNV')
)

Function(name='glColor3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glColor3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLint'),
arg2=ArgDef(name='green', type='GLint'),
arg3=ArgDef(name='blue', type='GLint')
)

Function(name='glColor3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glColor3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLshort'),
arg2=ArgDef(name='green', type='GLshort'),
arg3=ArgDef(name='blue', type='GLshort')
)

Function(name='glColor3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glColor3ub', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLubyte'),
arg2=ArgDef(name='green', type='GLubyte'),
arg3=ArgDef(name='blue', type='GLubyte')
)

Function(name='glColor3ubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLubyte*', wrapParams='3, v')
)

Function(name='glColor3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLuint'),
arg2=ArgDef(name='green', type='GLuint'),
arg3=ArgDef(name='blue', type='GLuint')
)

Function(name='glColor3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLuint*', wrapParams='3, v')
)

Function(name='glColor3us', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLushort'),
arg2=ArgDef(name='green', type='GLushort'),
arg3=ArgDef(name='blue', type='GLushort')
)

Function(name='glColor3usv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLushort*', wrapParams='3, v')
)

Function(name='glColor3xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfixed'),
arg2=ArgDef(name='green', type='GLfixed'),
arg3=ArgDef(name='blue', type='GLfixed')
)

Function(name='glColor3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='components', type='const GLfixed*')
)

Function(name='glColor4b', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLbyte'),
arg2=ArgDef(name='green', type='GLbyte'),
arg3=ArgDef(name='blue', type='GLbyte'),
arg4=ArgDef(name='alpha', type='GLbyte')
)

Function(name='glColor4bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*', wrapParams='4, v')
)

Function(name='glColor4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLdouble'),
arg2=ArgDef(name='green', type='GLdouble'),
arg3=ArgDef(name='blue', type='GLdouble'),
arg4=ArgDef(name='alpha', type='GLdouble')
)

Function(name='glColor4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glColor4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat'),
arg4=ArgDef(name='alpha', type='GLfloat')
)

Function(name='glColor4fNormal3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='r', type='GLfloat'),
arg2=ArgDef(name='g', type='GLfloat'),
arg3=ArgDef(name='b', type='GLfloat'),
arg4=ArgDef(name='a', type='GLfloat'),
arg5=ArgDef(name='nx', type='GLfloat'),
arg6=ArgDef(name='ny', type='GLfloat'),
arg7=ArgDef(name='nz', type='GLfloat'),
arg8=ArgDef(name='x', type='GLfloat'),
arg9=ArgDef(name='y', type='GLfloat'),
arg10=ArgDef(name='z', type='GLfloat')
)

Function(name='glColor4fNormal3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLfloat*', wrapParams='4, c'),
arg2=ArgDef(name='n', type='const GLfloat*', wrapParams='3, n'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glColor4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glColor4hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLhalfNV'),
arg2=ArgDef(name='green', type='GLhalfNV'),
arg3=ArgDef(name='blue', type='GLhalfNV'),
arg4=ArgDef(name='alpha', type='GLhalfNV')
)

Function(name='glColor4hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='4, v')
)

Function(name='glColor4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLint'),
arg2=ArgDef(name='green', type='GLint'),
arg3=ArgDef(name='blue', type='GLint'),
arg4=ArgDef(name='alpha', type='GLint')
)

Function(name='glColor4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glColor4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLshort'),
arg2=ArgDef(name='green', type='GLshort'),
arg3=ArgDef(name='blue', type='GLshort'),
arg4=ArgDef(name='alpha', type='GLshort')
)

Function(name='glColor4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glColor4ub', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLubyte'),
arg2=ArgDef(name='green', type='GLubyte'),
arg3=ArgDef(name='blue', type='GLubyte'),
arg4=ArgDef(name='alpha', type='GLubyte')
)

Function(name='glColor4ubVertex2fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='r', type='GLubyte'),
arg2=ArgDef(name='g', type='GLubyte'),
arg3=ArgDef(name='b', type='GLubyte'),
arg4=ArgDef(name='a', type='GLubyte'),
arg5=ArgDef(name='x', type='GLfloat'),
arg6=ArgDef(name='y', type='GLfloat')
)

Function(name='glColor4ubVertex2fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLubyte*', wrapParams='4, c'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glColor4ubVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='r', type='GLubyte'),
arg2=ArgDef(name='g', type='GLubyte'),
arg3=ArgDef(name='b', type='GLubyte'),
arg4=ArgDef(name='a', type='GLubyte'),
arg5=ArgDef(name='x', type='GLfloat'),
arg6=ArgDef(name='y', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat')
)

Function(name='glColor4ubVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLubyte*', wrapParams='4, c'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glColor4ubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLubyte*', wrapParams='4, v')
)

Function(name='glColor4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLuint'),
arg2=ArgDef(name='green', type='GLuint'),
arg3=ArgDef(name='blue', type='GLuint'),
arg4=ArgDef(name='alpha', type='GLuint')
)

Function(name='glColor4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLuint*', wrapParams='4, v')
)

Function(name='glColor4us', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLushort'),
arg2=ArgDef(name='green', type='GLushort'),
arg3=ArgDef(name='blue', type='GLushort'),
arg4=ArgDef(name='alpha', type='GLushort')
)

Function(name='glColor4usv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLushort*', wrapParams='4, v')
)

Function(name='glColor4x', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfixed'),
arg2=ArgDef(name='green', type='GLfixed'),
arg3=ArgDef(name='blue', type='GLfixed'),
arg4=ArgDef(name='alpha', type='GLfixed')
)

Function(name='glColor4xOES', enabled=True, type=Param, inheritFrom='glColor4x')

Function(name='glColor4xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='components', type='const GLfixed*')
)

Function(name='glColorFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei')
)

Function(name='glColorFragmentOp1ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMask', type='GLuint'),
arg4=ArgDef(name='dstMod', type='GLuint'),
arg5=ArgDef(name='arg1', type='GLuint'),
arg6=ArgDef(name='arg1Rep', type='GLuint'),
arg7=ArgDef(name='arg1Mod', type='GLuint')
)

Function(name='glColorFragmentOp2ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMask', type='GLuint'),
arg4=ArgDef(name='dstMod', type='GLuint'),
arg5=ArgDef(name='arg1', type='GLuint'),
arg6=ArgDef(name='arg1Rep', type='GLuint'),
arg7=ArgDef(name='arg1Mod', type='GLuint'),
arg8=ArgDef(name='arg2', type='GLuint'),
arg9=ArgDef(name='arg2Rep', type='GLuint'),
arg10=ArgDef(name='arg2Mod', type='GLuint')
)

Function(name='glColorFragmentOp3ATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='dst', type='GLuint'),
arg3=ArgDef(name='dstMask', type='GLuint'),
arg4=ArgDef(name='dstMod', type='GLuint'),
arg5=ArgDef(name='arg1', type='GLuint'),
arg6=ArgDef(name='arg1Rep', type='GLuint'),
arg7=ArgDef(name='arg1Mod', type='GLuint'),
arg8=ArgDef(name='arg2', type='GLuint'),
arg9=ArgDef(name='arg2Rep', type='GLuint'),
arg10=ArgDef(name='arg2Mod', type='GLuint'),
arg11=ArgDef(name='arg3', type='GLuint'),
arg12=ArgDef(name='arg3Rep', type='GLuint'),
arg13=ArgDef(name='arg3Mod', type='GLuint')
)

Function(name='glColorMask', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLboolean'),
arg2=ArgDef(name='green', type='GLboolean'),
arg3=ArgDef(name='blue', type='GLboolean'),
arg4=ArgDef(name='alpha', type='GLboolean')
)

Function(name='glColorMaskIndexedEXT', enabled=True, type=Param, inheritFrom='glColorMaski')

Function(name='glColorMaski', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='r', type='GLboolean'),
arg3=ArgDef(name='g', type='GLboolean'),
arg4=ArgDef(name='b', type='GLboolean'),
arg5=ArgDef(name='a', type='GLboolean')
)

Function(name='glColorMaskiEXT', enabled=True, type=Param, inheritFrom='glColorMaski')

Function(name='glColorMaskiOES', enabled=False, type=None, inheritFrom='glColorMaski')

Function(name='glColorMaterial', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glColorP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='GLuint')
)

Function(name='glColorP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='const GLuint*', wrapParams='3, color')
)

Function(name='glColorP4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='GLuint')
)

Function(name='glColorP4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='const GLuint*', wrapParams='4, color')
)

Function(name='glColorPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glColorPointerBounds', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='ptr', type='const GLvoid*', wrapType='CAttribPtr'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glColorPointerEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glColorPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLint'),
arg4=ArgDef(name='pointer', type='const void**'),
arg5=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glColorPointervINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='pointer', type='const void**')
)

Function(name='glColorSubTable', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='start', type='GLsizei'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='format', type='GLenum'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glColorSubTableEXT', enabled=False, type=None, inheritFrom='glColorSubTable')

Function(name='glColorTable', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='format', type='GLenum'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='table', type='const void*')
)

Function(name='glColorTableEXT', enabled=False, type=None, inheritFrom='glColorTable')

Function(name='glColorTableParameterfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glColorTableParameterfvSGI', enabled=False, type=None, inheritFrom='glColorTableParameterfv')

Function(name='glColorTableParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glColorTableParameterivSGI', enabled=False, type=None, inheritFrom='glColorTableParameteriv')

Function(name='glColorTableSGI', enabled=False, type=None, inheritFrom='glColorTable')

Function(name='glCombinerInputNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='variable', type='GLenum'),
arg4=ArgDef(name='input', type='GLenum'),
arg5=ArgDef(name='mapping', type='GLenum'),
arg6=ArgDef(name='componentUsage', type='GLenum')
)

Function(name='glCombinerOutputNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='abOutput', type='GLenum'),
arg4=ArgDef(name='cdOutput', type='GLenum'),
arg5=ArgDef(name='sumOutput', type='GLenum'),
arg6=ArgDef(name='scale', type='GLenum'),
arg7=ArgDef(name='bias', type='GLenum'),
arg8=ArgDef(name='abDotProduct', type='GLboolean'),
arg9=ArgDef(name='cdDotProduct', type='GLboolean'),
arg10=ArgDef(name='muxSum', type='GLboolean')
)

Function(name='glCombinerParameterfNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glCombinerParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glCombinerParameteriNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glCombinerParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*')
)

Function(name='glCombinerStageParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glCommandListSegmentsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='segments', type='GLuint')
)

Function(name='glCompileCommandListNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint')
)

Function(name='glCompileShader', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram')
)

Function(name='glCompileShaderARB', enabled=True, type=Param, inheritFrom='glCompileShader',
arg1=ArgDef(name='shaderObj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glCompileShaderIncludeARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='path', type='const GLchar*const*'),
arg4=ArgDef(name='length', type='const GLint*')
)

Function(name='glCompressedMultiTexImage1DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='imageSize', type='GLsizei'),
arg8=ArgDef(name='bits', type='const GLvoid*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, bits')
)

Function(name='glCompressedMultiTexImage2DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='imageSize', type='GLsizei'),
arg9=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, bits')
)

Function(name='glCompressedMultiTexImage3DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint'),
arg9=ArgDef(name='imageSize', type='GLsizei'),
arg10=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, bits')
)

Function(name='glCompressedMultiTexSubImage1DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='format', type='GLenum'),
arg7=ArgDef(name='imageSize', type='GLsizei'),
arg8=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, bits')
)

Function(name='glCompressedMultiTexSubImage2DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='imageSize', type='GLsizei'),
arg10=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, bits')
)

Function(name='glCompressedMultiTexSubImage3DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='depth', type='GLsizei'),
arg10=ArgDef(name='format', type='GLenum'),
arg11=ArgDef(name='imageSize', type='GLsizei'),
arg12=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, bits')
)

Function(name='glCompressedTexImage1D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='border', type='GLint'),
arg6=ArgDef(name='imageSize', type='GLsizei'),
arg7=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, data')
)

Function(name='glCompressedTexImage1DARB', enabled=True, type=Resource, inheritFrom='glCompressedTexImage1D')

Function(name='glCompressedTexImage2D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='imageSize', type='GLsizei'),
arg8=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, data')
)

Function(name='glCompressedTexImage2DARB', enabled=True, type=Resource, inheritFrom='glCompressedTexImage2D', recCond=False)

Function(name='glCompressedTexImage3D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='imageSize', type='GLsizei'),
arg9=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, data')
)

Function(name='glCompressedTexImage3DARB', enabled=True, type=Resource, inheritFrom='glCompressedTexImage3D')

Function(name='glCompressedTexImage3DOES', enabled=True, type=Resource, inheritFrom='glCompressedTexImage3D')

Function(name='glCompressedTexSubImage1D', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='imageSize', type='GLsizei'),
arg7=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, data')
)

Function(name='glCompressedTexSubImage1DARB', enabled=True, type=Resource, inheritFrom='glCompressedTexSubImage1D')

Function(name='glCompressedTexSubImage2D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='imageSize', type='GLsizei'),
arg9=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, data')
)

Function(name='glCompressedTexSubImage2DARB', enabled=True, type=Resource, inheritFrom='glCompressedTexSubImage2D', RecCond=False)

Function(name='glCompressedTexSubImage3D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='imageSize', type='GLsizei'),
arg11=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, data')
)

Function(name='glCompressedTexSubImage3DARB', enabled=True, type=Resource, recCond='ConditionTextureES(_recorder)', inheritFrom='glCompressedTexSubImage3D')

Function(name='glCompressedTexSubImage3DOES', enabled=True, type=Resource, inheritFrom='glCompressedTexSubImage3D')

Function(name='glCompressedTextureImage1DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='imageSize', type='GLsizei'),
arg8=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, bits')
)

Function(name='glCompressedTextureImage2DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='imageSize', type='GLsizei'),
arg9=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, bits')
)

Function(name='glCompressedTextureImage3DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint'),
arg9=ArgDef(name='imageSize', type='GLsizei'),
arg10=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, bits')
)

Function(name='glCompressedTextureSubImage1D', enabled=False, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='imageSize', type='GLsizei'),
arg7=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, data')
)

Function(name='glCompressedTextureSubImage1DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='format', type='GLenum'),
arg7=ArgDef(name='imageSize', type='GLsizei'),
arg8=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, imageSize, bits')
)

Function(name='glCompressedTextureSubImage2D', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='imageSize', type='GLsizei'),
arg9=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='texture, width, height, imageSize, data')
)

Function(name='glCompressedTextureSubImage2DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='imageSize', type='GLsizei'),
arg10=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, imageSize, bits')
)

Function(name='glCompressedTextureSubImage3D', enabled=False, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='imageSize', type='GLsizei'),
arg11=ArgDef(name='data', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, data')
)

Function(name='glCompressedTextureSubImage3DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='depth', type='GLsizei'),
arg10=ArgDef(name='format', type='GLenum'),
arg11=ArgDef(name='imageSize', type='GLsizei'),
arg12=ArgDef(name='bits', type='const void*', wrapType='CGLCompressedTexResource', wrapParams='target, width, height, depth, imageSize, bits')
)

Function(name='glConservativeRasterParameterfNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='value', type='GLfloat')
)

Function(name='glConservativeRasterParameteriNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glConvolutionFilter1D', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='format', type='GLenum'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='image', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, image')
)

Function(name='glConvolutionFilter1DEXT', enabled=True, type=Resource, inheritFrom='glConvolutionFilter1D')

Function(name='glConvolutionFilter2D', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='image', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, image')
)

Function(name='glConvolutionFilter2DEXT', enabled=True, type=Resource, inheritFrom='glConvolutionFilter2D')

Function(name='glConvolutionParameterf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat')
)

Function(name='glConvolutionParameterfEXT', enabled=True, type=Param, inheritFrom='glConvolutionParameterf')

Function(name='glConvolutionParameterfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glConvolutionParameterfvEXT', enabled=True, type=Param, inheritFrom='glConvolutionParameterfv')

Function(name='glConvolutionParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint')
)

Function(name='glConvolutionParameteriEXT', enabled=True, type=Param, inheritFrom='glConvolutionParameteri')

Function(name='glConvolutionParameteriv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glConvolutionParameterivEXT', enabled=True, type=Param, inheritFrom='glConvolutionParameteriv')

Function(name='glConvolutionParameterxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glConvolutionParameterxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*')
)

Function(name='glCopyBufferSubData', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='readTarget', type='GLenum'),
arg2=ArgDef(name='writeTarget', type='GLenum'),
arg3=ArgDef(name='readOffset', type='GLintptr'),
arg4=ArgDef(name='writeOffset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glCopyBufferSubDataNV', enabled=False, type=None, inheritFrom='glCopyBufferSubData')

Function(name='glCopyColorSubTable', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='start', type='GLsizei'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyColorSubTableEXT', enabled=True, type=Copy, inheritFrom='glCopyColorSubTable')

Function(name='glCopyColorTable', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyColorTableSGI', enabled=True, type=Copy, inheritFrom='glCopyColorTable')

Function(name='glCopyConvolutionFilter1D', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyConvolutionFilter1DEXT', enabled=True, type=Copy, inheritFrom='glCopyConvolutionFilter1D')

Function(name='glCopyConvolutionFilter2D', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyConvolutionFilter2DEXT', enabled=True, type=Copy, inheritFrom='glCopyConvolutionFilter2D')

Function(name='glCopyImageSubData', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='srcName', type='GLuint'),
arg2=ArgDef(name='srcTarget', type='GLenum'),
arg3=ArgDef(name='srcLevel', type='GLint'),
arg4=ArgDef(name='srcX', type='GLint'),
arg5=ArgDef(name='srcY', type='GLint'),
arg6=ArgDef(name='srcZ', type='GLint'),
arg7=ArgDef(name='dstName', type='GLuint'),
arg8=ArgDef(name='dstTarget', type='GLenum'),
arg9=ArgDef(name='dstLevel', type='GLint'),
arg10=ArgDef(name='dstX', type='GLint'),
arg11=ArgDef(name='dstY', type='GLint'),
arg12=ArgDef(name='dstZ', type='GLint'),
arg13=ArgDef(name='srcWidth', type='GLsizei'),
arg14=ArgDef(name='srcHeight', type='GLsizei'),
arg15=ArgDef(name='srcDepth', type='GLsizei')
)

Function(name='glCopyImageSubDataEXT', enabled=False, type=None, inheritFrom='glCopyImageSubData')

Function(name='glCopyImageSubDataNV', enabled=True, type=Copy, inheritFrom='glCopyImageSubData')

Function(name='glCopyImageSubDataOES', enabled=False, type=None, inheritFrom='glCopyImageSubData')

Function(name='glCopyMultiTexImage1DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint')
)

Function(name='glCopyMultiTexImage2DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='border', type='GLint')
)

Function(name='glCopyMultiTexSubImage1DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyMultiTexSubImage2DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='x', type='GLint'),
arg7=ArgDef(name='y', type='GLint'),
arg8=ArgDef(name='width', type='GLsizei'),
arg9=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyMultiTexSubImage3DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='x', type='GLint'),
arg8=ArgDef(name='y', type='GLint'),
arg9=ArgDef(name='width', type='GLsizei'),
arg10=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyNamedBufferSubData', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='readBuffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='writeBuffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='readOffset', type='GLintptr'),
arg4=ArgDef(name='writeOffset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glCopyPathNV', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='resultPath', type='GLuint'),
arg2=ArgDef(name='srcPath', type='GLuint')
)

Function(name='glCopyPixels', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='type', type='GLenum')
)

Function(name='glCopyTexImage1D', enabled=True, type=Copy, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint')
)

Function(name='glCopyTexImage1DEXT', enabled=True, type=Copy, inheritFrom='glCopyTexImage1D')

Function(name='glCopyTexImage2D', enabled=True, type=Copy, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint')
)

Function(name='glCopyTexImage2DEXT', enabled=True, type=Copy, inheritFrom='glCopyTexImage2D')

Function(name='glCopyTexSubImage1D', enabled=True, type=Copy, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyTexSubImage1DEXT', enabled=True, type=Copy, inheritFrom='glCopyTexSubImage1D')

Function(name='glCopyTexSubImage2D', enabled=True, type=Copy, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyTexSubImage2DEXT', enabled=True, type=Copy, inheritFrom='glCopyTexSubImage2D')

Function(name='glCopyTexSubImage3D', enabled=True, type=Copy, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='x', type='GLint'),
arg7=ArgDef(name='y', type='GLint'),
arg8=ArgDef(name='width', type='GLsizei'),
arg9=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyTexSubImage3DEXT', enabled=True, type=Copy, inheritFrom='glCopyTexSubImage3D')

Function(name='glCopyTexSubImage3DOES', enabled=True, type=Copy, inheritFrom='glCopyTexSubImage3D')

Function(name='glCopyTextureImage1DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint')
)

Function(name='glCopyTextureImage2DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='border', type='GLint')
)

Function(name='glCopyTextureLevelsAPPLE', enabled=False, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='destinationTexture', type='GLuint'),
arg2=ArgDef(name='sourceTexture', type='GLuint'),
arg3=ArgDef(name='sourceBaseLevel', type='GLint'),
arg4=ArgDef(name='sourceLevelCount', type='GLsizei')
)

Function(name='glCopyTextureSubImage1D', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyTextureSubImage1DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei')
)

Function(name='glCopyTextureSubImage2D', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='x', type='GLint'),
arg6=ArgDef(name='y', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyTextureSubImage2DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='x', type='GLint'),
arg7=ArgDef(name='y', type='GLint'),
arg8=ArgDef(name='width', type='GLsizei'),
arg9=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyTextureSubImage3D', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='x', type='GLint'),
arg7=ArgDef(name='y', type='GLint'),
arg8=ArgDef(name='width', type='GLsizei'),
arg9=ArgDef(name='height', type='GLsizei')
)

Function(name='glCopyTextureSubImage3DEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='x', type='GLint'),
arg8=ArgDef(name='y', type='GLint'),
arg9=ArgDef(name='width', type='GLsizei'),
arg10=ArgDef(name='height', type='GLsizei')
)

Function(name='glCoverFillPathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='coverMode', type='GLenum'),
arg6=ArgDef(name='transformType', type='GLenum'),
arg7=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glCoverFillPathNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='coverMode', type='GLenum')
)

Function(name='glCoverStrokePathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='coverMode', type='GLenum'),
arg6=ArgDef(name='transformType', type='GLenum'),
arg7=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glCoverStrokePathNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='coverMode', type='GLenum')
)

Function(name='glCoverageMaskNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLboolean')
)

Function(name='glCoverageModulationNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='components', type='GLenum')
)

Function(name='glCoverageModulationTableNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glCoverageOperationNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='operation', type='GLenum')
)

Function(name='glCreateBuffers', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='buffers', type='GLuint*', wrapType='CGLBuffer::CSMapArray', wrapParams='n, buffers')
)

Function(name='glCreateCommandListsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='lists', type='GLuint*')
)

Function(name='glCreateFramebuffers', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='framebuffers', type='GLuint*', wrapType='CGLFramebuffer::CSMapArray', wrapParams='n, framebuffers')
)

Function(name='glCreateMemoryObjectsEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='memoryObjects', type='GLuint*')
)

Function(name='glCreatePerfQueryINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryId', type='GLuint'),
arg2=ArgDef(name='queryHandle', type='GLuint*')
)

Function(name='glCreateProgram', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='GLuint', wrapType='CGLProgram')
)

Function(name='glCreateProgramObjectARB', enabled=True, type=Create, inheritFrom='glCreateProgram',
retV=RetDef(type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glCreateProgramPipelines', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='pipelines', type='GLuint*', wrapType='CGLPipeline::CSMapArray', wrapParams='n, pipelines')
)

Function(name='glCreateQueries', enabled=True, type=Gen|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='ids', type='GLuint*', wrapType='CGLQuery::CSMapArray', wrapParams='n, ids')
)

Function(name='glCreateRenderbuffers', enabled=True, type=Create,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='renderbuffers', type='GLuint*', wrapType='CGLRenderbuffer::CSMapArray', wrapParams='n, renderbuffers')
)

Function(name='glCreateSamplers', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='samplers', type='GLuint*', wrapType='CGLSampler::CSMapArray', wrapParams='n, samplers')
)

Function(name='glCreateShader', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='GLuint', wrapType='CGLProgram'),
arg1=ArgDef(name='type', type='GLenum')
)

Function(name='glCreateShaderObjectARB', enabled=True, type=Create, inheritFrom='glCreateShader',
retV=RetDef(type='GLhandleARB', wrapType='CGLProgram'),
arg1=ArgDef(name='shaderType', type='GLenum')
)

Function(name='glCreateShaderProgramEXT', enabled=False, type=None,
retV=RetDef(type='GLuint', wrapType='CGLProgram'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='string', type='const GLchar*')
)

Function(name='glCreateShaderProgramv', enabled=True, type=Create|Resource, stateTrack=True, runWrap=True,
retV=RetDef(type='GLuint', wrapType='CGLProgram'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei', wrapParams='1'),
arg3=ArgDef(name='strings', type='const GLchar*const*', wrapType='CShaderSource', wrapParams='count, strings, _strings.SHADER_PROGRAM')
)

Function(name='glCreateShaderProgramvEXT', enabled=True, type=Create|Resource, runWrap=True, inheritFrom='glCreateShaderProgramv')

Function(name='glCreateStatesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='states', type='GLuint*')
)

Function(name='glCreateSyncFromCLeventARB', enabled=False, type=None,
retV=RetDef(type='GLsync'),
arg1=ArgDef(name='context', type='struct _cl_context*'),
arg2=ArgDef(name='event', type='struct _cl_event*'),
arg3=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glCreateTextures', enabled=True, type=Create, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='textures', type='GLuint*', wrapType='CGLTexture::CSMapArray', wrapParams='n, textures')
)

Function(name='glCreateTransformFeedbacks', enabled=True, type=Create,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='GLuint*', wrapType='CGLTransformFeedback::CSMapArray', wrapParams='n, ids')
)

Function(name='glCreateVertexArrays', enabled=True, type=Create, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='arrays', type='GLuint*', wrapType='CGLVertexArray::CSMapArray', wrapParams='n, arrays')
)

Function(name='glCullFace', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glCullParameterdvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='GLdouble*')
)

Function(name='glCullParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='GLfloat*')
)

Function(name='glCurrentPaletteMatrixARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLint')
)

Function(name='glCurrentPaletteMatrixOES', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixpaletteindex', type='GLuint')
)

Function(name='glDebugMessageCallback', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='callback', type='GLDEBUGPROC'),
arg2=ArgDef(name='userParam', type='const void*')
)

Function(name='glDebugMessageCallbackAMD', enabled=False, type=None, inheritFrom='glDebugMessageCallback',
arg1=ArgDef(name='callback', type='GLDEBUGPROCAMD'),
arg2=ArgDef(name='userParam', type='void*')
)

Function(name='glDebugMessageCallbackARB', enabled=False, type=None, inheritFrom='glDebugMessageCallback',
arg1=ArgDef(name='callback', type='GLDEBUGPROCARB')
)

Function(name='glDebugMessageCallbackKHR', enabled=False, type=None, inheritFrom='glDebugMessageCallback',
arg1=ArgDef(name='callback',type='GLDEBUGPROCKHR')
)

Function(name='glDebugMessageControl', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='source', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='severity', type='GLenum'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='ids', type='const GLuint*'),
arg6=ArgDef(name='enabled', type='GLboolean')
)

Function(name='glDebugMessageControlARB', enabled=False, type=None, inheritFrom='glDebugMessageControl')

Function(name='glDebugMessageControlKHR', enabled=False, type=None, inheritFrom='glDebugMessageControl')

Function(name='glDebugMessageEnableAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='category', type='GLenum'),
arg2=ArgDef(name='severity', type='GLenum'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='ids', type='const GLuint*'),
arg5=ArgDef(name='enabled', type='GLboolean')
)

Function(name='glDebugMessageInsert', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='source', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='id', type='GLuint'),
arg4=ArgDef(name='severity', type='GLenum'),
arg5=ArgDef(name='length', type='GLsizei'),
arg6=ArgDef(name='buf', type='const GLchar*')
)

Function(name='glDebugMessageInsertAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='category', type='GLenum'),
arg2=ArgDef(name='severity', type='GLenum'),
arg3=ArgDef(name='id', type='GLuint'),
arg4=ArgDef(name='length', type='GLsizei'),
arg5=ArgDef(name='buf', type='const GLchar*')
)

Function(name='glDebugMessageInsertARB', enabled=False, type=None, inheritFrom='glDebugMessageInsert')

Function(name='glDebugMessageInsertKHR', enabled=False, type=None, inheritFrom='glDebugMessageInsert')

Function(name='glDeformSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glDeformationMap3dSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLdouble'),
arg3=ArgDef(name='u2', type='GLdouble'),
arg4=ArgDef(name='ustride', type='GLint'),
arg5=ArgDef(name='uorder', type='GLint'),
arg6=ArgDef(name='v1', type='GLdouble'),
arg7=ArgDef(name='v2', type='GLdouble'),
arg8=ArgDef(name='vstride', type='GLint'),
arg9=ArgDef(name='vorder', type='GLint'),
arg10=ArgDef(name='w1', type='GLdouble'),
arg11=ArgDef(name='w2', type='GLdouble'),
arg12=ArgDef(name='wstride', type='GLint'),
arg13=ArgDef(name='worder', type='GLint'),
arg14=ArgDef(name='points', type='const GLdouble*')
)

Function(name='glDeformationMap3fSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLfloat'),
arg3=ArgDef(name='u2', type='GLfloat'),
arg4=ArgDef(name='ustride', type='GLint'),
arg5=ArgDef(name='uorder', type='GLint'),
arg6=ArgDef(name='v1', type='GLfloat'),
arg7=ArgDef(name='v2', type='GLfloat'),
arg8=ArgDef(name='vstride', type='GLint'),
arg9=ArgDef(name='vorder', type='GLint'),
arg10=ArgDef(name='w1', type='GLfloat'),
arg11=ArgDef(name='w2', type='GLfloat'),
arg12=ArgDef(name='wstride', type='GLint'),
arg13=ArgDef(name='worder', type='GLint'),
arg14=ArgDef(name='points', type='const GLfloat*')
)

Function(name='glDeleteAsyncMarkersSGIX', enabled=True, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='marker', type='GLuint'),
arg2=ArgDef(name='range', type='GLsizei')
)

Function(name='glDeleteBuffers', enabled=True, type=Delete, stateTrack=True, runCond='ConditionCurrentContextZero()',
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='buffers', type='const GLuint*', wrapType='CGLBuffer::CSUnmapArray', wrapParams='n, buffers', removeMapping=True)
)

Function(name='glDeleteBuffersARB', enabled=True, type=Delete, inheritFrom='glDeleteBuffers')

Function(name='glDeleteCommandListsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='lists', type='const GLuint*')
)

Function(name='glDeleteFencesAPPLE', enabled=False, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='fences', type='const GLuint*')
)

Function(name='glDeleteFencesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='fences', type='const GLuint*')
)

Function(name='glDeleteFragmentShaderATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glDeleteFramebuffers', enabled=True, type=Delete, stateTrack=True, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='framebuffers', type='const GLuint*', wrapType='CGLFramebuffer::CSUnmapArray', wrapParams='n, framebuffers', removeMapping=True)
)

Function(name='glDeleteFramebuffersEXT', enabled=True, type=Delete, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='framebuffers', type='const GLuint*', wrapType='CGLFramebufferEXT::CSUnmapArray', wrapParams='n, framebuffers', removeMapping=True)
)

Function(name='glDeleteFramebuffersOES', enabled=True, type=Delete, inheritFrom='glDeleteFramebuffers')

Function(name='glDeleteLists', enabled=True, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='range', type='GLsizei')
)

Function(name='glDeleteMemoryObjectsEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='memoryObjects', type='const GLuint*')
)

Function(name='glDeleteNamedStringARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='namelen', type='GLint'),
arg2=ArgDef(name='name', type='const GLchar*')
)

Function(name='glDeleteNamesAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='identifier', type='GLenum'),
arg2=ArgDef(name='num', type='GLuint'),
arg3=ArgDef(name='names', type='const GLuint*')
)

Function(name='glDeleteObjectARB', enabled=True, type=Delete, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glDeleteOcclusionQueriesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='const GLuint*')
)

Function(name='glDeletePathsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='range', type='GLsizei')
)

Function(name='glDeletePerfMonitorsAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='monitors', type='GLuint*')
)

Function(name='glDeletePerfQueryINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryHandle', type='GLuint')
)

Function(name='glDeleteProgram', enabled=True, type=Resource, stateTrack=True, runWrap=True, runCond='ConditionCurrentContextZero()',
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glDeleteProgramPipelines', enabled=True, type=Delete, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='pipelines', type='const GLuint*', wrapType='CGLPipeline::CSUnmapArray', wrapParams='n, pipelines', removeMapping=True)
)

Function(name='glDeleteProgramPipelinesEXT', enabled=True, type=Delete, inheritFrom='glDeleteProgramPipelines')

Function(name='glDeleteProgramsARB', enabled=True, type=Delete, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='programs', type='const GLuint*', wrapType='CGLProgram::CSUnmapArray', wrapParams='n, programs')
)

Function(name='glDeleteProgramsNV', enabled=True, type=Delete, inheritFrom='glDeleteProgramsARB')

Function(name='glDeleteQueries', enabled=True, type=Delete|Query,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='const GLuint*', wrapType='CGLQuery::CSUnmapArray', wrapParams='n, ids', removeMapping=True)
)

Function(name='glDeleteQueriesARB', enabled=True, type=Delete|Query, inheritFrom='glDeleteQueries')

Function(name='glDeleteQueriesEXT', enabled=True, type=Delete|Query, inheritFrom='glDeleteQueries')

Function(name='glDeleteQueryResourceTagNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='tagIds', type='const GLint*')
)

Function(name='glDeleteRenderbuffers', enabled=True, type=Delete, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='renderbuffers', type='const GLuint*', wrapType='CGLRenderbuffer::CSUnmapArray', wrapParams='n, renderbuffers', removeMapping=True)
)

Function(name='glDeleteRenderbuffersEXT', enabled=True, type=Delete, stateTrack=True, recWrap=True, inheritFrom='glDeleteRenderbuffers',
arg2=ArgDef(name='renderbuffers', type='const GLuint*', wrapType='CGLRenderbufferEXT::CSUnmapArray', wrapParams='n, renderbuffers', removeMapping=True)
)

Function(name='glDeleteRenderbuffersOES', enabled=True, type=Delete, inheritFrom='glDeleteRenderbuffers')

Function(name='glDeleteSamplers', enabled=True, type=Delete, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLsizei'),
arg2=ArgDef(name='samplers', type='const GLuint*', wrapType='CGLSampler::CSUnmapArray', wrapParams='count, samplers', removeMapping=True)
)

Function(name='glDeleteSemaphoresEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='semaphores', type='const GLuint*')
)

Function(name='glDeleteShader', enabled=True, type=Delete, stateTrack=True, recWrap=True, runCond='ConditionCurrentContextZero()',
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram')
)

Function(name='glDeleteStatesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='states', type='const GLuint*')
)

Function(name='glDeleteSync', enabled=True, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='sync', type='GLsync', wrapType='CGLsync', removeMapping=True)
)

Function(name='glDeleteSyncAPPLE', enabled=False, type=Delete, inheritFrom='glDeleteSync')

Function(name='glDeleteTextures', enabled=True, type=Delete, stateTrack=True, runCond='ConditionCurrentContextZero()', recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='textures', type='const GLuint*', wrapType='CGLTexture::CSUnmapArray', wrapParams='n, textures', removeMapping=True)
)

Function(name='glDeleteTexturesEXT', enabled=True, type=Delete, inheritFrom='glDeleteTextures', runCond=False)

Function(name='glDeleteTransformFeedbacks', enabled=True, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='const GLuint*',wrapType='CGLTransformFeedback::CSUnmapArray', wrapParams='n, ids', removeMapping=True)
)

Function(name='glDeleteTransformFeedbacksNV', enabled=True, type=Delete, inheritFrom='glDeleteTransformFeedbacks')

Function(name='glDeleteVertexArrays', enabled=True, type=Delete, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='arrays', type='const GLuint*', wrapType='CGLVertexArray::CSUnmapArray', wrapParams='n, arrays', removeMapping=True)
)

Function(name='glDeleteVertexArraysAPPLE', enabled=False, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='arrays', type='const GLuint*')
)

Function(name='glDeleteVertexArraysOES', enabled=True, type=Delete, inheritFrom='glDeleteVertexArrays')

Function(name='glDeleteVertexShaderEXT', enabled=True, type=Delete,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glDepthBoundsEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='zmin', type='GLclampd'),
arg2=ArgDef(name='zmax', type='GLclampd')
)

Function(name='glDepthBoundsdNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='zmin', type='GLdouble'),
arg2=ArgDef(name='zmax', type='GLdouble')
)

Function(name='glDepthFunc', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum')
)

Function(name='glDepthMask', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='flag', type='GLboolean')
)

Function(name='glDepthRange', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLdouble'),
arg2=ArgDef(name='f', type='GLdouble')
)

Function(name='glDepthRangeArrayfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glDepthRangeArrayfvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glDepthRangeArrayv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLdouble*', wrapParams='count, v')
)

Function(name='glDepthRangeIndexed', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLdouble'),
arg3=ArgDef(name='f', type='GLdouble')
)

Function(name='glDepthRangeIndexedfNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLfloat'),
arg3=ArgDef(name='f', type='GLfloat')
)

Function(name='glDepthRangeIndexedfOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLfloat'),
arg3=ArgDef(name='f', type='GLfloat')
)

Function(name='glDepthRangedNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='zNear', type='GLdouble'),
arg2=ArgDef(name='zFar', type='GLdouble')
)

Function(name='glDepthRangef', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLfloat'),
arg2=ArgDef(name='f', type='GLfloat')
)

Function(name='glDepthRangefOES', enabled=True, type=Param, inheritFrom='glDepthRangef',
arg1=ArgDef(name='zNear', type='GLclampf'),
arg2=ArgDef(name='zFar', type='GLclampf')
)

Function(name='glDepthRangex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLfixed'),
arg2=ArgDef(name='f', type='GLfixed')
)

Function(name='glDepthRangexOES', enabled=True, type=Param, inheritFrom='glDepthRangex')

Function(name='glDetachObjectARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='containerObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='attachedObj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glDetachShader', enabled=True, type=Param, runCond='ConditionCurrentContextZero()', stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram')
)

Function(name='glDetailTexFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='points', type='const GLfloat*')
)

Function(name='glDisable', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='cap', type='GLenum')
)

Function(name='glDisableClientState', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum')
)

Function(name='glDisableClientStateIndexedEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableClientStateiEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableDriverControlQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='driverControl', type='GLuint')
)

Function(name='glDisableIndexedEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableVariantClientStateEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glDisableVertexArrayAttrib', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableVertexArrayAttribEXT', enabled=False, type=None, inheritFrom='glDisableVertexArrayAttrib')

Function(name='glDisableVertexArrayEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='array', type='GLenum')
)

Function(name='glDisableVertexAttribAPPLE', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum')
)

Function(name='glDisableVertexAttribArray', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableVertexAttribArrayARB', enabled=True, type=Param, recWrap=True, inheritFrom='glDisableVertexAttribArray')

Function(name='glDisablei', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glDisableiEXT', enabled=True, type=Param, inheritFrom='glDisablei')

Function(name='glDisableiNV', enabled=False, type=None, inheritFrom='glDisablei')

Function(name='glDisableiOES', enabled=False, type=None, inheritFrom='glDisablei')

Function(name='glDiscardFramebufferEXT', enabled=True, type=Param, runCond='ConditionExtension("GL_EXT_discard_framebuffer")',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='numAttachments', type='GLsizei'),
arg3=ArgDef(name='attachments', type='const GLenum*', wrapParams='numAttachments, attachments')
)

Function(name='glDispatchCompute', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='num_groups_x', type='GLuint'),
arg2=ArgDef(name='num_groups_y', type='GLuint'),
arg3=ArgDef(name='num_groups_z', type='GLuint')
)

Function(name='glDispatchComputeGroupSizeARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='num_groups_x', type='GLuint'),
arg2=ArgDef(name='num_groups_y', type='GLuint'),
arg3=ArgDef(name='num_groups_z', type='GLuint'),
arg4=ArgDef(name='group_size_x', type='GLuint'),
arg5=ArgDef(name='group_size_y', type='GLuint'),
arg6=ArgDef(name='group_size_z', type='GLuint')
)

Function(name='glDispatchComputeIndirect', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='indirect', type='GLintptr')
)

Function(name='glDrawArrays', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(first, count, 0, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei')
)

Function(name='glDrawArraysEXT', enabled=True, type=Render, inheritFrom='glDrawArrays')

Function(name='glDrawArraysIndirect', enabled=True, type=Render, preToken='CgitsClientIndirectArraysUpdate(mode, indirect, 1, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr')
)

Function(name='glDrawArraysInstanced', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(first, count, instancecount, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='instancecount', type='GLsizei')
)

Function(name='glDrawArraysInstancedANGLE', enabled=True, type=Render, execPostRecWrap=True, inheritFrom='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedARB', enabled=True, type=Render, inheritFrom='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedBaseInstance', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)',
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='instancecount', type='GLsizei'),
arg5=ArgDef(name='baseinstance', type='GLuint')
)

Function(name='glDrawArraysInstancedBaseInstanceEXT', enabled=False, type=None, preToken='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)', inheritFrom='glDrawArraysInstancedBaseInstance')

Function(name='glDrawArraysInstancedEXT', enabled=True, type=Render, inheritFrom='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedNV', enabled=False, type=None, preToken='CgitsClientArraysUpdate(first, count, instancecount, 0)', inheritFrom='glDrawArraysInstanced')

Function(name='glDrawBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='buf', type='GLenum')
)

Function(name='glDrawBuffers', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='bufs', type='const GLenum*', wrapParams='n,bufs')
)

Function(name='glDrawBuffersARB', enabled=True, type=Param, inheritFrom='glDrawBuffers')

Function(name='glDrawBuffersATI', enabled=True, type=Param, inheritFrom='glDrawBuffers')

Function(name='glDrawBuffersEXT', enabled=False, type=None, inheritFrom='glDrawBuffers')

Function(name='glDrawBuffersIndexedEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLint'),
arg2=ArgDef(name='location', type='const GLenum*'),
arg3=ArgDef(name='indices', type='const GLint*')
)

Function(name='glDrawBuffersNV', enabled=True, type=Param, inheritFrom='glDrawBuffers')

Function(name='glDrawCommandsAddressNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='primitiveMode', type='GLenum'),
arg2=ArgDef(name='indirects', type='const GLuint64*'),
arg3=ArgDef(name='sizes', type='const GLsizei*'),
arg4=ArgDef(name='count', type='GLuint')
)

Function(name='glDrawCommandsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='primitiveMode', type='GLenum'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='indirects', type='const GLintptr*'),
arg4=ArgDef(name='sizes', type='const GLsizei*'),
arg5=ArgDef(name='count', type='GLuint')
)

Function(name='glDrawCommandsStatesAddressNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='indirects', type='const GLuint64*'),
arg2=ArgDef(name='sizes', type='const GLsizei*'),
arg3=ArgDef(name='states', type='const GLuint*'),
arg4=ArgDef(name='fbos', type='const GLuint*'),
arg5=ArgDef(name='count', type='GLuint')
)

Function(name='glDrawCommandsStatesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='indirects', type='const GLintptr*'),
arg3=ArgDef(name='sizes', type='const GLsizei*'),
arg4=ArgDef(name='states', type='const GLuint*'),
arg5=ArgDef(name='fbos', type='const GLuint*'),
arg6=ArgDef(name='count', type='GLuint')
)

Function(name='glDrawElementArrayAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei')
)

Function(name='glDrawElementArrayATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei')
)

Function(name='glDrawElements', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', recCond='count>=0 && Recording(_recorder)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr')
)

Function(name='glDrawElementsBaseVertex', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg5=ArgDef(name='basevertex', type='GLint')
)

Function(name='glDrawElementsBaseVertexEXT', enabled=False, type=None, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inheritFrom='glDrawElementsBaseVertex')

Function(name='glDrawElementsBaseVertexOES', enabled=False, type=None, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inheritFrom='glDrawElementsBaseVertex')

Function(name='glDrawElementsIndirect', enabled=True, type=Render, preToken='CgitsClientIndirectArraysUpdate(mode, type, indirect, 1, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr')
)

Function(name='glDrawElementsInstanced', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg5=ArgDef(name='instancecount', type='GLsizei')
)

Function(name='glDrawElementsInstancedANGLE', enabled=True, type=Render, inheritFrom='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedARB', enabled=True, type=Render, inheritFrom='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedBaseInstance', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)',
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg5=ArgDef(name='instancecount', type='GLsizei'),
arg6=ArgDef(name='baseinstance', type='GLuint')
)

Function(name='glDrawElementsInstancedBaseInstanceEXT', enabled=False, type=None, preToken='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)', inheritFrom='glDrawElementsInstancedBaseInstance')

Function(name='glDrawElementsInstancedBaseVertex', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, basevertex)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg5=ArgDef(name='instancecount', type='GLsizei'),
arg6=ArgDef(name='basevertex', type='GLint')
)

Function(name='glDrawElementsInstancedBaseVertexBaseInstance', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, basevertex)',
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg5=ArgDef(name='instancecount', type='GLsizei'),
arg6=ArgDef(name='basevertex', type='GLint'),
arg7=ArgDef(name='baseinstance', type='GLuint')
)

Function(name='glDrawElementsInstancedBaseVertexBaseInstanceEXT', enabled=False, type=None, inheritFrom='glDrawElementsInstancedBaseVertexBaseInstance')

Function(name='glDrawElementsInstancedBaseVertexEXT', enabled=False, type=None, inheritFrom='glDrawElementsInstancedBaseVertex')

Function(name='glDrawElementsInstancedBaseVertexOES', enabled=False, type=None, inheritFrom='glDrawElementsInstancedBaseVertex')

Function(name='glDrawElementsInstancedEXT', enabled=True, type=Render, inheritFrom='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedNV', enabled=False, type=None, inheritFrom='glDrawElementsInstanced')

Function(name='glDrawMeshArraysSUN', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='width', type='GLsizei')
)

Function(name='glDrawPixels', enabled=True, type=Fill, preSchedule='coherentBufferUpdate_PS(_recorder)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='width', type='GLsizei'),
arg2=ArgDef(name='height', type='GLsizei'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='GL_TEXTURE_2D, format, type, width, height, pixels')
)

Function(name='glDrawRangeElementArrayAPPLE', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='end', type='GLuint'),
arg4=ArgDef(name='first', type='GLint'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glDrawRangeElementArrayATI', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='end', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei')
)

Function(name='glDrawRangeElements', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='end', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr')
)

Function(name='glDrawRangeElementsBaseVertex', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='end', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='indices', type='const void*', wrapType='CIndexPtr'),
arg7=ArgDef(name='basevertex', type='GLint')
)

Function(name='glDrawRangeElementsBaseVertexEXT', enabled=False, type=None, inheritFrom='glDrawRangeElementsBaseVertex')

Function(name='glDrawRangeElementsBaseVertexOES', enabled=False, type=None, inheritFrom='glDrawRangeElementsBaseVertex')

Function(name='glDrawRangeElementsEXT', enabled=True, type=Render, inheritFrom='glDrawRangeElements')

Function(name='glDrawTexfOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat'),
arg4=ArgDef(name='width', type='GLfloat'),
arg5=ArgDef(name='height', type='GLfloat')
)

Function(name='glDrawTexfvOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfloat*', wrapParams='5, coords')
)

Function(name='glDrawTexiOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint'),
arg4=ArgDef(name='width', type='GLint'),
arg5=ArgDef(name='height', type='GLint')
)

Function(name='glDrawTexivOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLint*', wrapParams='5, coords')
)

Function(name='glDrawTexsOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort'),
arg4=ArgDef(name='width', type='GLshort'),
arg5=ArgDef(name='height', type='GLshort')
)

Function(name='glDrawTexsvOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLshort*', wrapParams='5, coords')
)

Function(name='glDrawTextureNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg3=ArgDef(name='x0', type='GLfloat'),
arg4=ArgDef(name='y0', type='GLfloat'),
arg5=ArgDef(name='x1', type='GLfloat'),
arg6=ArgDef(name='y1', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat'),
arg8=ArgDef(name='s0', type='GLfloat'),
arg9=ArgDef(name='t0', type='GLfloat'),
arg10=ArgDef(name='s1', type='GLfloat'),
arg11=ArgDef(name='t1', type='GLfloat')
)

Function(name='glDrawTexxOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed'),
arg4=ArgDef(name='width', type='GLfixed'),
arg5=ArgDef(name='height', type='GLfixed')
)

Function(name='glDrawTexxvOES', enabled=True, type=Fill, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*', wrapParams='5, coords')
)

Function(name='glDrawTransformFeedback', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint')
)

Function(name='glDrawTransformFeedbackEXT', enabled=False, type=None, inheritFrom='glDrawTransformFeedback')

Function(name='glDrawTransformFeedbackInstanced', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='instancecount', type='GLsizei')
)

Function(name='glDrawTransformFeedbackInstancedEXT', enabled=False, type=None, inheritFrom='glDrawTransformFeedbackInstanced')

Function(name='glDrawTransformFeedbackNV', enabled=True, type=Render, inheritFrom='glDrawTransformFeedback')

Function(name='glDrawTransformFeedbackStream', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='stream', type='GLuint')
)

Function(name='glDrawTransformFeedbackStreamInstanced', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='stream', type='GLuint'),
arg4=ArgDef(name='instancecount', type='GLsizei')
)

Function(name='glDrawVkImageNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vkImage', type='GLuint64'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg3=ArgDef(name='x0', type='GLfloat'),
arg4=ArgDef(name='y0', type='GLfloat'),
arg5=ArgDef(name='x1', type='GLfloat'),
arg6=ArgDef(name='y1', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat'),
arg8=ArgDef(name='s0', type='GLfloat'),
arg9=ArgDef(name='t0', type='GLfloat'),
arg10=ArgDef(name='s1', type='GLfloat'),
arg11=ArgDef(name='t1', type='GLfloat')
)

Function(name='glEGLImageTargetRenderbufferStorageOES', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='image', type='GLeglImageOES', wrapType='CEGLImageKHR')
)

Function(name='glEGLImageTargetTexStorageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='image', type='GLeglImageOES', wrapType='CEGLImageKHR'),
arg3=ArgDef(name='attrib_list', type='const GLint*')
)

Function(name='glEGLImageTargetTexture2DOES', enabled=True, type=Param, runWrap=True, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='image', type='GLeglImageOES', wrapType='CEGLImageKHR')
)

Function(name='glEGLImageTargetTextureStorageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='image', type='GLeglImageOES', wrapType='CEGLImageKHR'),
arg3=ArgDef(name='attrib_list', type='const GLint*')
)

Function(name='glEdgeFlag', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='flag', type='GLboolean')
)

Function(name='glEdgeFlagFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stride', type='GLsizei')
)

Function(name='glEdgeFlagPointer', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stride', type='GLsizei'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glEdgeFlagPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stride', type='GLsizei'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const GLboolean*')
)

Function(name='glEdgeFlagPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stride', type='GLint'),
arg2=ArgDef(name='pointer', type='const GLboolean**'),
arg3=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glEdgeFlagv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='flag', type='const GLboolean*')
)

Function(name='glElementPointerAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glElementPointerATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glEnable', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='cap', type='GLenum')
)

Function(name='glEnableClientState', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum')
)

Function(name='glEnableClientStateIndexedEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableClientStateiEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableDriverControlQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='driverControl', type='GLuint')
)

Function(name='glEnableIndexedEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableVariantClientStateEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glEnableVertexArrayAttrib', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableVertexArrayAttribEXT', enabled=True, type=Param, inheritFrom='glEnableVertexArrayAttrib')

Function(name='glEnableVertexArrayEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='array', type='GLenum')
)

Function(name='glEnableVertexAttribAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum')
)

Function(name='glEnableVertexAttribArray', enabled=True, type=Param, recWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableVertexAttribArrayARB', enabled=True, type=Param, recWrap=True, inheritFrom='glEnableVertexAttribArray')

Function(name='glEnablei', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEnableiEXT', enabled=True, type=Param, inheritFrom='glEnablei')

Function(name='glEnableiNV', enabled=False, type=None, inheritFrom='glEnablei')

Function(name='glEnableiOES', enabled=False, type=None, inheritFrom='glEnablei')

Function(name='glEnd', enabled=True, type=Param, runWrap=True, stateTrack=True, suffix="""// See comment in glBegin for rationale of following line.
  globalMutex.unlock();""",
retV=RetDef(type='void')
)

Function(name='glEndConditionalRender', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glEndConditionalRenderNV', enabled=True, type=Param, inheritFrom='glEndConditionalRender')

Function(name='glEndConditionalRenderNVX', enabled=False, type=None, inheritFrom='glEndConditionalRender')

Function(name='glEndFragmentShaderATI', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glEndList', enabled=True, type=Param, recWrap=True,
retV=RetDef(type='void')
)

Function(name='glEndOcclusionQueryNV', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glEndPerfMonitorAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='monitor', type='GLuint')
)

Function(name='glEndPerfQueryINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryHandle', type='GLuint')
)

Function(name='glEndQuery', enabled=True, type=Param|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glEndQueryARB', enabled=True, type=Param|Query, inheritFrom='glEndQuery')

Function(name='glEndQueryEXT', enabled=True, type=Param|Query, inheritFrom='glEndQuery')

Function(name='glEndQueryIndexed', enabled=True, type=Param|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glEndTilingQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='preserveMask', type='GLbitfield')
)

Function(name='glEndTransformFeedback', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glEndTransformFeedbackEXT', enabled=True, type=Param, inheritFrom='glEndTransformFeedback')

Function(name='glEndTransformFeedbackNV', enabled=True, type=Param, inheritFrom='glEndTransformFeedback')

Function(name='glEndVertexShaderEXT', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glEndVideoCaptureNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint')
)

Function(name='glEvalCoord1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLdouble')
)

Function(name='glEvalCoord1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='const GLdouble*', wrapParams='1, u')
)

Function(name='glEvalCoord1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLfloat')
)

Function(name='glEvalCoord1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='const GLfloat*', wrapParams='1, u')
)

Function(name='glEvalCoord1xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLfixed')
)

Function(name='glEvalCoord1xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glEvalCoord2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLdouble'),
arg2=ArgDef(name='v', type='GLdouble')
)

Function(name='glEvalCoord2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='const GLdouble*', wrapParams='2, u')
)

Function(name='glEvalCoord2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLfloat'),
arg2=ArgDef(name='v', type='GLfloat')
)

Function(name='glEvalCoord2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='const GLfloat*', wrapParams='2, u')
)

Function(name='glEvalCoord2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='u', type='GLfixed'),
arg2=ArgDef(name='v', type='GLfixed')
)

Function(name='glEvalCoord2xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glEvalMapsNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glEvalMesh1', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='i1', type='GLint'),
arg3=ArgDef(name='i2', type='GLint')
)

Function(name='glEvalMesh2', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='i1', type='GLint'),
arg3=ArgDef(name='i2', type='GLint'),
arg4=ArgDef(name='j1', type='GLint'),
arg5=ArgDef(name='j2', type='GLint')
)

Function(name='glEvalPoint1', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='i', type='GLint')
)

Function(name='glEvalPoint2', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='i', type='GLint'),
arg2=ArgDef(name='j', type='GLint')
)

Function(name='glEvaluateDepthValuesARB', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glExecuteProgramNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glExtGetBufferPointervQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='params', type='void**')
)

Function(name='glExtGetBuffersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffers', type='GLuint*'),
arg2=ArgDef(name='maxBuffers', type='GLint'),
arg3=ArgDef(name='numBuffers', type='GLint*')
)

Function(name='glExtGetFramebuffersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffers', type='GLuint*'),
arg2=ArgDef(name='maxFramebuffers', type='GLint'),
arg3=ArgDef(name='numFramebuffers', type='GLint*')
)

Function(name='glExtGetProgramBinarySourceQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='source', type='GLchar*'),
arg4=ArgDef(name='length', type='GLint*')
)

Function(name='glExtGetProgramsQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='programs', type='GLuint*'),
arg2=ArgDef(name='maxPrograms', type='GLint'),
arg3=ArgDef(name='numPrograms', type='GLint*')
)

Function(name='glExtGetRenderbuffersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='renderbuffers', type='GLuint*'),
arg2=ArgDef(name='maxRenderbuffers', type='GLint'),
arg3=ArgDef(name='numRenderbuffers', type='GLint*')
)

Function(name='glExtGetShadersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='shaders', type='GLuint*'),
arg2=ArgDef(name='maxShaders', type='GLint'),
arg3=ArgDef(name='numShaders', type='GLint*')
)

Function(name='glExtGetTexLevelParameterivQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='face', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLint*')
)

Function(name='glExtGetTexSubImageQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='texels', type='void*')
)

Function(name='glExtGetTexturesQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='textures', type='GLuint*'),
arg2=ArgDef(name='maxTextures', type='GLint'),
arg3=ArgDef(name='numTextures', type='GLint*')
)

Function(name='glExtIsProgramBinaryQCOM', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glExtTexObjectStateOverrideiQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glExtractComponentEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='res', type='GLuint'),
arg2=ArgDef(name='src', type='GLuint'),
arg3=ArgDef(name='num', type='GLuint')
)

Function(name='glFeedbackBuffer', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLsizei'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='buffer', type='GLfloat*')
)

Function(name='glFeedbackBufferxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='buffer', type='const GLfixed*')
)

Function(name='glFenceSync', enabled=True, type=Create,
retV=RetDef(type='GLsync', wrapType='CGLsync'),
arg1=ArgDef(name='condition', type='GLenum'),
arg2=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glFenceSyncAPPLE', enabled=False, type=Create, inheritFrom='glFenceSync')

Function(name='glFinalCombinerInputNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='variable', type='GLenum'),
arg2=ArgDef(name='input', type='GLenum'),
arg3=ArgDef(name='mapping', type='GLenum'),
arg4=ArgDef(name='componentUsage', type='GLenum')
)

Function(name='glFinish', enabled=True, type=Exec, runWrap=True, ccodeWrap=True, recWrap=True,
retV=RetDef(type='void')
)

Function(name='glFinishAsyncSGIX', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='markerp', type='GLuint*')
)

Function(name='glFinishFenceAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glFinishFenceNV', enabled=True, type=Exec,
retV=RetDef(type='void'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glFinishObjectAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void'),
arg1=ArgDef(name='object', type='GLenum'),
arg2=ArgDef(name='name', type='GLint')
)

Function(name='glFinishRenderAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void')
)

Function(name='glFinishTextureSUNX', enabled=True, type=Exec,
retV=RetDef(type='void')
)

Function(name='glFlush', enabled=True, type=Exec, runWrap=True, ccodeWrap=True, recWrap=True,
retV=RetDef(type='void')
)

Function(name='glFlushMappedBufferRange', enabled=True, type=Exec, stateTrack=True, preToken='CgitsFlushMappedBufferRange(target,offset,length)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='length', type='GLsizeiptr')
)

Function(name='glFlushMappedBufferRangeAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glFlushMappedBufferRangeEXT', enabled=True, type=Exec, inheritFrom='glFlushMappedBufferRange', preToken=False )

Function(name='glFlushMappedNamedBufferRange', enabled=True, type=Exec, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='length', type='GLsizeiptr')
)

Function(name='glFlushMappedNamedBufferRangeEXT', enabled=True, type=Exec, inheritFrom='glFlushMappedNamedBufferRange')

Function(name='glFlushPixelDataRangeNV', enabled=True, type=Exec,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glFlushRasterSGIX', enabled=True, type=Exec,
retV=RetDef(type='void')
)

Function(name='glFlushRenderAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void')
)

Function(name='glFlushStaticDataIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glFlushVertexArrayRangeAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='length', type='GLsizei'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glFlushVertexArrayRangeNV', enabled=True, type=Exec,
retV=RetDef(type='void')
)

Function(name='glFogCoordFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei')
)

Function(name='glFogCoordPointer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glFogCoordPointerEXT', enabled=True, type=Param, inheritFrom='glFogCoordPointer')

Function(name='glFogCoordPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLint'),
arg3=ArgDef(name='pointer', type='const void**'),
arg4=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glFogCoordd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLdouble')
)

Function(name='glFogCoorddEXT', enabled=True, type=Param, inheritFrom='glFogCoordd')

Function(name='glFogCoorddv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='const GLdouble*', wrapParams='1, coord')
)

Function(name='glFogCoorddvEXT', enabled=True, type=Param, inheritFrom='glFogCoorddv')

Function(name='glFogCoordf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLfloat')
)

Function(name='glFogCoordfEXT', enabled=True, type=Param, inheritFrom='glFogCoordf')

Function(name='glFogCoordfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='const GLfloat*', wrapParams='1, coord')
)

Function(name='glFogCoordfvEXT', enabled=True, type=Param, inheritFrom='glFogCoordfv')

Function(name='glFogCoordhNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='fog', type='GLhalfNV')
)

Function(name='glFogCoordhvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='fog', type='const GLhalfNV*', wrapParams='1, fog')
)

Function(name='glFogFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='points', type='const GLfloat*')
)

Function(name='glFogf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glFogfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glFogi', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glFogiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*', wrapParams='1, params')
)

Function(name='glFogx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfixed')
)

Function(name='glFogxOES', enabled=True, type=Param, inheritFrom='glFogx')

Function(name='glFogxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfixed*', wrapParams='1, params')
)

Function(name='glFogxvOES', enabled=True, type=Param, inheritFrom='glFogxv')

Function(name='glFragmentColorMaterialSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glFragmentCoverageColorNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='color', type='GLuint')
)

Function(name='glFragmentLightModelfSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glFragmentLightModelfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glFragmentLightModeliSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glFragmentLightModelivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*')
)

Function(name='glFragmentLightfSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glFragmentLightfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glFragmentLightiSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glFragmentLightivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glFragmentMaterialfSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glFragmentMaterialfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glFragmentMaterialiSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glFragmentMaterialivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glFrameTerminatorGREMEDY', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glFrameZoomSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLint')
)

Function(name='glFramebufferDrawBufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebufferEXT'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glFramebufferDrawBuffersEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='bufs', type='const GLenum*')
)

Function(name='glFramebufferFetchBarrierEXT', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glFramebufferFetchBarrierQCOM', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glFramebufferFoveationConfigQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='numLayers', type='GLuint'),
arg3=ArgDef(name='focalPointsPerLayer', type='GLuint'),
arg4=ArgDef(name='requestedFeatures', type='GLuint'),
arg5=ArgDef(name='providedFeatures', type='GLuint*')
)

Function(name='glFramebufferFoveationParametersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='layer', type='GLuint'),
arg3=ArgDef(name='focalPoint', type='GLuint'),
arg4=ArgDef(name='focalX', type='GLfloat'),
arg5=ArgDef(name='focalY', type='GLfloat'),
arg6=ArgDef(name='gainX', type='GLfloat'),
arg7=ArgDef(name='gainY', type='GLfloat'),
arg8=ArgDef(name='foveaArea', type='GLfloat')
)

Function(name='glFramebufferParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glFramebufferPixelLocalStorageSizeEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLuint'),
arg2=ArgDef(name='size', type='GLsizei')
)

Function(name='glFramebufferReadBufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebufferEXT'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glFramebufferRenderbuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='renderbuffertarget', type='GLenum'),
arg4=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glFramebufferRenderbufferEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferRenderbuffer',
arg4=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT')
)

Function(name='glFramebufferRenderbufferOES', enabled=True, type=Param, inheritFrom='glFramebufferRenderbuffer')

Function(name='glFramebufferSampleLocationsfvARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glFramebufferSampleLocationsfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glFramebufferSamplePositionsfvAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='numsamples', type='GLuint'),
arg3=ArgDef(name='pixelindex', type='GLuint'),
arg4=ArgDef(name='values', type='const GLfloat*')
)

Function(name='glFramebufferTexture', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint')
)

Function(name='glFramebufferTexture1D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint')
)

Function(name='glFramebufferTexture1DEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTexture1D')

Function(name='glFramebufferTexture2D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint')
)

Function(name='glFramebufferTexture2DDownsampleIMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint'),
arg6=ArgDef(name='xscale', type='GLint'),
arg7=ArgDef(name='yscale', type='GLint')
)

Function(name='glFramebufferTexture2DEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTexture2D')

Function(name='glFramebufferTexture2DMultisampleEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint'),
arg6=ArgDef(name='samples', type='GLsizei')
)

Function(name='glFramebufferTexture2DMultisampleIMG', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint'),
arg6=ArgDef(name='samples', type='GLsizei')
)

Function(name='glFramebufferTexture2DOES', enabled=True, type=Param, inheritFrom='glFramebufferTexture2D')

Function(name='glFramebufferTexture3D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint')
)

Function(name='glFramebufferTexture3DEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTexture3D')

Function(name='glFramebufferTexture3DOES', enabled=True, type=Param, inheritFrom='glFramebufferTexture3D')

Function(name='glFramebufferTextureARB', enabled=True, type=Param, inheritFrom='glFramebufferTexture')

Function(name='glFramebufferTextureEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTexture')

Function(name='glFramebufferTextureFaceARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='face', type='GLenum')
)

Function(name='glFramebufferTextureFaceEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTextureFaceARB')

Function(name='glFramebufferTextureLayer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='layer', type='GLint')
)

Function(name='glFramebufferTextureLayerARB', enabled=True, type=Param, inheritFrom='glFramebufferTextureLayer')

Function(name='glFramebufferTextureLayerDownsampleIMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='layer', type='GLint'),
arg6=ArgDef(name='xscale', type='GLint'),
arg7=ArgDef(name='yscale', type='GLint')
)

Function(name='glFramebufferTextureLayerEXT', enabled=True, type=Param, stateTrack=True, recWrap=True, inheritFrom='glFramebufferTextureLayer')

Function(name='glFramebufferTextureMultisampleMultiviewOVR', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='samples', type='GLsizei'),
arg6=ArgDef(name='baseViewIndex', type='GLint'),
arg7=ArgDef(name='numViews', type='GLsizei')
)

Function(name='glFramebufferTextureMultiviewOVR', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='baseViewIndex', type='GLint'),
arg6=ArgDef(name='numViews', type='GLsizei')
)

Function(name='glFramebufferTextureOES', enabled=False, type=None, inheritFrom='glFramebufferTexture')

Function(name='glFreeObjectBufferATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glFrontFace', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glFrustum', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='left', type='GLdouble'),
arg2=ArgDef(name='right', type='GLdouble'),
arg3=ArgDef(name='bottom', type='GLdouble'),
arg4=ArgDef(name='top', type='GLdouble'),
arg5=ArgDef(name='zNear', type='GLdouble'),
arg6=ArgDef(name='zFar', type='GLdouble')
)

Function(name='glFrustumf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='l', type='GLfloat'),
arg2=ArgDef(name='r', type='GLfloat'),
arg3=ArgDef(name='b', type='GLfloat'),
arg4=ArgDef(name='t', type='GLfloat'),
arg5=ArgDef(name='n', type='GLfloat'),
arg6=ArgDef(name='f', type='GLfloat')
)

Function(name='glFrustumfOES', enabled=True, type=Param, inheritFrom='glFrustumf')

Function(name='glFrustumx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='l', type='GLfixed'),
arg2=ArgDef(name='r', type='GLfixed'),
arg3=ArgDef(name='b', type='GLfixed'),
arg4=ArgDef(name='t', type='GLfixed'),
arg5=ArgDef(name='n', type='GLfixed'),
arg6=ArgDef(name='f', type='GLfixed')
)

Function(name='glFrustumxOES', enabled=True, type=Param, inheritFrom='glFrustumx')

Function(name='glGenAsyncMarkersSGIX', enabled=True, type=Gen,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='range', type='GLsizei')
)

Function(name='glGenBuffers', enabled=True, type=Gen, stateTrack=True, recCond='ConditionBufferES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='buffers', type='GLuint*', wrapType='CGLBuffer::CSMapArray', wrapParams='n, buffers')
)

Function(name='glGenBuffersARB', enabled=True, type=Gen, inheritFrom='glGenBuffers')

Function(name='glGenFencesAPPLE', enabled=False, type=Gen,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='fences', type='GLuint*')
)

Function(name='glGenFencesNV', enabled=True, type=Gen,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='fences', type='GLuint*', wrapType='CGLfence::CSMapArray', wrapParams='n, fences')
)

Function(name='glGenFragmentShadersATI', enabled=True, type=Gen,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='range', type='GLuint')
)

Function(name='glGenFramebuffers', enabled=True, type=Gen, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='framebuffers', type='GLuint*', wrapType='CGLFramebuffer::CSMapArray', wrapParams='n, framebuffers')
)

Function(name='glGenFramebuffersEXT', enabled=True, type=Gen, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='framebuffers', type='GLuint*', wrapType='CGLFramebufferEXT::CSMapArray', wrapParams='n, framebuffers')
)

Function(name='glGenFramebuffersOES', enabled=True, type=Gen, recWrap=False, inheritFrom='glGenFramebuffersEXT')

Function(name='glGenLists', enabled=True, type=Gen,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='range', type='GLsizei')
)

Function(name='glGenNamesAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='identifier', type='GLenum'),
arg2=ArgDef(name='num', type='GLuint'),
arg3=ArgDef(name='names', type='GLuint*')
)

Function(name='glGenOcclusionQueriesNV', enabled=False, type=Gen|Query, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='GLuint*', wrapParams='n, ids')
)

Function(name='glGenPathsNV', enabled=True, type=Gen,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='range', type='GLsizei')
)

Function(name='glGenPerfMonitorsAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='monitors', type='GLuint*')
)

Function(name='glGenProgramPipelines', enabled=True, type=Gen, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='pipelines', type='GLuint*', wrapType='CGLPipeline::CSMapArray', wrapParams='n, pipelines')
)

Function(name='glGenProgramPipelinesEXT', enabled=True, type=Gen, inheritFrom='glGenProgramPipelines')

Function(name='glGenProgramsARB', enabled=True, type=Gen,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='programs', type='GLuint*', wrapType='CGLProgram::CSMapArray', wrapParams='n, programs')
)

Function(name='glGenProgramsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='programs', type='GLuint*')
)

Function(name='glGenQueries', enabled=True, type=Gen|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='GLuint*', wrapType='CGLQuery::CSMapArray', wrapParams='n, ids')
)

Function(name='glGenQueriesARB', enabled=True, type=Gen|Query, inheritFrom='glGenQueries')

Function(name='glGenQueriesEXT', enabled=False, type=None, inheritFrom='glGenQueries')

Function(name='glGenQueryResourceTagNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='tagIds', type='GLint*')
)

Function(name='glGenRenderbuffers', enabled=True, type=Gen, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='renderbuffers', type='GLuint*', wrapType='CGLRenderbuffer::CSMapArray', wrapParams='n, renderbuffers')
)

Function(name='glGenRenderbuffersEXT', enabled=True, type=Gen, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='renderbuffers', type='GLuint*', wrapType='CGLRenderbufferEXT::CSMapArray', wrapParams='n, renderbuffers')
)

Function(name='glGenRenderbuffersOES', enabled=True, type=Gen, recWrap=False, inheritFrom='glGenRenderbuffersEXT')

Function(name='glGenSamplers', enabled=True, type=Gen, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLsizei'),
arg2=ArgDef(name='samplers', type='GLuint*', wrapType='CGLSampler::CSMapArray', wrapParams='count, samplers')
)

Function(name='glGenSemaphoresEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='semaphores', type='GLuint*')
)

Function(name='glGenSymbolsEXT', enabled=False, type=None,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='datatype', type='GLenum'),
arg2=ArgDef(name='storagetype', type='GLenum'),
arg3=ArgDef(name='range', type='GLenum'),
arg4=ArgDef(name='components', type='GLuint')
)

Function(name='glGenTextures', enabled=True, type=Gen, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='textures', type='GLuint*', wrapType='CGLTexture::CSMapArray', wrapParams='n, textures')
)

Function(name='glGenTexturesEXT', enabled=True, type=Gen, inheritFrom='glGenTextures')

Function(name='glGenTransformFeedbacks', enabled=True, type=Gen,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='ids', type='GLuint*', wrapType='CGLTransformFeedback::CSMapArray', wrapParams='n, ids')
)

Function(name='glGenTransformFeedbacksNV', enabled=True, type=Gen, inheritFrom='glGenTransformFeedbacks')

Function(name='glGenVertexArrays', enabled=True, type=Gen, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='arrays', type='GLuint*', wrapType='CGLVertexArray::CSMapArray', wrapParams='n, arrays')
)

Function(name='glGenVertexArraysAPPLE', enabled=False, type=Gen,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='arrays', type='GLuint*')
)

Function(name='glGenVertexArraysOES', enabled=True, type=Gen, inheritFrom='glGenVertexArrays')

Function(name='glGenVertexShadersEXT', enabled=True, type=Gen,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='range', type='GLuint')
)

Function(name='glGenerateMipmap', enabled=True, type=Param, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glGenerateMipmapEXT', enabled=True, type=Param, recWrap=True, inheritFrom='glGenerateMipmap')

Function(name='glGenerateMipmapOES', enabled=True, type=Param, inheritFrom='glGenerateMipmap')

Function(name='glGenerateMultiTexMipmapEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum')
)

Function(name='glGenerateTextureMipmap', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint')
)

Function(name='glGenerateTextureMipmapEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum')
)

Function(name='glGetActiveAtomicCounterBufferiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='bufferIndex', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetActiveAttrib', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg5=ArgDef(name='size', type='GLint*', wrapType='COutArgument'),
arg6=ArgDef(name='type', type='GLenum*', wrapType='COutArgument'),
arg7=ArgDef(name='name', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveAttribARB', enabled=True, type=Get, inheritFrom='glGetActiveAttrib',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg7=ArgDef(name='name', type='GLcharARB*', wrapType='COutArgument')
)

Function(name='glGetActiveSubroutineName', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='bufsize', type='GLsizei'),
arg5=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg6=ArgDef(name='name', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveSubroutineUniformName', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='bufsize', type='GLsizei'),
arg5=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg6=ArgDef(name='name', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveSubroutineUniformiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='values', type='GLint*')
)

Function(name='glGetActiveUniform', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg5=ArgDef(name='size', type='GLint*', wrapType='COutArgument'),
arg6=ArgDef(name='type', type='GLenum*', wrapType='COutArgument'),
arg7=ArgDef(name='name', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveUniformARB', enabled=True, type=Get, inheritFrom='glGetActiveUniform',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg7=ArgDef(name='name', type='GLcharARB*', wrapType='COutArgument')
)

Function(name='glGetActiveUniformBlockName', enabled=True, type=Get, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformBlockIndex', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*', wrapParams='1, length'),
arg5=ArgDef(name='uniformBlockName', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveUniformBlockiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformBlockIndex', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetActiveUniformName', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformIndex', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg5=ArgDef(name='uniformName', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetActiveUniformsiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformCount', type='GLsizei'),
arg3=ArgDef(name='uniformIndices', type='const GLuint*', wrapParams='uniformCount, uniformIndices'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetActiveVaryingNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='size', type='GLsizei*'),
arg6=ArgDef(name='type', type='GLenum*'),
arg7=ArgDef(name='name', type='GLchar*')
)

Function(name='glGetArrayObjectfvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetArrayObjectivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='array', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetAttachedObjectsARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='containerObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='maxCount', type='GLsizei'),
arg3=ArgDef(name='count', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='obj', type='GLhandleARB*', wrapType='COutArgument')
)

Function(name='glGetAttachedShaders', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='maxCount', type='GLsizei'),
arg3=ArgDef(name='count', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='shaders', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetAttribLocation', enabled=True, type=Get,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetAttribLocationARB', enabled=True, type=Get, inheritFrom='glGetAttribLocation',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLcharARB*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetBooleanIndexedvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLboolean*')
)

Function(name='glGetBooleani_v', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLboolean*', wrapType='COutArgument')
)

Function(name='glGetBooleanv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLboolean*', wrapType='COutArgument')
)

Function(name='glGetBufferParameteri64v', enabled=True, type=Get, inheritFrom='glGetBufferParameteriv',
arg3=ArgDef(name='params', type='GLint64*', wrapType='COutArgument')
)

Function(name='glGetBufferParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetBufferParameterivARB', enabled=True, type=Get, inheritFrom='glGetBufferParameteriv')

Function(name='glGetBufferParameterui64vNV', enabled=True, type=Get, inheritFrom='glGetBufferParameteriv',
arg3=ArgDef(name='params', type='GLuint64EXT*', wrapType='COutArgument')
)

Function(name='glGetBufferPointerv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='void**', wrapType='COutArgument')
)

Function(name='glGetBufferPointervARB', enabled=True, type=Get, inheritFrom='glGetBufferPointerv')

Function(name='glGetBufferPointervOES', enabled=True, type=Get, inheritFrom='glGetBufferPointerv')

Function(name='glGetBufferSubData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='data', type='void*')
)

Function(name='glGetBufferSubDataARB', enabled=False, type=None, inheritFrom='glGetBufferSubData',
arg2=ArgDef(name='offset', type='GLintptrARB'),
arg3=ArgDef(name='size', type='GLsizeiptrARB'),
)

Function(name='glGetClipPlane', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='plane', type='GLenum'),
arg2=ArgDef(name='equation', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetClipPlanef', enabled=True, type=Get, inheritFrom='glGetClipPlane',
arg2=ArgDef(name='eqn', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetClipPlanefOES', enabled=True, type=Get, inheritFrom='glGetClipPlane',
arg2=ArgDef(name='eqn', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetClipPlanex', enabled=True, type=Get, inheritFrom='glGetClipPlane',
arg2=ArgDef(name='eqn', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetClipPlanexOES', enabled=True, type=Get, inheritFrom='glGetClipPlane',
arg2=ArgDef(name='eqn', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetColorTable', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='table', type='void*')
)

Function(name='glGetColorTableEXT', enabled=False, type=None, inheritFrom='glGetColorTable')

Function(name='glGetColorTableParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetColorTableParameterfvEXT', enabled=True, type=Get, inheritFrom='glGetColorTableParameterfv')

Function(name='glGetColorTableParameterfvSGI', enabled=True, type=Get, inheritFrom='glGetColorTableParameterfv')

Function(name='glGetColorTableParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetColorTableParameterivEXT', enabled=True, type=Get, inheritFrom='glGetColorTableParameteriv')

Function(name='glGetColorTableParameterivSGI', enabled=True, type=Get, inheritFrom='glGetColorTableParameteriv')

Function(name='glGetColorTableSGI', enabled=False, type=None, inheritFrom='glGetColorTable')

Function(name='glGetCombinerInputParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='variable', type='GLenum'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetCombinerInputParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='variable', type='GLenum'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLint*')
)

Function(name='glGetCombinerOutputParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetCombinerOutputParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='portion', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetCombinerStageParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stage', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetCommandHeaderNV', enabled=False, type=None,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='tokenID', type='GLenum'),
arg2=ArgDef(name='size', type='GLuint')
)

Function(name='glGetCompressedMultiTexImageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='lod', type='GLint'),
arg4=ArgDef(name='img', type='void*')
)

Function(name='glGetCompressedTexImage', enabled=True, type=Get, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='img', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetCompressedTexImageARB', enabled=False, type=None, inheritFrom='glGetCompressedTexImage')

Function(name='glGetCompressedTextureImage', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='pixels', type='void*')
)

Function(name='glGetCompressedTextureImageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='lod', type='GLint'),
arg4=ArgDef(name='img', type='GLvoid*')
)

Function(name='glGetCompressedTextureSubImage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='bufSize', type='GLsizei'),
arg10=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetConvolutionFilter', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='image', type='void*')
)

Function(name='glGetConvolutionFilterEXT', enabled=False, type=None, inheritFrom='glGetConvolutionFilter')

Function(name='glGetConvolutionParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetConvolutionParameterfvEXT', enabled=True, type=Get, inheritFrom='glGetConvolutionParameterfv')

Function(name='glGetConvolutionParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetConvolutionParameterivEXT', enabled=True, type=Get, inheritFrom='glGetConvolutionParameteriv')

Function(name='glGetConvolutionParameterxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*')
)

Function(name='glGetCoverageModulationTableNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='bufsize', type='GLsizei'),
arg2=ArgDef(name='v', type='GLfloat*')
)

Function(name='glGetDebugMessageLog', enabled=False, type=None,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='count', type='GLuint'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='sources', type='GLenum*'),
arg4=ArgDef(name='types', type='GLenum*'),
arg5=ArgDef(name='ids', type='GLuint*'),
arg6=ArgDef(name='severities', type='GLenum*'),
arg7=ArgDef(name='lengths', type='GLsizei*'),
arg8=ArgDef(name='messageLog', type='GLchar*')
)

Function(name='glGetDebugMessageLogAMD', enabled=False, type=None,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='count', type='GLuint'),
arg2=ArgDef(name='bufsize', type='GLsizei'),
arg3=ArgDef(name='categories', type='GLenum*'),
arg4=ArgDef(name='severities', type='GLuint*'),
arg5=ArgDef(name='ids', type='GLuint*'),
arg6=ArgDef(name='lengths', type='GLsizei*'),
arg7=ArgDef(name='message', type='GLchar*')
)

Function(name='glGetDebugMessageLogARB', enabled=False, type=None, inheritFrom='glGetDebugMessageLog')

Function(name='glGetDebugMessageLogKHR', enabled=False, type=None, inheritFrom='glGetDebugMessageLog')

Function(name='glGetDetailTexFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='points', type='GLfloat*')
)

Function(name='glGetDoubleIndexedvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLdouble*')
)

Function(name='glGetDoublei_v', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLdouble*')
)

Function(name='glGetDoublei_vEXT', enabled=False, type=None, inheritFrom='glGetDoublei_v')

Function(name='glGetDoublev', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetDriverControlStringQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='driverControl', type='GLuint'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='driverControlString', type='GLchar*')
)

Function(name='glGetDriverControlsQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='num', type='GLint*'),
arg2=ArgDef(name='size', type='GLsizei'),
arg3=ArgDef(name='driverControls', type='GLuint*')
)

Function(name='glGetError', enabled=True, type=Get,
retV=RetDef(type='GLenum')
)

Function(name='glGetFenceivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='fence', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFinalCombinerInputParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='variable', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetFinalCombinerInputParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='variable', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFirstPerfQueryIdINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryId', type='GLuint*')
)

Function(name='glGetFixedv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='GLfixed*')
)

Function(name='glGetFixedvOES', enabled=False, type=None, inheritFrom='glGetFixedv')

Function(name='glGetFloatIndexedvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLfloat*')
)

Function(name='glGetFloati_v', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLfloat*')
)

Function(name='glGetFloati_vEXT', enabled=False, type=None, inheritFrom='glGetFloati_v')

Function(name='glGetFloati_vNV', enabled=False, type=None, inheritFrom='glGetFloati_v')

Function(name='glGetFloati_vOES', enabled=False, type=None, inheritFrom='glGetFloati_v')

Function(name='glGetFloatv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetFogFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='points', type='GLfloat*')
)

Function(name='glGetFragDataIndex', enabled=True, type=Get,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetFragDataIndexEXT', enabled=False, type=None, inheritFrom='glGetFragDataIndex')

Function(name='glGetFragDataLocation', enabled=True, type=Get,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetFragDataLocationEXT', enabled=True, type=Get, inheritFrom='glGetFragDataLocation')

Function(name='glGetFragmentLightfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetFragmentLightivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFragmentMaterialfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetFragmentMaterialivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFramebufferAttachmentParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetFramebufferAttachmentParameterivEXT', enabled=True, type=Get, inheritFrom='glGetFramebufferAttachmentParameteriv')

Function(name='glGetFramebufferAttachmentParameterivOES', enabled=True, type=Get, inheritFrom='glGetFramebufferAttachmentParameteriv')

Function(name='glGetFramebufferParameterfvAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='numsamples', type='GLuint'),
arg4=ArgDef(name='pixelindex', type='GLuint'),
arg5=ArgDef(name='size', type='GLsizei'),
arg6=ArgDef(name='values', type='GLfloat*')
)

Function(name='glGetFramebufferParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFramebufferParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetFramebufferPixelLocalStorageSizeEXT', enabled=False, type=None,
retV=RetDef(type='GLsizei'),
arg1=ArgDef(name='target', type='GLuint')
)

Function(name='glGetGraphicsResetStatus', enabled=False, type=None,
retV=RetDef(type='GLenum')
)

Function(name='glGetGraphicsResetStatusARB', enabled=True, type=Get, inheritFrom='glGetGraphicsResetStatus')

Function(name='glGetGraphicsResetStatusEXT', enabled=True, type=Get, inheritFrom='glGetGraphicsResetStatus')

Function(name='glGetGraphicsResetStatusKHR', enabled=False, type=None, inheritFrom='glGetGraphicsResetStatus')

Function(name='glGetHandleARB', enabled=True, type=Get,
retV=RetDef(type='GLhandleARB'),
arg1=ArgDef(name='pname', type='GLenum')
)

Function(name='glGetHistogram', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='reset', type='GLboolean'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='values', type='void*')
)

Function(name='glGetHistogramEXT', enabled=False, type=None, inheritFrom='glGetHistogram')

Function(name='glGetHistogramParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetHistogramParameterfvEXT', enabled=True, type=Get, inheritFrom='glGetHistogramParameterfv')

Function(name='glGetHistogramParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetHistogramParameterivEXT', enabled=True, type=Get, inheritFrom='glGetHistogramParameteriv')

Function(name='glGetHistogramParameterxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*')
)

Function(name='glGetImageHandleARB', enabled=True, type=Get,
retV=RetDef(type='GLuint64', wrapType='CGLTextureHandle'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='layered', type='GLboolean'),
arg4=ArgDef(name='layer', type='GLint'),
arg5=ArgDef(name='format', type='GLenum')
)

Function(name='glGetImageHandleNV', enabled=True, type=Get,
retV=RetDef(type='GLuint64'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='layered', type='GLboolean'),
arg4=ArgDef(name='layer', type='GLint'),
arg5=ArgDef(name='format', type='GLenum')
)

Function(name='glGetImageTransformParameterfvHP', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetImageTransformParameterivHP', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetInfoLogARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='maxLength', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='infoLog', type='GLcharARB*', wrapType='COutArgument')
)

Function(name='glGetInstrumentsSGIX', enabled=True, type=Get,
retV=RetDef(type='GLint')
)

Function(name='glGetInteger64i_v', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLint64*', wrapType='COutArgument')
)

Function(name='glGetInteger64v', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLint64*', wrapType='COutArgument')
)

Function(name='glGetInteger64vAPPLE', enabled=False, type=Get, inheritFrom='glGetInteger64v')

Function(name='glGetIntegerIndexedvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLint*')
)

Function(name='glGetIntegeri_v', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetIntegeri_vEXT', enabled=True, type=Get, inheritFrom='glGetIntegeri_v')

Function(name='glGetIntegerui64i_vNV', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='result', type='GLuint64EXT*', wrapType='COutArgument')
)

Function(name='glGetIntegerui64vNV', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLenum'),
arg2=ArgDef(name='result', type='GLuint64EXT*', wrapType='COutArgument')
)

Function(name='glGetIntegerv', enabled=True, type=Get, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetInternalformatSampleivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='samples', type='GLsizei'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='bufSize', type='GLsizei'),
arg6=ArgDef(name='params', type='GLint*')
)

Function(name='glGetInternalformati64v', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='bufSize', type='GLsizei'),
arg5=ArgDef(name='params', type='GLint64*', wrapType='COutArgument')
)

Function(name='glGetInternalformativ', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='bufSize', type='GLsizei'),
arg5=ArgDef(name='params', type='GLint*')
)

Function(name='glGetInvariantBooleanvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLboolean*')
)

Function(name='glGetInvariantFloatvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLfloat*')
)

Function(name='glGetInvariantIntegervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLint*')
)

Function(name='glGetLightfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetLightiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetLightxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*')
)

Function(name='glGetLightxv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetLightxvOES', enabled=True, type=Get, inheritFrom='glGetLightxv')

Function(name='glGetListParameterfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetListParameterivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetLocalConstantBooleanvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLboolean*')
)

Function(name='glGetLocalConstantFloatvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLfloat*')
)

Function(name='glGetLocalConstantIntegervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLint*')
)

Function(name='glGetMapAttribParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMapAttribParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMapControlPointsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='ustride', type='GLsizei'),
arg5=ArgDef(name='vstride', type='GLsizei'),
arg6=ArgDef(name='packed', type='GLboolean'),
arg7=ArgDef(name='points', type='void*')
)

Function(name='glGetMapParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMapParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMapdv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='v', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetMapfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='v', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetMapiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='v', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetMapxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='v', type='GLfixed*')
)

Function(name='glGetMaterialfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetMaterialiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetMaterialxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glGetMaterialxv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetMaterialxvOES', enabled=True, type=Get, inheritFrom='glGetMaterialxv')

Function(name='glGetMemoryObjectParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='memoryObject', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMinmax', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='reset', type='GLboolean'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='values', type='void*')
)

Function(name='glGetMinmaxEXT', enabled=False, type=None, inheritFrom='glGetMinmax')

Function(name='glGetMinmaxParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetMinmaxParameterfvEXT', enabled=True, type=Get, inheritFrom='glGetMinmaxParameterfv')

Function(name='glGetMinmaxParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetMinmaxParameterivEXT', enabled=True, type=Get, inheritFrom='glGetMinmaxParameteriv')

Function(name='glGetMultiTexEnvfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMultiTexEnvivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMultiTexGendvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetMultiTexGenfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMultiTexGenivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMultiTexImageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='format', type='GLenum'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='pixels', type='void*')
)

Function(name='glGetMultiTexLevelParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMultiTexLevelParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMultiTexParameterIivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMultiTexParameterIuivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetMultiTexParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetMultiTexParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetMultisamplefv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='val', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetMultisamplefvNV', enabled=True, type=Get, inheritFrom='glGetMultisamplefv')

Function(name='glGetNamedBufferParameteri64v', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint64*')
)

Function(name='glGetNamedBufferParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetNamedBufferParameterivEXT', enabled=False, type=None, inheritFrom='glGetNamedBufferParameteriv')

Function(name='glGetNamedBufferParameterui64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64EXT*')
)

Function(name='glGetNamedBufferPointerv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='void**', wrapType='COutArgument')
)

Function(name='glGetNamedBufferPointervEXT', enabled=True, type=Get, inheritFrom='glGetNamedBufferPointerv')

Function(name='glGetNamedBufferSubData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='data', type='void*')
)

Function(name='glGetNamedBufferSubDataEXT', enabled=False, type=None, inheritFrom='glGetNamedBufferSubData')

Function(name='glGetNamedFramebufferAttachmentParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetNamedFramebufferAttachmentParameterivEXT', enabled=False, type=None, inheritFrom='glGetNamedFramebufferAttachmentParameteriv')

Function(name='glGetNamedFramebufferParameterfvAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='numsamples', type='GLuint'),
arg4=ArgDef(name='pixelindex', type='GLuint'),
arg5=ArgDef(name='size', type='GLsizei'),
arg6=ArgDef(name='values', type='GLfloat*')
)

Function(name='glGetNamedFramebufferParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint*')
)

Function(name='glGetNamedFramebufferParameterivEXT', enabled=False, type=None, inheritFrom='glGetNamedFramebufferParameteriv')

Function(name='glGetNamedProgramLocalParameterIivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetNamedProgramLocalParameterIuivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetNamedProgramLocalParameterdvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetNamedProgramLocalParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetNamedProgramStringEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='string', type='void*')
)

Function(name='glGetNamedProgramivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetNamedRenderbufferParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetNamedRenderbufferParameterivEXT', enabled=False, type=None, inheritFrom='glGetNamedRenderbufferParameteriv',
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT')
)

Function(name='glGetNamedStringARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='namelen', type='GLint'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='stringlen', type='GLint*'),
arg5=ArgDef(name='string', type='GLchar*')
)

Function(name='glGetNamedStringivARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='namelen', type='GLint'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetNextPerfQueryIdINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryId', type='GLuint'),
arg2=ArgDef(name='nextQueryId', type='GLuint*')
)

Function(name='glGetObjectBufferfvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetObjectBufferivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetObjectLabel', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='identifier', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='label', type='GLchar*')
)

Function(name='glGetObjectLabelEXT', enabled=False, type=None, inheritFrom='glGetObjectLabel')

Function(name='glGetObjectLabelKHR', enabled=False, type=None, inheritFrom='glGetObjectLabel')

Function(name='glGetObjectParameterfvARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetObjectParameterivAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='objectType', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetObjectParameterivARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetObjectPtrLabel', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='ptr', type='const void*'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='label', type='GLchar*')
)

Function(name='glGetObjectPtrLabelKHR', enabled=False, type=None, inheritFrom='glGetObjectPtrLabel')

Function(name='glGetOcclusionQueryivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetOcclusionQueryuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetPathColorGenfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='color', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLfloat*')
)

Function(name='glGetPathColorGenivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='color', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLint*')
)

Function(name='glGetPathCommandsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='commands', type='GLubyte*')
)

Function(name='glGetPathCoordsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='coords', type='GLfloat*')
)

Function(name='glGetPathDashArrayNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='dashArray', type='GLfloat*')
)

Function(name='glGetPathLengthNV', enabled=True, type=Get,
retV=RetDef(type='GLfloat'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='startSegment', type='GLsizei'),
arg3=ArgDef(name='numSegments', type='GLsizei')
)

Function(name='glGetPathMetricRangeNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='metricQueryMask', type='GLbitfield'),
arg2=ArgDef(name='firstPathName', type='GLuint'),
arg3=ArgDef(name='numPaths', type='GLsizei'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='metrics', type='GLfloat*')
)

Function(name='glGetPathMetricsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='metricQueryMask', type='GLbitfield'),
arg2=ArgDef(name='numPaths', type='GLsizei'),
arg3=ArgDef(name='pathNameType', type='GLenum'),
arg4=ArgDef(name='paths', type='const void*'),
arg5=ArgDef(name='pathBase', type='GLuint'),
arg6=ArgDef(name='stride', type='GLsizei'),
arg7=ArgDef(name='metrics', type='GLfloat*')
)

Function(name='glGetPathParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLfloat*')
)

Function(name='glGetPathParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLint*')
)

Function(name='glGetPathSpacingNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pathListMode', type='GLenum'),
arg2=ArgDef(name='numPaths', type='GLsizei'),
arg3=ArgDef(name='pathNameType', type='GLenum'),
arg4=ArgDef(name='paths', type='const void*'),
arg5=ArgDef(name='pathBase', type='GLuint'),
arg6=ArgDef(name='advanceScale', type='GLfloat'),
arg7=ArgDef(name='kerningScale', type='GLfloat'),
arg8=ArgDef(name='transformType', type='GLenum'),
arg9=ArgDef(name='returnedSpacing', type='GLfloat*')
)

Function(name='glGetPathTexGenfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texCoordSet', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLfloat*')
)

Function(name='glGetPathTexGenivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texCoordSet', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLint*')
)

Function(name='glGetPerfCounterInfoINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryId', type='GLuint'),
arg2=ArgDef(name='counterId', type='GLuint'),
arg3=ArgDef(name='counterNameLength', type='GLuint'),
arg4=ArgDef(name='counterName', type='GLchar*'),
arg5=ArgDef(name='counterDescLength', type='GLuint'),
arg6=ArgDef(name='counterDesc', type='GLchar*'),
arg7=ArgDef(name='counterOffset', type='GLuint*'),
arg8=ArgDef(name='counterDataSize', type='GLuint*'),
arg9=ArgDef(name='counterTypeEnum', type='GLuint*'),
arg10=ArgDef(name='counterDataTypeEnum', type='GLuint*'),
arg11=ArgDef(name='rawCounterMaxValue', type='GLuint64*')
)

Function(name='glGetPerfMonitorCounterDataAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='monitor', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='dataSize', type='GLsizei'),
arg4=ArgDef(name='data', type='GLuint*'),
arg5=ArgDef(name='bytesWritten', type='GLint*')
)

Function(name='glGetPerfMonitorCounterInfoAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='group', type='GLuint'),
arg2=ArgDef(name='counter', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='data', type='void*')
)

Function(name='glGetPerfMonitorCounterStringAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='group', type='GLuint'),
arg2=ArgDef(name='counter', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='counterString', type='GLchar*')
)

Function(name='glGetPerfMonitorCountersAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='group', type='GLuint'),
arg2=ArgDef(name='numCounters', type='GLint*'),
arg3=ArgDef(name='maxActiveCounters', type='GLint*'),
arg4=ArgDef(name='counterSize', type='GLsizei'),
arg5=ArgDef(name='counters', type='GLuint*')
)

Function(name='glGetPerfMonitorGroupStringAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='group', type='GLuint'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='groupString', type='GLchar*')
)

Function(name='glGetPerfMonitorGroupsAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numGroups', type='GLint*'),
arg2=ArgDef(name='groupsSize', type='GLsizei'),
arg3=ArgDef(name='groups', type='GLuint*')
)

Function(name='glGetPerfQueryDataINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryHandle', type='GLuint'),
arg2=ArgDef(name='flags', type='GLuint'),
arg3=ArgDef(name='dataSize', type='GLsizei'),
arg4=ArgDef(name='data', type='void*'),
arg5=ArgDef(name='bytesWritten', type='GLuint*')
)

Function(name='glGetPerfQueryIdByNameINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryName', type='GLchar*'),
arg2=ArgDef(name='queryId', type='GLuint*')
)

Function(name='glGetPerfQueryInfoINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='queryId', type='GLuint'),
arg2=ArgDef(name='queryNameLength', type='GLuint'),
arg3=ArgDef(name='queryName', type='GLchar*'),
arg4=ArgDef(name='dataSize', type='GLuint*'),
arg5=ArgDef(name='noCounters', type='GLuint*'),
arg6=ArgDef(name='noInstances', type='GLuint*'),
arg7=ArgDef(name='capsMask', type='GLuint*')
)

Function(name='glGetPixelMapfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='values', type='GLfloat*')
)

Function(name='glGetPixelMapuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='values', type='GLuint*')
)

Function(name='glGetPixelMapusv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='values', type='GLushort*')
)

Function(name='glGetPixelMapxv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='values', type='GLfixed*')
)

Function(name='glGetPixelTexGenParameterfvSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetPixelTexGenParameterivSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='GLint*')
)

Function(name='glGetPixelTransformParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetPixelTransformParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetPointerIndexedvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='void**')
)

Function(name='glGetPointeri_vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='void**')
)

Function(name='glGetPointerv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='void**', wrapType='COutArgument')
)

Function(name='glGetPointervEXT', enabled=True, type=Get, inheritFrom='glGetPointerv')

Function(name='glGetPointervKHR', enabled=False, type=None, inheritFrom='glGetPointerv')

Function(name='glGetPolygonStipple', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLubyte*', wrapType='COutArgument')
)

Function(name='glGetProgramBinary', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='binaryFormat', type='GLenum*'),
arg5=ArgDef(name='binary', type='void*')
)

Function(name='glGetProgramBinaryOES', enabled=False, type=None, inheritFrom='glGetProgramBinary')

Function(name='glGetProgramEnvParameterIivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetProgramEnvParameterIuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetProgramEnvParameterdvARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetProgramEnvParameterfvARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetProgramInfoLog', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='infoLog', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetProgramInterfaceiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetProgramLocalParameterIivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetProgramLocalParameterIuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetProgramLocalParameterdvARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetProgramLocalParameterfvARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetProgramNamedParameterdvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetProgramNamedParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetProgramParameterdvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetProgramParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetProgramPipelineInfoLog', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='infoLog', type='GLchar*')
)

Function(name='glGetProgramPipelineInfoLogEXT', enabled=False, type=None, inheritFrom='glGetProgramPipelineInfoLog')

Function(name='glGetProgramPipelineiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetProgramPipelineivEXT', enabled=False, type=None, inheritFrom='glGetProgramPipelineiv')

Function(name='glGetProgramResourceIndex', enabled=True, type=Get, runWrap=True, ccodeWrap=True,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetProgramResourceLocation', enabled=True, type=GetEssential, runWrap=True,
retV=RetDef(type='GLint', wrapType='CRecUniformLocation', wrapParams='programInterface == GL_UNIFORM ? return_value : -1, program, name'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetProgramResourceLocationIndex', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetProgramResourceLocationIndexEXT', enabled=False, type=None, inheritFrom='glGetProgramResourceLocationIndex')

Function(name='glGetProgramResourceName', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='bufSize', type='GLsizei'),
arg5=ArgDef(name='length', type='GLsizei*'),
arg6=ArgDef(name='name', type='GLchar*')
)

Function(name='glGetProgramResourcefvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='propCount', type='GLsizei'),
arg5=ArgDef(name='props', type='const GLenum*'),
arg6=ArgDef(name='bufSize', type='GLsizei'),
arg7=ArgDef(name='length', type='GLsizei*'),
arg8=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetProgramResourceiv', enabled=True, type=GetEssential, runWrap=True, ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='programInterface', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint', wrapType='CGLResourceIndex', wrapParams='program, programInterface, index'),
arg4=ArgDef(name='propCount', type='GLsizei'),
arg5=ArgDef(name='props', type='const GLenum*', wrapParams='propCount, props'),
arg6=ArgDef(name='bufSize', type='GLsizei'),
arg7=ArgDef(name='length', type='GLsizei*', wrapParams='1, length'), # TODO: This is an output sizei, not an input array.
arg8=ArgDef(name='params', type='GLint*', wrapType='CGLProgramResourceivHelper', wrapParams='program, programInterface, index, propCount, props, bufSize, params')
)

Function(name='glGetProgramStageiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='values', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetProgramStringARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='string', type='void*')
)

Function(name='glGetProgramStringNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='program', type='GLubyte*')
)

Function(name='glGetProgramSubroutineParameteruivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='param', type='GLuint*')
)

Function(name='glGetProgramiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetProgramivARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetProgramivNV', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetQueryBufferObjecti64v', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glGetQueryBufferObjectiv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glGetQueryBufferObjectui64v', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glGetQueryBufferObjectuiv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glGetQueryIndexediv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetQueryObjecti64v', enabled=True, type=Get|Query, runCond='ConditionQueries()', 
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint64*', wrapType='COutArgument')
)

Function(name='glGetQueryObjecti64vEXT', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjecti64v',
arg3=ArgDef(name='params', type='GLint64EXT*', wrapType='COutArgument')
)

Function(name='glGetQueryObjecti64vINTEL', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjecti64v')

Function(name='glGetQueryObjectiv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetQueryObjectivARB', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjectiv')

Function(name='glGetQueryObjectivEXT', enabled=False, type=None, inheritFrom='glGetQueryObjectiv')

Function(name='glGetQueryObjectui64v', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64*', wrapType='COutArgument')
)

Function(name='glGetQueryObjectui64vEXT', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64EXT*', wrapType='COutArgument')
)

Function(name='glGetQueryObjectui64vINTEL', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjectui64v')

Function(name='glGetQueryObjectuiv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetQueryObjectuivARB', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjectuiv')

Function(name='glGetQueryObjectuivEXT', enabled=True, type=Get|Query, inheritFrom='glGetQueryObjectuiv')

Function(name='glGetQueryiv', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetQueryivARB', enabled=True, type=Get|Query, inheritFrom='glGetQueryiv')

Function(name='glGetQueryivEXT', enabled=False, type=None, inheritFrom='glGetQueryiv')

Function(name='glGetRenderbufferParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetRenderbufferParameterivEXT', enabled=True, type=Get, inheritFrom='glGetRenderbufferParameteriv')

Function(name='glGetRenderbufferParameterivOES', enabled=True, type=Get, inheritFrom='glGetRenderbufferParameteriv')

Function(name='glGetSamplerParameterIiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetSamplerParameterIivEXT', enabled=False, type=None, inheritFrom='glGetSamplerParameterIiv')

Function(name='glGetSamplerParameterIivOES', enabled=False, type=None, inheritFrom='glGetSamplerParameterIiv')

Function(name='glGetSamplerParameterIuiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetSamplerParameterIuivEXT', enabled=False, type=None, inheritFrom='glGetSamplerParameterIuiv')

Function(name='glGetSamplerParameterIuivOES', enabled=False, type=None, inheritFrom='glGetSamplerParameterIuiv')

Function(name='glGetSamplerParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetSamplerParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetSemaphoreParameterui64vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64*')
)

Function(name='glGetSeparableFilter', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='row', type='void*'),
arg5=ArgDef(name='column', type='void*'),
arg6=ArgDef(name='span', type='void*')
)

Function(name='glGetSeparableFilterEXT', enabled=False, type=None, inheritFrom='glGetSeparableFilter')

Function(name='glGetShaderInfoLog', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='infoLog', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetShaderPrecisionFormat', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='shadertype', type='GLenum'),
arg2=ArgDef(name='precisiontype', type='GLenum'),
arg3=ArgDef(name='range', type='GLint*', wrapType='COutArgument'),
arg4=ArgDef(name='precision', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetShaderSource', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='source', type='GLchar*', wrapType='COutArgument')
)

Function(name='glGetShaderSourceARB', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='obj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='maxLength', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*', wrapType='COutArgument'),
arg4=ArgDef(name='source', type='GLcharARB*', wrapType='COutArgument')
)

Function(name='glGetShaderiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetSharpenTexFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='points', type='GLfloat*')
)

Function(name='glGetStageIndexNV', enabled=False, type=None,
retV=RetDef(type='GLushort'),
arg1=ArgDef(name='shadertype', type='GLenum')
)

Function(name='glGetString', enabled=True, type=Get, interceptorExecOverride=True,
retV=RetDef(type='const GLubyte*', wrapType='CGLconstuchar_ptr'),
arg1=ArgDef(name='name', type='GLenum')
)

Function(name='glGetStringi', enabled=True, type=Get, interceptorExecOverride=True,
retV=RetDef(type='const GLubyte*', wrapType='CGLconstuchar_ptr'),
arg1=ArgDef(name='name', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glGetSubroutineIndex', enabled=True, type=Get, runWrap=True, ccodeWrap=True,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetSubroutineUniformLocation', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='shadertype', type='GLenum'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetSynciv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='sync', type='GLsync'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='values', type='GLint*')
)

Function(name='glGetSyncivAPPLE', enabled=False, type=None, inheritFrom='glGetSynciv')

Function(name='glGetTexBumpParameterfvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat*')
)

Function(name='glGetTexBumpParameterivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint*')
)

Function(name='glGetTexEnvfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetTexEnviv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetTexEnvxv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetTexEnvxvOES', enabled=True, type=Get, inheritFrom='glGetTexEnvxv')

Function(name='glGetTexFilterFuncSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='filter', type='GLenum'),
arg3=ArgDef(name='weights', type='GLfloat*')
)

Function(name='glGetTexGendv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetTexGenfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetTexGenfvOES', enabled=True, type=Get, inheritFrom='glGetTexGenfv')

Function(name='glGetTexGeniv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetTexGenivOES', enabled=True, type=Get, inheritFrom='glGetTexGeniv')

Function(name='glGetTexGenxvOES', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetTexImage', enabled=True, type=Get, runWrap=True, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetTexLevelParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetTexLevelParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetTexLevelParameterxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfixed*')
)

Function(name='glGetTexParameterIiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetTexParameterIivEXT', enabled=True, type=Get, inheritFrom='glGetTexParameterIiv')

Function(name='glGetTexParameterIivOES', enabled=False, type=None, inheritFrom='glGetTexParameterIiv')

Function(name='glGetTexParameterIuiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetTexParameterIuivEXT', enabled=True, type=Get, inheritFrom='glGetTexParameterIuiv')

Function(name='glGetTexParameterIuivOES', enabled=False, type=None, inheritFrom='glGetTexParameterIuiv')

Function(name='glGetTexParameterPointervAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='void**')
)

Function(name='glGetTexParameterfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetTexParameteriv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetTexParameterxv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfixed*', wrapType='COutArgument')
)

Function(name='glGetTexParameterxvOES', enabled=True, type=Get, inheritFrom='glGetTexParameterxv')

Function(name='glGetTextureHandleARB', enabled=True, type=Get,
retV=RetDef(type='GLuint64', wrapType='CGLTextureHandle'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glGetTextureHandleIMG', enabled=False, type=None,
retV=RetDef(type='GLuint64'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glGetTextureHandleNV', enabled=False, type=None,
retV=RetDef(type='GLuint64'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glGetTextureImage', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='bufSize', type='GLsizei'),
arg6=ArgDef(name='pixels', type='void*')
)

Function(name='glGetTextureImageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='format', type='GLenum'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='pixels', type='GLvoid*')
)

Function(name='glGetTextureLevelParameterfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetTextureLevelParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetTextureLevelParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureLevelParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='pname', type='GLenum'),
arg5=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureParameterIiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureParameterIivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureParameterIuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetTextureParameterIuivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetTextureParameterfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetTextureParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetTextureParameteriv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTextureSamplerHandleARB', enabled=True, type=Get,
retV=RetDef(type='GLuint64', wrapType='CGLTextureHandle'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler')
)

Function(name='glGetTextureSamplerHandleIMG', enabled=False, type=None,
retV=RetDef(type='GLuint64'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler')
)

Function(name='glGetTextureSamplerHandleNV', enabled=False, type=None,
retV=RetDef(type='GLuint64'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler')
)

Function(name='glGetTextureSubImage', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='bufSize', type='GLsizei'),
arg12=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetTrackMatrixivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='address', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetTransformFeedbackVarying', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='size', type='GLsizei*'),
arg6=ArgDef(name='type', type='GLenum*'),
arg7=ArgDef(name='name', type='GLchar*')
)

Function(name='glGetTransformFeedbackVaryingEXT', enabled=False, type=None, inheritFrom='glGetTransformFeedbackVarying')

Function(name='glGetTransformFeedbackVaryingNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='location', type='GLint*')
)

Function(name='glGetTransformFeedbacki64_v', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfb', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='param', type='GLint64*')
)

Function(name='glGetTransformFeedbacki_v', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfb', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='param', type='GLint*')
)

Function(name='glGetTransformFeedbackiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfb', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint*')
)

Function(name='glGetTranslatedShaderSourceANGLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='bufsize', type='GLsizei'),
arg3=ArgDef(name='length', type='GLsizei*'),
arg4=ArgDef(name='source', type='GLchar*')
)

Function(name='glGetUniformBlockIndex', enabled=True, type=Get, runWrap=True, ccodeWrap=True,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformBlockName', type='const GLchar*', wrapParams='uniformBlockName, \'\\0\', 1')
)

Function(name='glGetUniformBufferSizeEXT', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint')
)

Function(name='glGetUniformIndices', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformCount', type='GLsizei'),
arg3=ArgDef(name='uniformNames', type='const GLchar*const*', wrapType='CStringArray', wrapParams='uniformNames, uniformCount'),
arg4=ArgDef(name='uniformIndices', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetUniformLocation', enabled=True, type=GetEssential, runWrap=True, ccodeWrap=True,
retV=RetDef(type='GLint', wrapType='CRecUniformLocation', wrapParams='return_value, program, name'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetUniformLocationARB', enabled=True, type=GetEssential, runWrap=True, ccodeWrap=True,
retV=RetDef(type='GLint', wrapType='CRecUniformLocation', wrapParams='return_value, programObj, name'),
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLcharARB*', wrapType='CGLchar::CSArray', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetUniformOffsetEXT', enabled=False, type=None,
retV=RetDef(type='GLintptr'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint')
)

Function(name='glGetUniformSubroutineuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='shadertype', type='GLenum'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetUniformdv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetUniformfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetUniformfvARB', enabled=True, type=Get, inheritFrom='glGetUniformfv',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='programObj, location'),
)

Function(name='glGetUniformi64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='params', type='GLint64*')
)

Function(name='glGetUniformi64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='params', type='GLint64EXT*')
)

Function(name='glGetUniformiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetUniformivARB', enabled=True, type=Get, inheritFrom='glGetUniformiv',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='programObj, location')
)

Function(name='glGetUniformui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='params', type='GLuint64*')
)

Function(name='glGetUniformui64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='params', type='GLuint64EXT*')
)

Function(name='glGetUniformuiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='params', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetUniformuivEXT', enabled=True, type=Get, inheritFrom='glGetUniformuiv')

Function(name='glGetUnsignedBytei_vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='data', type='GLubyte*')
)

Function(name='glGetUnsignedBytevEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='data', type='GLubyte*')
)

Function(name='glGetVariantArrayObjectfvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetVariantArrayObjectivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetVariantBooleanvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLboolean*')
)

Function(name='glGetVariantFloatvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLfloat*')
)

Function(name='glGetVariantIntegervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='GLint*')
)

Function(name='glGetVariantPointervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='value', type='GLenum'),
arg3=ArgDef(name='data', type='void**')
)

Function(name='glGetVaryingLocationNV', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetVertexArrayIndexed64iv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint64*', wrapType='CGLint64_ptr')
)

Function(name='glGetVertexArrayIndexediv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetVertexArrayIntegeri_vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint*')
)

Function(name='glGetVertexArrayIntegervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint*')
)

Function(name='glGetVertexArrayPointeri_vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='void**')
)

Function(name='glGetVertexArrayPointervEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='void**')
)

Function(name='glGetVertexArrayiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribArrayObjectfvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetVertexAttribArrayObjectivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetVertexAttribIiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribIivEXT', enabled=True, type=Get, inheritFrom='glGetVertexAttribIiv')

Function(name='glGetVertexAttribIuiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribIuivEXT', enabled=True, type=Get, inheritFrom='glGetVertexAttribIuiv')

Function(name='glGetVertexAttribLdv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribLdvEXT', enabled=True, type=Get, inheritFrom='glGetVertexAttribLdv')

Function(name='glGetVertexAttribLi64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint64EXT*')
)

Function(name='glGetVertexAttribLui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64EXT*')
)

Function(name='glGetVertexAttribLui64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64EXT*')
)

Function(name='glGetVertexAttribPointerv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='pointer', type='void**', wrapType='COutArgument')
)

Function(name='glGetVertexAttribPointervARB', enabled=True, type=Get, inheritFrom='glGetVertexAttribPointerv')

Function(name='glGetVertexAttribPointervNV', enabled=True, type=Get, inheritFrom='glGetVertexAttribPointerv')

Function(name='glGetVertexAttribdv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLdouble*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribdvARB', enabled=True, type=Get, inheritFrom='glGetVertexAttribdv')

Function(name='glGetVertexAttribdvNV', enabled=True, type=Get, inheritFrom='glGetVertexAttribdv')

Function(name='glGetVertexAttribfv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLfloat*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribfvARB', enabled=True, type=Get, inheritFrom='glGetVertexAttribfv')

Function(name='glGetVertexAttribfvNV', enabled=True, type=Get, inheritFrom='glGetVertexAttribfv')

Function(name='glGetVertexAttribiv', enabled=True, type=Get,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*', wrapType='COutArgument')
)

Function(name='glGetVertexAttribivARB', enabled=True, type=Get, inheritFrom='glGetVertexAttribiv')

Function(name='glGetVertexAttribivNV', enabled=True, type=Get, inheritFrom='glGetVertexAttribiv')

Function(name='glGetVideoCaptureStreamdvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetVideoCaptureStreamfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetVideoCaptureStreamivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetVideoCaptureivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetVideoi64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint64EXT*')
)

Function(name='glGetVideoivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLint*')
)

Function(name='glGetVideoui64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint64EXT*')
)

Function(name='glGetVideouivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetVkProcAddrNV', enabled=False, type=None,
retV=RetDef(type='GLVULKANPROCNV'),
arg1=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glGetnColorTable', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='bufSize', type='GLsizei'),
arg5=ArgDef(name='table', type='void*')
)

Function(name='glGetnColorTableARB', enabled=False, type=None, inheritFrom='glGetnColorTable')

Function(name='glGetnCompressedTexImage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='lod', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetnCompressedTexImageARB', enabled=False, type=None, inheritFrom='glGetnCompressedTexImage')

Function(name='glGetnConvolutionFilter', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='bufSize', type='GLsizei'),
arg5=ArgDef(name='image', type='void*')
)

Function(name='glGetnConvolutionFilterARB', enabled=False, type=None, inheritFrom='glGetnConvolutionFilter')

Function(name='glGetnHistogram', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='reset', type='GLboolean'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='bufSize', type='GLsizei'),
arg6=ArgDef(name='values', type='void*')
)

Function(name='glGetnHistogramARB', enabled=False, type=None, inheritFrom='glGetnHistogram')

Function(name='glGetnMapdv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='v', type='GLdouble*')
)

Function(name='glGetnMapdvARB', enabled=False, type=None, inheritFrom='glGetnMapdv')

Function(name='glGetnMapfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='v', type='GLfloat*')
)

Function(name='glGetnMapfvARB', enabled=False, type=None, inheritFrom='glGetnMapfv')

Function(name='glGetnMapiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='query', type='GLenum'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='v', type='GLint*')
)

Function(name='glGetnMapivARB', enabled=False, type=None, inheritFrom='glGetnMapiv')

Function(name='glGetnMinmax', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='reset', type='GLboolean'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='bufSize', type='GLsizei'),
arg6=ArgDef(name='values', type='void*')
)

Function(name='glGetnMinmaxARB', enabled=False, type=None, inheritFrom='glGetnMinmax')

Function(name='glGetnPixelMapfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='values', type='GLfloat*')
)

Function(name='glGetnPixelMapfvARB', enabled=False, type=None, inheritFrom='glGetnPixelMapfv')

Function(name='glGetnPixelMapuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='values', type='GLuint*')
)

Function(name='glGetnPixelMapuivARB', enabled=False, type=None, inheritFrom='glGetnPixelMapuiv')

Function(name='glGetnPixelMapusv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='bufSize', type='GLsizei'),
arg3=ArgDef(name='values', type='GLushort*')
)

Function(name='glGetnPixelMapusvARB', enabled=False, type=None, inheritFrom='glGetnPixelMapusv')

Function(name='glGetnPolygonStipple', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='bufSize', type='GLsizei'),
arg2=ArgDef(name='pattern', type='GLubyte*')
)

Function(name='glGetnPolygonStippleARB', enabled=False, type=None, inheritFrom='glGetnPolygonStipple')

Function(name='glGetnSeparableFilter', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='rowBufSize', type='GLsizei'),
arg5=ArgDef(name='row', type='void*'),
arg6=ArgDef(name='columnBufSize', type='GLsizei'),
arg7=ArgDef(name='column', type='void*'),
arg8=ArgDef(name='span', type='void*')
)

Function(name='glGetnSeparableFilterARB', enabled=False, type=None, inheritFrom='glGetnSeparableFilter')

Function(name='glGetnTexImage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='bufSize', type='GLsizei'),
arg6=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glGetnTexImageARB', enabled=False, type=None, inheritFrom='glGetnTexImage')

Function(name='glGetnUniformdv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLdouble*')
)

Function(name='glGetnUniformdvARB', enabled=False, type=None, inheritFrom='glGetnUniformdv')

Function(name='glGetnUniformfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLfloat*')
)

Function(name='glGetnUniformfvARB', enabled=False, type=None, inheritFrom='glGetnUniformfv')

Function(name='glGetnUniformfvEXT', enabled=False, type=None, inheritFrom='glGetnUniformfv')

Function(name='glGetnUniformfvKHR', enabled=False, type=None, inheritFrom='glGetnUniformfv')

Function(name='glGetnUniformi64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLint64*')
)

Function(name='glGetnUniformiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glGetnUniformivARB', enabled=False, type=None, inheritFrom='glGetnUniformiv')

Function(name='glGetnUniformivEXT', enabled=False, type=None, inheritFrom='glGetnUniformiv')

Function(name='glGetnUniformivKHR', enabled=False, type=None, inheritFrom='glGetnUniformiv')

Function(name='glGetnUniformui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLuint64*')
)

Function(name='glGetnUniformuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='params', type='GLuint*')
)

Function(name='glGetnUniformuivARB', enabled=False, type=None, inheritFrom='glGetnUniformuiv')

Function(name='glGetnUniformuivKHR', enabled=False, type=None, inheritFrom='glGetnUniformuiv')

Function(name='glGlobalAlphaFactorbSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLbyte')
)

Function(name='glGlobalAlphaFactordSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLdouble')
)

Function(name='glGlobalAlphaFactorfSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat')
)

Function(name='glGlobalAlphaFactoriSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLint')
)

Function(name='glGlobalAlphaFactorsSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLshort')
)

Function(name='glGlobalAlphaFactorubSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLubyte')
)

Function(name='glGlobalAlphaFactoruiSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLuint')
)

Function(name='glGlobalAlphaFactorusSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLushort')
)

Function(name='glHint', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glHintPGI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='mode', type='GLint')
)

Function(name='glHistogram', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='width', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='sink', type='GLboolean')
)

Function(name='glHistogramEXT', enabled=True, type=Param, inheritFrom='glHistogram')

Function(name='glIglooInterfaceSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const void*')
)

Function(name='glImageTransformParameterfHP', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glImageTransformParameterfvHP', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glImageTransformParameteriHP', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glImageTransformParameterivHP', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glImportMemoryFdEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='memory', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint64'),
arg3=ArgDef(name='handleType', type='GLenum'),
arg4=ArgDef(name='fd', type='GLint')
)

Function(name='glImportMemoryWin32HandleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='memory', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint64'),
arg3=ArgDef(name='handleType', type='GLenum'),
arg4=ArgDef(name='handle', type='void*')
)

Function(name='glImportMemoryWin32NameEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='memory', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint64'),
arg3=ArgDef(name='handleType', type='GLenum'),
arg4=ArgDef(name='name', type='const void*')
)

Function(name='glImportSemaphoreFdEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='handleType', type='GLenum'),
arg3=ArgDef(name='fd', type='GLint')
)

Function(name='glImportSemaphoreWin32HandleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='handleType', type='GLenum'),
arg3=ArgDef(name='handle', type='void*')
)

Function(name='glImportSemaphoreWin32NameEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='handleType', type='GLenum'),
arg3=ArgDef(name='name', type='const void*')
)

Function(name='glImportSyncEXT', enabled=False, type=None,
retV=RetDef(type='GLsync'),
arg1=ArgDef(name='external_sync_type', type='GLenum'),
arg2=ArgDef(name='external_sync', type='GLintptr'),
arg3=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glIndexFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei')
)

Function(name='glIndexFuncEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLclampf')
)

Function(name='glIndexMask', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLuint')
)

Function(name='glIndexMaterialEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glIndexPointer', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glIndexPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glIndexPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLint'),
arg3=ArgDef(name='pointer', type='const void**'),
arg4=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glIndexd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLdouble')
)

Function(name='glIndexdv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLdouble*')
)

Function(name='glIndexf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLfloat')
)

Function(name='glIndexfv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLfloat*')
)

Function(name='glIndexi', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLint')
)

Function(name='glIndexiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLint*')
)

Function(name='glIndexs', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLshort')
)

Function(name='glIndexsv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLshort*')
)

Function(name='glIndexub', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='GLubyte')
)

Function(name='glIndexubv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='c', type='const GLubyte*')
)

Function(name='glIndexxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='component', type='GLfixed')
)

Function(name='glIndexxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='component', type='const GLfixed*')
)

Function(name='glInitNames', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glInsertComponentEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='res', type='GLuint'),
arg2=ArgDef(name='src', type='GLuint'),
arg3=ArgDef(name='num', type='GLuint')
)

Function(name='glInsertEventMarkerEXT', enabled=False, type=None, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='length', type='GLsizei'),
arg2=ArgDef(name='marker', type='const GLchar*')
)

Function(name='glInstrumentsBufferSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLsizei'),
arg2=ArgDef(name='buffer', type='GLint*')
)

Function(name='glInterleavedArrays', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='format', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glInterpolatePathsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='resultPath', type='GLuint'),
arg2=ArgDef(name='pathA', type='GLuint'),
arg3=ArgDef(name='pathB', type='GLuint'),
arg4=ArgDef(name='weight', type='GLfloat')
)

Function(name='glInvalidateBufferData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glInvalidateBufferSubData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='length', type='GLsizeiptr')
)

Function(name='glInvalidateFramebuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='numAttachments', type='GLsizei'),
arg3=ArgDef(name='attachments', type='const GLenum*', wrapParams='numAttachments, attachments')
)

Function(name='glInvalidateNamedFramebufferData', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='numAttachments', type='GLsizei'),
arg3=ArgDef(name='attachments', type='const GLenum*', wrapParams='numAttachments, attachments')
)

Function(name='glInvalidateNamedFramebufferSubData', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='numAttachments', type='GLsizei'),
arg3=ArgDef(name='attachments', type='const GLenum*'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei')
)

Function(name='glInvalidateSubFramebuffer', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='numAttachments', type='GLsizei'),
arg3=ArgDef(name='attachments', type='const GLenum*'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei')
)

Function(name='glInvalidateTexImage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint')
)

Function(name='glInvalidateTexSubImage', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei')
)

Function(name='glIsAsyncMarkerSGIX', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='marker', type='GLuint')
)

Function(name='glIsBuffer', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glIsBufferARB', enabled=True, type=Get, inheritFrom='glIsBuffer')

Function(name='glIsBufferResidentNV', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glIsCommandListNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='list', type='GLuint')
)

Function(name='glIsEnabled', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='cap', type='GLenum')
)

Function(name='glIsEnabledIndexedEXT', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glIsEnabledi', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint')
)

Function(name='glIsEnablediEXT', enabled=True, type=Get, inheritFrom='glIsEnabledi')

Function(name='glIsEnablediNV', enabled=False, type=None, inheritFrom='glIsEnabledi')

Function(name='glIsEnablediOES', enabled=False, type=None, inheritFrom='glIsEnabledi')

Function(name='glIsFenceAPPLE', enabled=False, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glIsFenceNV', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='fence', type='GLuint', wrapParam='CGLfence')
)


Function(name='glIsFramebuffer', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer')
)

Function(name='glIsFramebufferEXT', enabled=True, type=Get, recWrap=True,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebufferEXT')
)

Function(name='glIsFramebufferOES', enabled=True, type=Get, inheritFrom='glIsFramebuffer')

Function(name='glIsImageHandleResidentARB', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glIsImageHandleResidentNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glIsList', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='list', type='GLuint')
)

Function(name='glIsMemoryObjectEXT', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='memoryObject', type='GLuint')
)

Function(name='glIsNameAMD', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='identifier', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint')
)

Function(name='glIsNamedBufferResidentNV', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glIsNamedStringARB', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='namelen', type='GLint'),
arg2=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1')
)

Function(name='glIsObjectBufferATI', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glIsOcclusionQueryNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='id', type='GLuint')
)

Function(name='glIsPathNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='path', type='GLuint')
)

Function(name='glIsPointInFillPathNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='mask', type='GLuint'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat')
)

Function(name='glIsPointInStrokePathNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat')
)

Function(name='glIsProgram', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glIsProgramARB', enabled=True, type=Get, inheritFrom='glIsProgram')

Function(name='glIsProgramNV', enabled=True, type=Get, inheritFrom='glIsProgram')

Function(name='glIsProgramPipeline', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline')
)

Function(name='glIsProgramPipelineEXT', enabled=True, type=Get, inheritFrom='glIsProgramPipeline')

Function(name='glIsQuery', enabled=True, type=Get|Query, runCond='ConditionQueries()',
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery')
)

Function(name='glIsQueryARB', enabled=True, type=Get, inheritFrom='glIsQuery')

Function(name='glIsQueryEXT', enabled=True, type=Get, inheritFrom='glIsQuery')

Function(name='glIsRenderbuffer', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glIsRenderbufferEXT', enabled=True, type=Get, recWrap=True, inheritFrom='glIsRenderbuffer',
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT')
)

Function(name='glIsRenderbufferOES', enabled=True, type=Get, inheritFrom='glIsRenderbuffer')

Function(name='glIsSampler', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler')
)

Function(name='glIsSemaphoreEXT', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='semaphore', type='GLuint')
)

Function(name='glIsShader', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram')
)

Function(name='glIsStateNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='state', type='GLuint')
)

Function(name='glIsSync', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='sync', type='GLsync', wrapType='CGLsync')
)

Function(name='glIsSyncAPPLE', enabled=False, type=Get, inheritFrom='glIsSync')

Function(name='glIsTexture', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glIsTextureEXT', enabled=True, type=Get, inheritFrom='glIsTexture', runWrap=False, recWrap=False)

Function(name='glIsTextureHandleResidentARB', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glIsTextureHandleResidentNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glIsTransformFeedback', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLTransformFeedback')
)

Function(name='glIsTransformFeedbackNV', enabled=True, type=Get, inheritFrom='glIsTransformFeedback')

Function(name='glIsVariantEnabledEXT', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='cap', type='GLenum')
)

Function(name='glIsVertexArray', enabled=True, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='array', type='GLuint', wrapType='CGLVertexArray')
)

Function(name='glIsVertexArrayAPPLE', enabled=False, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='array', type='GLuint')
)

Function(name='glIsVertexArrayOES', enabled=True, type=Get, inheritFrom='glIsVertexArray')

Function(name='glIsVertexAttribEnabledAPPLE', enabled=False, type=Get,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum')
)

Function(name='glLGPUCopyImageSubDataNVX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='sourceGpu', type='GLuint'),
arg2=ArgDef(name='destinationGpuMask', type='GLbitfield'),
arg3=ArgDef(name='srcName', type='GLuint'),
arg4=ArgDef(name='srcTarget', type='GLenum'),
arg5=ArgDef(name='srcLevel', type='GLint'),
arg6=ArgDef(name='srcX', type='GLint'),
arg7=ArgDef(name='srxY', type='GLint'),
arg8=ArgDef(name='srcZ', type='GLint'),
arg9=ArgDef(name='dstName', type='GLuint'),
arg10=ArgDef(name='dstTarget', type='GLenum'),
arg11=ArgDef(name='dstLevel', type='GLint'),
arg12=ArgDef(name='dstX', type='GLint'),
arg13=ArgDef(name='dstY', type='GLint'),
arg14=ArgDef(name='dstZ', type='GLint'),
arg15=ArgDef(name='width', type='GLsizei'),
arg16=ArgDef(name='height', type='GLsizei'),
arg17=ArgDef(name='depth', type='GLsizei')
)

Function(name='glLGPUInterlockNVX', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glLGPUNamedBufferSubDataNVX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpuMask', type='GLbitfield'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='offset', type='GLintptr'),
arg4=ArgDef(name='size', type='GLsizeiptr'),
arg5=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glLabelObjectEXT', enabled=True, type=Param, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='object', type='GLuint'),
arg3=ArgDef(name='length', type='GLsizei'),
arg4=ArgDef(name='label', type='const GLchar*', wrapParams='length, label')
)

Function(name='glLabelObjectWithResponsibleProcessAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='pid', type='GLint')
)

Function(name='glLightEnviSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glLightModelf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glLightModelfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glLightModeli', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glLightModeliv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname,params')
)

Function(name='glLightModelx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfixed')
)

Function(name='glLightModelxOES', enabled=True, type=Param, inheritFrom='glLightModelx')

Function(name='glLightModelxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname,params')
)

Function(name='glLightModelxvOES', enabled=True, type=Param, inheritFrom='glLightModelxv')

Function(name='glLightf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glLightfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glLighti', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glLightiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glLightx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glLightxOES', enabled=True, type=Param, inheritFrom='glLightx')

Function(name='glLightxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='light', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glLightxvOES', enabled=True, type=Param, inheritFrom='glLightxv')

Function(name='glLineStipple', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLint'),
arg2=ArgDef(name='pattern', type='GLushort')
)

Function(name='glLineWidth', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='width', type='GLfloat')
)

Function(name='glLineWidthx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='width', type='GLfixed')
)

Function(name='glLineWidthxOES', enabled=True, type=Param, inheritFrom='glLineWidthx')

Function(name='glLinkProgram', enabled=True, type=Param, recWrap=True, runWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glLinkProgramARB', enabled=True, type=Param, recWrap=True, runWrap=True, stateTrack=True, inheritFrom='glLinkProgram')

Function(name='glListBase', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='base', type='GLuint')
)

Function(name='glListDrawCommandsStatesClientNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='segment', type='GLuint'),
arg3=ArgDef(name='indirects', type='const void**'),
arg4=ArgDef(name='sizes', type='const GLsizei*'),
arg5=ArgDef(name='states', type='const GLuint*'),
arg6=ArgDef(name='fbos', type='const GLuint*'),
arg7=ArgDef(name='count', type='GLuint')
)

Function(name='glListParameterfSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glListParameterfvSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glListParameteriSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glListParameterivSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glLoadIdentity', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glLoadIdentityDeformationMapSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glLoadMatrixd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLdouble*', wrapParams='16, m')
)

Function(name='glLoadMatrixf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfloat*', wrapParams='16, m')
)

Function(name='glLoadMatrixx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfixed*', wrapParams='16, m')
)

Function(name='glLoadMatrixxOES', enabled=True, type=Param, inheritFrom='glLoadMatrixx')

Function(name='glLoadName', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='name', type='GLuint')
)

Function(name='glLoadPaletteFromModelViewMatrixOES', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glLoadProgramNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='len', type='GLsizei'),
arg4=ArgDef(name='program', type='const GLubyte*')
)

Function(name='glLoadTransposeMatrixd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLdouble*', wrapParams='16, m')
)

Function(name='glLoadTransposeMatrixdARB', enabled=True, type=Param, inheritFrom='glLoadTransposeMatrixd')

Function(name='glLoadTransposeMatrixf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfloat*', wrapParams='16, m')
)

Function(name='glLoadTransposeMatrixfARB', enabled=True, type=Param, inheritFrom='glLoadTransposeMatrixf')

Function(name='glLoadTransposeMatrixxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfixed*')
)

Function(name='glLockArraysEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLint'),
arg2=ArgDef(name='count', type='GLsizei')
)

Function(name='glLogicOp', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='opcode', type='GLenum')
)

Function(name='glMakeBufferNonResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glMakeBufferResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMakeImageHandleNonResidentARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64', wrapType='CGLTextureHandle')
)

Function(name='glMakeImageHandleNonResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glMakeImageHandleResidentARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64', wrapType='CGLTextureHandle'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMakeImageHandleResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMakeNamedBufferNonResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glMakeNamedBufferResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMakeTextureHandleNonResidentARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64', wrapType='CGLTextureHandle')
)

Function(name='glMakeTextureHandleNonResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glMakeTextureHandleResidentARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64', wrapType='CGLTextureHandle')
)

Function(name='glMakeTextureHandleResidentNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='handle', type='GLuint64')
)

Function(name='glMap1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLdouble'),
arg3=ArgDef(name='u2', type='GLdouble'),
arg4=ArgDef(name='stride', type='GLint'),
arg5=ArgDef(name='order', type='GLint'),
arg6=ArgDef(name='points', type='const GLdouble*', wrapType='CGLdouble::CGLMapArray', wrapParams='target, order, points')
)

Function(name='glMap1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLfloat'),
arg3=ArgDef(name='u2', type='GLfloat'),
arg4=ArgDef(name='stride', type='GLint'),
arg5=ArgDef(name='order', type='GLint'),
arg6=ArgDef(name='points', type='const GLfloat*', wrapType='CGLfloat::CGLMapArray', wrapParams='target, order, points')
)

Function(name='glMap1xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLfixed'),
arg3=ArgDef(name='u2', type='GLfixed'),
arg4=ArgDef(name='stride', type='GLint'),
arg5=ArgDef(name='order', type='GLint'),
arg6=ArgDef(name='points', type='GLfixed')
)

Function(name='glMap2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLdouble'),
arg3=ArgDef(name='u2', type='GLdouble'),
arg4=ArgDef(name='ustride', type='GLint'),
arg5=ArgDef(name='uorder', type='GLint'),
arg6=ArgDef(name='v1', type='GLdouble'),
arg7=ArgDef(name='v2', type='GLdouble'),
arg8=ArgDef(name='vstride', type='GLint'),
arg9=ArgDef(name='vorder', type='GLint'),
arg10=ArgDef(name='points', type='const GLdouble*', wrapType='CGLdouble::CGLMapArray', wrapParams='target, uorder, vorder, points')
)

Function(name='glMap2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLfloat'),
arg3=ArgDef(name='u2', type='GLfloat'),
arg4=ArgDef(name='ustride', type='GLint'),
arg5=ArgDef(name='uorder', type='GLint'),
arg6=ArgDef(name='v1', type='GLfloat'),
arg7=ArgDef(name='v2', type='GLfloat'),
arg8=ArgDef(name='vstride', type='GLint'),
arg9=ArgDef(name='vorder', type='GLint'),
arg10=ArgDef(name='points', type='const GLfloat*', wrapType='CGLfloat::CGLMapArray', wrapParams='target, uorder, vorder, points')
)

Function(name='glMap2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='u1', type='GLfixed'),
arg3=ArgDef(name='u2', type='GLfixed'),
arg4=ArgDef(name='ustride', type='GLint'),
arg5=ArgDef(name='uorder', type='GLint'),
arg6=ArgDef(name='v1', type='GLfixed'),
arg7=ArgDef(name='v2', type='GLfixed'),
arg8=ArgDef(name='vstride', type='GLint'),
arg9=ArgDef(name='vorder', type='GLint'),
arg10=ArgDef(name='points', type='GLfixed')
)

Function(name='glMapBuffer', enabled=True, type=Map, stateTrack=True, recCond='ConditionBufferES(_recorder)',
retV=RetDef(type='void*', wrapType='CGLvoid_ptr'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMapBufferARB', enabled=True, type=Map, inheritFrom='glMapBuffer', stateTrack=True)

Function(name='glMapBufferOES', enabled=True, type=Map, inheritFrom='glMapBuffer', stateTrack=True)

Function(name='glMapBufferRange', enabled=True, type=Map, stateTrack=True, recCond='ConditionBufferES(_recorder)', interceptorExecOverride=True,
retV=RetDef(type='void*', wrapType='CGLvoid_ptr'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='length', type='GLsizeiptr'),
arg4=ArgDef(name='access', type='GLbitfield', wrapType='CMapAccess')
)

Function(name='glMapBufferRangeEXT', enabled=True, type=Map, interceptorExecOverride=True, inheritFrom='glMapBufferRange')

Function(name='glMapControlPointsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='ustride', type='GLsizei'),
arg5=ArgDef(name='vstride', type='GLsizei'),
arg6=ArgDef(name='uorder', type='GLint'),
arg7=ArgDef(name='vorder', type='GLint'),
arg8=ArgDef(name='packed', type='GLboolean'),
arg9=ArgDef(name='points', type='const void*')
)

Function(name='glMapGrid1d', enabled=True, type=Map,
retV=RetDef(type='void'),
arg1=ArgDef(name='un', type='GLint'),
arg2=ArgDef(name='u1', type='GLdouble'),
arg3=ArgDef(name='u2', type='GLdouble')
)

Function(name='glMapGrid1f', enabled=True, type=Map,
retV=RetDef(type='void'),
arg1=ArgDef(name='un', type='GLint'),
arg2=ArgDef(name='u1', type='GLfloat'),
arg3=ArgDef(name='u2', type='GLfloat')
)

Function(name='glMapGrid1xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLint'),
arg2=ArgDef(name='u1', type='GLfixed'),
arg3=ArgDef(name='u2', type='GLfixed')
)

Function(name='glMapGrid2d', enabled=True, type=Map,
retV=RetDef(type='void'),
arg1=ArgDef(name='un', type='GLint'),
arg2=ArgDef(name='u1', type='GLdouble'),
arg3=ArgDef(name='u2', type='GLdouble'),
arg4=ArgDef(name='vn', type='GLint'),
arg5=ArgDef(name='v1', type='GLdouble'),
arg6=ArgDef(name='v2', type='GLdouble')
)

Function(name='glMapGrid2f', enabled=True, type=Map,
retV=RetDef(type='void'),
arg1=ArgDef(name='un', type='GLint'),
arg2=ArgDef(name='u1', type='GLfloat'),
arg3=ArgDef(name='u2', type='GLfloat'),
arg4=ArgDef(name='vn', type='GLint'),
arg5=ArgDef(name='v1', type='GLfloat'),
arg6=ArgDef(name='v2', type='GLfloat')
)

Function(name='glMapGrid2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLint'),
arg2=ArgDef(name='u1', type='GLfixed'),
arg3=ArgDef(name='u2', type='GLfixed'),
arg4=ArgDef(name='v1', type='GLfixed'),
arg5=ArgDef(name='v2', type='GLfixed')
)

Function(name='glMapNamedBuffer', enabled=True, type=Map, stateTrack=True,
retV=RetDef(type='void*'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glMapNamedBufferEXT', enabled=True, type=Map, inheritFrom='glMapNamedBuffer')

Function(name='glMapNamedBufferRange', enabled=True, type=Map, stateTrack=True, interceptorExecOverride=True,
retV=RetDef(type='void*'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='length', type='GLsizeiptr'),
arg4=ArgDef(name='access', type='GLbitfield', wrapType='CMapAccess')
)

Function(name='glMapNamedBufferRangeEXT', enabled=True, type=Map, interceptorExecOverride=True, inheritFrom='glMapNamedBufferRange')

Function(name='glMapObjectBufferATI', enabled=False, type=None,
retV=RetDef(type='void*'),
arg1=ArgDef(name='buffer', type='GLuint')
)

Function(name='glMapParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glMapParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glMapTexture2DINTEL', enabled=True, type=Param, ccodeWrap=True, stateTrack=True, interceptorExecOverride=True,
retV=RetDef(type='void*', wrapType='CGLMappedTexturePtr'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='access', type='GLbitfield', wrapParams='GL_MAP_WRITE_BIT | GL_MAP_READ_BIT'),
arg4=ArgDef(name='stride', type='GLint*', wrapType='COutArgument'),
arg5=ArgDef(name='layout', type='GLenum*', wrapType='COutArgument')
)

Function(name='glMapVertexAttrib1dAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint'),
arg3=ArgDef(name='u1', type='GLdouble'),
arg4=ArgDef(name='u2', type='GLdouble'),
arg5=ArgDef(name='stride', type='GLint'),
arg6=ArgDef(name='order', type='GLint'),
arg7=ArgDef(name='points', type='const GLdouble*')
)

Function(name='glMapVertexAttrib1fAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint'),
arg3=ArgDef(name='u1', type='GLfloat'),
arg4=ArgDef(name='u2', type='GLfloat'),
arg5=ArgDef(name='stride', type='GLint'),
arg6=ArgDef(name='order', type='GLint'),
arg7=ArgDef(name='points', type='const GLfloat*')
)

Function(name='glMapVertexAttrib2dAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint'),
arg3=ArgDef(name='u1', type='GLdouble'),
arg4=ArgDef(name='u2', type='GLdouble'),
arg5=ArgDef(name='ustride', type='GLint'),
arg6=ArgDef(name='uorder', type='GLint'),
arg7=ArgDef(name='v1', type='GLdouble'),
arg8=ArgDef(name='v2', type='GLdouble'),
arg9=ArgDef(name='vstride', type='GLint'),
arg10=ArgDef(name='vorder', type='GLint'),
arg11=ArgDef(name='points', type='const GLdouble*')
)

Function(name='glMapVertexAttrib2fAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLuint'),
arg3=ArgDef(name='u1', type='GLfloat'),
arg4=ArgDef(name='u2', type='GLfloat'),
arg5=ArgDef(name='ustride', type='GLint'),
arg6=ArgDef(name='uorder', type='GLint'),
arg7=ArgDef(name='v1', type='GLfloat'),
arg8=ArgDef(name='v2', type='GLfloat'),
arg9=ArgDef(name='vstride', type='GLint'),
arg10=ArgDef(name='vorder', type='GLint'),
arg11=ArgDef(name='points', type='const GLfloat*')
)

Function(name='glMaterialf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glMaterialfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glMateriali', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glMaterialiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glMaterialx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glMaterialxOES', enabled=True, type=Param, inheritFrom='glMaterialx')

Function(name='glMaterialxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glMaterialxvOES', enabled=True, type=Param, inheritFrom='glMaterialxv')

Function(name='glMatrixFrustumEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='left', type='GLdouble'),
arg3=ArgDef(name='right', type='GLdouble'),
arg4=ArgDef(name='bottom', type='GLdouble'),
arg5=ArgDef(name='top', type='GLdouble'),
arg6=ArgDef(name='zNear', type='GLdouble'),
arg7=ArgDef(name='zFar', type='GLdouble')
)

Function(name='glMatrixIndexPointerARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glMatrixIndexPointerOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glMatrixIndexPointerOESBounds', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const GLvoid*'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glMatrixIndexubvARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='indices', type='const GLubyte*')
)

Function(name='glMatrixIndexuivARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='indices', type='const GLuint*')
)

Function(name='glMatrixIndexusvARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='indices', type='const GLushort*')
)

Function(name='glMatrixLoad3x2fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixLoad3x3fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixLoadIdentityEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glMatrixLoadTranspose3x3fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixLoadTransposedEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLdouble*')
)

Function(name='glMatrixLoadTransposefEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixLoaddEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLdouble*', wrapParams='16, m')
)

Function(name='glMatrixLoadfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*', wrapParams='16, m')
)

Function(name='glMatrixMode', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glMatrixMult3x2fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixMult3x3fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixMultTranspose3x3fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='matrixMode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixMultTransposedEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLdouble*')
)

Function(name='glMatrixMultTransposefEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixMultdEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLdouble*')
)

Function(name='glMatrixMultfEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='m', type='const GLfloat*')
)

Function(name='glMatrixOrthoEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='left', type='GLdouble'),
arg3=ArgDef(name='right', type='GLdouble'),
arg4=ArgDef(name='bottom', type='GLdouble'),
arg5=ArgDef(name='top', type='GLdouble'),
arg6=ArgDef(name='zNear', type='GLdouble'),
arg7=ArgDef(name='zFar', type='GLdouble')
)

Function(name='glMatrixPopEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glMatrixPushEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glMatrixRotatedEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='angle', type='GLdouble'),
arg3=ArgDef(name='x', type='GLdouble'),
arg4=ArgDef(name='y', type='GLdouble'),
arg5=ArgDef(name='z', type='GLdouble')
)

Function(name='glMatrixRotatefEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='angle', type='GLfloat'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat'),
arg5=ArgDef(name='z', type='GLfloat')
)

Function(name='glMatrixScaledEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glMatrixScalefEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glMatrixTranslatedEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glMatrixTranslatefEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glMaxShaderCompilerThreadsARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLuint')
)

Function(name='glMaxShaderCompilerThreadsKHR', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLuint')
)

Function(name='glMemoryBarrier', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='barriers', type='GLbitfield')
)

Function(name='glMemoryBarrierByRegion',enabled=False,type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='barriers', type='GLbitfield')
)

Function(name='glMemoryBarrierEXT', enabled=True, type=Param, inheritFrom='glMemoryBarrier')

Function(name='glMemoryObjectParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='memoryObject', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glMinSampleShading', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLfloat')
)

Function(name='glMinSampleShadingARB', enabled=True, type=Param, inheritFrom='glMinSampleShading')

Function(name='glMinSampleShadingOES', enabled=False, type=None, inheritFrom='glMinSampleShading')

Function(name='glMinmax', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='sink', type='GLboolean')
)

Function(name='glMinmaxEXT', enabled=True, type=Param, inheritFrom='glMinmax')

Function(name='glMultMatrixd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLdouble*', wrapParams='16, m')
)

Function(name='glMultMatrixf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfloat*', wrapParams='16, m')
)

Function(name='glMultMatrixx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfixed*', wrapParams='16, m')
)

Function(name='glMultMatrixxOES', enabled=True, type=Param, inheritFrom='glMultMatrixx')

Function(name='glMultTransposeMatrixd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLdouble*', wrapParams='16, m')
)

Function(name='glMultTransposeMatrixdARB', enabled=True, type=Param, inheritFrom='glMultTransposeMatrixd')

Function(name='glMultTransposeMatrixf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfloat*', wrapParams='16, m')
)

Function(name='glMultTransposeMatrixfARB', enabled=True, type=Param, inheritFrom='glMultTransposeMatrixf')

Function(name='glMultTransposeMatrixxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='m', type='const GLfixed*')
)

Function(name='glMultiDrawArrays', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(first, count, drawcount)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='const GLint*', wrapParams='drawcount, first'),
arg3=ArgDef(name='count', type='const GLsizei*', wrapParams='drawcount, count'),
arg4=ArgDef(name='drawcount', type='GLsizei')
)

Function(name='glMultiDrawArraysEXT', enabled=True, type=Render, inheritFrom='glMultiDrawArrays')

Function(name='glMultiDrawArraysIndirect', enabled=True, type=Render, preToken='CgitsClientIndirectArraysUpdate(mode, indirect, drawcount, stride)',
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg3=ArgDef(name='drawcount', type='GLsizei'),
arg4=ArgDef(name='stride', type='GLsizei')
)

Function(name='glMultiDrawArraysIndirectAMD', enabled=True, type=Render, inheritFrom='glMultiDrawArraysIndirect')

Function(name='glMultiDrawArraysIndirectBindlessCountNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg3=ArgDef(name='drawCount', type='GLsizei'),
arg4=ArgDef(name='maxDrawCount', type='GLsizei'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='vertexBufferCount', type='GLint')
)

Function(name='glMultiDrawArraysIndirectBindlessNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg3=ArgDef(name='drawCount', type='GLsizei'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='vertexBufferCount', type='GLint')
)

Function(name='glMultiDrawArraysIndirectCount', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg3=ArgDef(name='drawcount', type='GLintptr'),
arg4=ArgDef(name='maxdrawcount', type='GLsizei'),
arg5=ArgDef(name='stride', type='GLsizei')
)

Function(name='glMultiDrawArraysIndirectCountARB', enabled=False, type=None, inheritFrom='glMultiDrawArraysIndirectCount')

Function(name='glMultiDrawArraysIndirectEXT', enabled=False, type=None, inheritFrom='glMultiDrawArraysIndirect')

Function(name='glMultiDrawElementArrayAPPLE', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='first', type='const GLint*'),
arg3=ArgDef(name='count', type='const GLsizei*'),
arg4=ArgDef(name='primcount', type='GLsizei')
)

Function(name='glMultiDrawElements', enabled=True, type=Render, preToken='CgitsClientArraysUpdate(type, count, indices, drawcount)', execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='const GLsizei*', wrapParams='drawcount, count'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*const*', wrapType='CDataPtrArray', wrapParams='GL_ELEMENT_ARRAY_BUFFER, indices, drawcount'),
arg5=ArgDef(name='drawcount', type='GLsizei')
)

Function(name='glMultiDrawElementsBaseVertex', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='const GLsizei*'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*const*'),
arg5=ArgDef(name='drawcount', type='GLsizei'),
arg6=ArgDef(name='basevertex', type='const GLint*')
)

Function(name='glMultiDrawElementsBaseVertexEXT', enabled=False, type=None, inheritFrom='glMultiDrawElementsBaseVertex')

Function(name='glMultiDrawElementsBaseVertexOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode',type='GLenum'),
arg2=ArgDef(name='count',type='const GLsizei*'),
arg3=ArgDef(name='type',type='GLenum'),
arg4=ArgDef(name='indices',type='const void**'),
arg5=ArgDef(name='primcount',type='GLsizei'),
arg6=ArgDef(name='basevertex',type='const GLint*')
)

Function(name='glMultiDrawElementsEXT', enabled=True, type=Render, inheritFrom='glMultiDrawElements')

Function(name='glMultiDrawElementsIndirect', enabled=True, type=Render, preToken='CgitsClientIndirectArraysUpdate(mode, type, indirect, drawcount, stride)',
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg4=ArgDef(name='drawcount', type='GLsizei'),
arg5=ArgDef(name='stride', type='GLsizei')
)

Function(name='glMultiDrawElementsIndirectAMD', enabled=True, type=Render, inheritFrom='glMultiDrawElementsIndirect')

Function(name='glMultiDrawElementsIndirectBindlessCountNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg4=ArgDef(name='drawCount', type='GLsizei'),
arg5=ArgDef(name='maxDrawCount', type='GLsizei'),
arg6=ArgDef(name='stride', type='GLsizei'),
arg7=ArgDef(name='vertexBufferCount', type='GLint')
)

Function(name='glMultiDrawElementsIndirectBindlessNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg4=ArgDef(name='drawCount', type='GLsizei'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='vertexBufferCount', type='GLint')
)

Function(name='glMultiDrawElementsIndirectCount', enabled=True, type=Render,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='indirect', type='const void*', wrapType='CIndirectPtr'),
arg4=ArgDef(name='drawcount', type='GLintptr'),
arg5=ArgDef(name='maxdrawcount', type='GLsizei'),
arg6=ArgDef(name='stride', type='GLsizei')
)

Function(name='glMultiDrawElementsIndirectCountARB', enabled=True, type=Render, inheritFrom='glMultiDrawElementsIndirectCount')

Function(name='glMultiDrawElementsIndirectEXT', enabled=False, type=None, inheritFrom='glMultiDrawElementsIndirect')

Function(name='glMultiDrawRangeElementArrayAPPLE', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='end', type='GLuint'),
arg4=ArgDef(name='first', type='const GLint*'),
arg5=ArgDef(name='count', type='const GLsizei*'),
arg6=ArgDef(name='primcount', type='GLsizei')
)

Function(name='glMultiModeDrawArraysIBM', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='const GLenum*'),
arg2=ArgDef(name='first', type='const GLint*'),
arg3=ArgDef(name='count', type='const GLsizei*'),
arg4=ArgDef(name='primcount', type='GLsizei'),
arg5=ArgDef(name='modestride', type='GLint')
)

Function(name='glMultiModeDrawElementsIBM', enabled=False, type=None, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='const GLenum*'),
arg2=ArgDef(name='count', type='const GLsizei*'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='indices', type='const void*const*'),
arg5=ArgDef(name='primcount', type='GLsizei'),
arg6=ArgDef(name='modestride', type='GLint')
)

Function(name='glMultiTexBufferEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glMultiTexCoord1bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLbyte')
)

Function(name='glMultiTexCoord1bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glMultiTexCoord1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLdouble')
)

Function(name='glMultiTexCoord1dARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1d')

Function(name='glMultiTexCoord1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='1, v')
)

Function(name='glMultiTexCoord1dvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1dv')

Function(name='glMultiTexCoord1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLfloat')
)

Function(name='glMultiTexCoord1fARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1f')

Function(name='glMultiTexCoord1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='1, v')
)

Function(name='glMultiTexCoord1fvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1fv')

Function(name='glMultiTexCoord1hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLhalfNV')
)

Function(name='glMultiTexCoord1hvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLhalfNV*')
)

Function(name='glMultiTexCoord1i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLint')
)

Function(name='glMultiTexCoord1iARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1i')

Function(name='glMultiTexCoord1iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='1, v')
)

Function(name='glMultiTexCoord1ivARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1iv')

Function(name='glMultiTexCoord1s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLshort')
)

Function(name='glMultiTexCoord1sARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1s')

Function(name='glMultiTexCoord1sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='1, v')
)

Function(name='glMultiTexCoord1svARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord1sv')

Function(name='glMultiTexCoord1xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLfixed')
)

Function(name='glMultiTexCoord1xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glMultiTexCoord2bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLbyte'),
arg3=ArgDef(name='t', type='GLbyte')
)

Function(name='glMultiTexCoord2bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glMultiTexCoord2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLdouble'),
arg3=ArgDef(name='t', type='GLdouble')
)

Function(name='glMultiTexCoord2dARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2d')

Function(name='glMultiTexCoord2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glMultiTexCoord2dvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2dv')

Function(name='glMultiTexCoord2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat')
)

Function(name='glMultiTexCoord2fARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2f')

Function(name='glMultiTexCoord2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glMultiTexCoord2fvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2fv')

Function(name='glMultiTexCoord2hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLhalfNV'),
arg3=ArgDef(name='t', type='GLhalfNV')
)

Function(name='glMultiTexCoord2hvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLhalfNV*')
)

Function(name='glMultiTexCoord2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLint'),
arg3=ArgDef(name='t', type='GLint')
)

Function(name='glMultiTexCoord2iARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2i')

Function(name='glMultiTexCoord2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glMultiTexCoord2ivARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2iv')

Function(name='glMultiTexCoord2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLshort'),
arg3=ArgDef(name='t', type='GLshort')
)

Function(name='glMultiTexCoord2sARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2s')

Function(name='glMultiTexCoord2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glMultiTexCoord2svARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord2sv')

Function(name='glMultiTexCoord2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLfixed'),
arg3=ArgDef(name='t', type='GLfixed')
)

Function(name='glMultiTexCoord2xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glMultiTexCoord3bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLbyte'),
arg3=ArgDef(name='t', type='GLbyte'),
arg4=ArgDef(name='r', type='GLbyte')
)

Function(name='glMultiTexCoord3bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glMultiTexCoord3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLdouble'),
arg3=ArgDef(name='t', type='GLdouble'),
arg4=ArgDef(name='r', type='GLdouble')
)

Function(name='glMultiTexCoord3dARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3d')

Function(name='glMultiTexCoord3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glMultiTexCoord3dvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3dv')

Function(name='glMultiTexCoord3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat'),
arg4=ArgDef(name='r', type='GLfloat')
)

Function(name='glMultiTexCoord3fARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3f')

Function(name='glMultiTexCoord3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glMultiTexCoord3fvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3fv')

Function(name='glMultiTexCoord3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLhalfNV'),
arg3=ArgDef(name='t', type='GLhalfNV'),
arg4=ArgDef(name='r', type='GLhalfNV')
)

Function(name='glMultiTexCoord3hvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLhalfNV*')
)

Function(name='glMultiTexCoord3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLint'),
arg3=ArgDef(name='t', type='GLint'),
arg4=ArgDef(name='r', type='GLint')
)

Function(name='glMultiTexCoord3iARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3i')

Function(name='glMultiTexCoord3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glMultiTexCoord3ivARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3iv')

Function(name='glMultiTexCoord3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLshort'),
arg3=ArgDef(name='t', type='GLshort'),
arg4=ArgDef(name='r', type='GLshort')
)

Function(name='glMultiTexCoord3sARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3s')

Function(name='glMultiTexCoord3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glMultiTexCoord3svARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord3sv')

Function(name='glMultiTexCoord3xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLfixed'),
arg3=ArgDef(name='t', type='GLfixed'),
arg4=ArgDef(name='r', type='GLfixed')
)

Function(name='glMultiTexCoord3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glMultiTexCoord4bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLbyte'),
arg3=ArgDef(name='t', type='GLbyte'),
arg4=ArgDef(name='r', type='GLbyte'),
arg5=ArgDef(name='q', type='GLbyte')
)

Function(name='glMultiTexCoord4bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glMultiTexCoord4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLdouble'),
arg3=ArgDef(name='t', type='GLdouble'),
arg4=ArgDef(name='r', type='GLdouble'),
arg5=ArgDef(name='q', type='GLdouble')
)

Function(name='glMultiTexCoord4dARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4d')

Function(name='glMultiTexCoord4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glMultiTexCoord4dvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4dv')

Function(name='glMultiTexCoord4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat'),
arg4=ArgDef(name='r', type='GLfloat'),
arg5=ArgDef(name='q', type='GLfloat')
)

Function(name='glMultiTexCoord4fARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4f')

Function(name='glMultiTexCoord4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glMultiTexCoord4fvARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4fv')

Function(name='glMultiTexCoord4hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLhalfNV'),
arg3=ArgDef(name='t', type='GLhalfNV'),
arg4=ArgDef(name='r', type='GLhalfNV'),
arg5=ArgDef(name='q', type='GLhalfNV')
)

Function(name='glMultiTexCoord4hvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLhalfNV*')
)

Function(name='glMultiTexCoord4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLint'),
arg3=ArgDef(name='t', type='GLint'),
arg4=ArgDef(name='r', type='GLint'),
arg5=ArgDef(name='q', type='GLint')
)

Function(name='glMultiTexCoord4iARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4i')

Function(name='glMultiTexCoord4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glMultiTexCoord4ivARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4iv')

Function(name='glMultiTexCoord4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='s', type='GLshort'),
arg3=ArgDef(name='t', type='GLshort'),
arg4=ArgDef(name='r', type='GLshort'),
arg5=ArgDef(name='q', type='GLshort')
)

Function(name='glMultiTexCoord4sARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4s')

Function(name='glMultiTexCoord4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glMultiTexCoord4svARB', enabled=True, type=Param, inheritFrom='glMultiTexCoord4sv')

Function(name='glMultiTexCoord4x', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='s', type='GLfixed'),
arg3=ArgDef(name='t', type='GLfixed'),
arg4=ArgDef(name='r', type='GLfixed'),
arg5=ArgDef(name='q', type='GLfixed')
)

Function(name='glMultiTexCoord4xOES', enabled=True, type=Param, inheritFrom='glMultiTexCoord4x')

Function(name='glMultiTexCoord4xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glMultiTexCoordP1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='GLuint')
)

Function(name='glMultiTexCoordP1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='const GLuint*', wrapParams='1, coords')
)

Function(name='glMultiTexCoordP2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='GLuint')
)

Function(name='glMultiTexCoordP2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='const GLuint*', wrapParams='2, coords')
)

Function(name='glMultiTexCoordP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='GLuint')
)

Function(name='glMultiTexCoordP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='const GLuint*', wrapParams='3, coords')
)

Function(name='glMultiTexCoordP4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='GLuint')
)

Function(name='glMultiTexCoordP4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLenum'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='coords', type='const GLuint*', wrapParams='4, coords')
)

Function(name='glMultiTexCoordPointerEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glMultiTexEnvfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLfloat')
)

Function(name='glMultiTexEnvfvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexEnviEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint')
)

Function(name='glMultiTexEnvivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexGendEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLdouble')
)

Function(name='glMultiTexGendvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLdouble*')
)

Function(name='glMultiTexGenfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLfloat')
)

Function(name='glMultiTexGenfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glMultiTexGeniEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint')
)

Function(name='glMultiTexGenivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='coord', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*')
)

Function(name='glMultiTexImage1DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='type', type='GLenum'),
arg9=ArgDef(name='pixels', type='const void*')
)

Function(name='glMultiTexImage2DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='type', type='GLenum'),
arg10=ArgDef(name='pixels', type='const void*')
)

Function(name='glMultiTexImage3DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='pixels', type='const void*')
)

Function(name='glMultiTexParameterIivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexParameterIuivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexParameterfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLfloat')
)

Function(name='glMultiTexParameterfvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexParameteriEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint')
)

Function(name='glMultiTexParameterivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glMultiTexRenderbufferEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glMultiTexSubImage1DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='format', type='GLenum'),
arg7=ArgDef(name='type', type='GLenum'),
arg8=ArgDef(name='pixels', type='const void*')
)

Function(name='glMultiTexSubImage2DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='type', type='GLenum'),
arg10=ArgDef(name='pixels', type='const void*')
)

Function(name='glMultiTexSubImage3DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texunit', type='GLenum'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='depth', type='GLsizei'),
arg10=ArgDef(name='format', type='GLenum'),
arg11=ArgDef(name='type', type='GLenum'),
arg12=ArgDef(name='pixels', type='const void*')
)

Function(name='glMulticastBarrierNV', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glMulticastBlitFramebufferNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='srcGpu', type='GLuint'),
arg2=ArgDef(name='dstGpu', type='GLuint'),
arg3=ArgDef(name='srcX0', type='GLint'),
arg4=ArgDef(name='srcY0', type='GLint'),
arg5=ArgDef(name='srcX1', type='GLint'),
arg6=ArgDef(name='srcY1', type='GLint'),
arg7=ArgDef(name='dstX0', type='GLint'),
arg8=ArgDef(name='dstY0', type='GLint'),
arg9=ArgDef(name='dstX1', type='GLint'),
arg10=ArgDef(name='dstY1', type='GLint'),
arg11=ArgDef(name='mask', type='GLbitfield'),
arg12=ArgDef(name='filter', type='GLenum')
)

Function(name='glMulticastBufferSubDataNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpuMask', type='GLbitfield'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='offset', type='GLintptr'),
arg4=ArgDef(name='size', type='GLsizeiptr'),
arg5=ArgDef(name='data', type='const void*', wrapType='CBinaryResource')
)

Function(name='glMulticastCopyBufferSubDataNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='readGpu', type='GLuint'),
arg2=ArgDef(name='writeGpuMask', type='GLbitfield'),
arg3=ArgDef(name='readBuffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='writeBuffer', type='GLuint', wrapType='CGLBuffer'),
arg5=ArgDef(name='readOffset', type='GLintptr'),
arg6=ArgDef(name='writeOffset', type='GLintptr'),
arg7=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glMulticastCopyImageSubDataNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='srcGpu', type='GLuint'),
arg2=ArgDef(name='dstGpuMask', type='GLbitfield'),
arg3=ArgDef(name='srcName', type='GLuint'),
arg4=ArgDef(name='srcTarget', type='GLenum'),
arg5=ArgDef(name='srcLevel', type='GLint'),
arg6=ArgDef(name='srcX', type='GLint'),
arg7=ArgDef(name='srcY', type='GLint'),
arg8=ArgDef(name='srcZ', type='GLint'),
arg9=ArgDef(name='dstName', type='GLuint'),
arg10=ArgDef(name='dstTarget', type='GLenum'),
arg11=ArgDef(name='dstLevel', type='GLint'),
arg12=ArgDef(name='dstX', type='GLint'),
arg13=ArgDef(name='dstY', type='GLint'),
arg14=ArgDef(name='dstZ', type='GLint'),
arg15=ArgDef(name='srcWidth', type='GLsizei'),
arg16=ArgDef(name='srcHeight', type='GLsizei'),
arg17=ArgDef(name='srcDepth', type='GLsizei')
)

Function(name='glMulticastFramebufferSampleLocationsfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpu', type='GLuint'),
arg2=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg3=ArgDef(name='start', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glMulticastGetQueryObjecti64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpu', type='GLuint'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint64*')
)

Function(name='glMulticastGetQueryObjectivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpu', type='GLuint'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLint*')
)

Function(name='glMulticastGetQueryObjectui64vNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpu', type='GLuint'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLuint64*')
)

Function(name='glMulticastGetQueryObjectuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='gpu', type='GLuint'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='GLuint*')
)

Function(name='glMulticastWaitSyncNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='signalGpu', type='GLuint'),
arg2=ArgDef(name='waitGpuMask', type='GLbitfield')
)

Function(name='glNamedBufferData', enabled=True, type=Resource, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size'),
arg4=ArgDef(name='usage', type='GLenum')
)

Function(name='glNamedBufferDataEXT', enabled=True, type=Resource, inheritFrom='glNamedBufferData')

Function(name='glNamedBufferPageCommitmentARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='commit', type='GLboolean')
)

Function(name='glNamedBufferPageCommitmentEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='commit', type='GLboolean')
)

Function(name='glNamedBufferStorage', enabled=True, type=Resource, stateTrack=True, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size'),
arg4=ArgDef(name='flags', type='GLbitfield', wrapType='CBufferStorageFlags')
)

Function(name='glNamedBufferStorageEXT', enabled=True, type=Resource, interceptorExecOverride=True, inheritFrom='glNamedBufferStorage')

Function(name='glNamedBufferStorageExternalEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='clientBuffer', type='GLeglClientBufferEXT'),
arg5=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glNamedBufferStorageMemEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='size', type='GLsizeiptr'),
arg3=ArgDef(name='memory', type='GLuint'),
arg4=ArgDef(name='offset', type='GLuint64')
)

Function(name='glNamedBufferSubData', enabled=True, type=Resource, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLintptr'),
arg3=ArgDef(name='size', type='GLsizeiptr'),
arg4=ArgDef(name='data', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, data, size')
)

Function(name='glNamedBufferSubDataEXT', enabled=True, type=Resource, inheritFrom='glNamedBufferSubData')

Function(name='glNamedCopyBufferSubDataEXT', enabled=True, type=Copy,
retV=RetDef(type='void'),
arg1=ArgDef(name='readBuffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='writeBuffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='readOffset', type='GLintptr'),
arg4=ArgDef(name='writeOffset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glNamedFramebufferDrawBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='buf', type='GLenum')
)

Function(name='glNamedFramebufferDrawBuffers', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='bufs', type='const GLenum*', wrapParams='n, bufs')
)

Function(name='glNamedFramebufferParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glNamedFramebufferParameteriEXT', enabled=True, type=Param, inheritFrom='glNamedFramebufferParameteri',
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer')
)

Function(name='glNamedFramebufferReadBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint'),
arg2=ArgDef(name='src', type='GLenum')
)

Function(name='glNamedFramebufferRenderbuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='renderbuffertarget', type='GLenum'),
arg4=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glNamedFramebufferRenderbufferEXT', enabled=True, type=Param, inheritFrom='glNamedFramebufferRenderbuffer',
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg4=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glNamedFramebufferSampleLocationsfvARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glNamedFramebufferSampleLocationsfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='start', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glNamedFramebufferSamplePositionsfvAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='numsamples', type='GLuint'),
arg3=ArgDef(name='pixelindex', type='GLuint'),
arg4=ArgDef(name='values', type='const GLfloat*')
)

Function(name='glNamedFramebufferTexture', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint')
)

Function(name='glNamedFramebufferTexture1DEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint')
)

Function(name='glNamedFramebufferTexture2DEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint')
)

Function(name='glNamedFramebufferTexture3DEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='textarget', type='GLenum'),
arg4=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg5=ArgDef(name='level', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint')
)

Function(name='glNamedFramebufferTextureEXT', enabled=True, type=Param, inheritFrom='glNamedFramebufferTexture',
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer')
)

Function(name='glNamedFramebufferTextureFaceEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='face', type='GLenum')
)

Function(name='glNamedFramebufferTextureLayer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer'),
arg2=ArgDef(name='attachment', type='GLenum'),
arg3=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='level', type='GLint'),
arg5=ArgDef(name='layer', type='GLint')
)

Function(name='glNamedFramebufferTextureLayerEXT', enabled=False, type=None, inheritFrom='glNamedFramebufferTextureLayer',
arg1=ArgDef(name='framebuffer', type='GLuint', wrapType='CGLFramebuffer')
)

Function(name='glNamedProgramLocalParameter4dEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='x', type='GLdouble'),
arg5=ArgDef(name='y', type='GLdouble'),
arg6=ArgDef(name='z', type='GLdouble'),
arg7=ArgDef(name='w', type='GLdouble')
)

Function(name='glNamedProgramLocalParameter4dvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='const GLdouble*', wrapParams='4, params')
)

Function(name='glNamedProgramLocalParameter4fEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='x', type='GLfloat'),
arg5=ArgDef(name='y', type='GLfloat'),
arg6=ArgDef(name='z', type='GLfloat'),
arg7=ArgDef(name='w', type='GLfloat')
)

Function(name='glNamedProgramLocalParameter4fvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapParams='4, params')
)

Function(name='glNamedProgramLocalParameterI4iEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='x', type='GLint'),
arg5=ArgDef(name='y', type='GLint'),
arg6=ArgDef(name='z', type='GLint'),
arg7=ArgDef(name='w', type='GLint')
)

Function(name='glNamedProgramLocalParameterI4ivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='const GLint*', wrapParams='4, params')
)

Function(name='glNamedProgramLocalParameterI4uiEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='x', type='GLuint'),
arg5=ArgDef(name='y', type='GLuint'),
arg6=ArgDef(name='z', type='GLuint'),
arg7=ArgDef(name='w', type='GLuint')
)

Function(name='glNamedProgramLocalParameterI4uivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='params', type='const GLuint*', wrapParams='4, params')
)

Function(name='glNamedProgramLocalParameters4fvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLfloat*', wrapParams='count*4, params')
)

Function(name='glNamedProgramLocalParametersI4ivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLint*', wrapParams='count*4, params')
)

Function(name='glNamedProgramLocalParametersI4uivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLuint*', wrapParams='count*4, params')
)

Function(name='glNamedProgramStringEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='format', type='GLenum'),
arg4=ArgDef(name='len', type='GLsizei'),
arg5=ArgDef(name='string', type='const void*')
)

Function(name='glNamedRenderbufferStorage', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei')
)

Function(name='glNamedRenderbufferStorageEXT', enabled=False, type=None, inheritFrom='glNamedRenderbufferStorage',
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT'),
)

Function(name='glNamedRenderbufferStorageMultisample', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei')
)

Function(name='glNamedRenderbufferStorageMultisampleCoverageEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer'),
arg2=ArgDef(name='coverageSamples', type='GLsizei'),
arg3=ArgDef(name='colorSamples', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei')
)

Function(name='glNamedRenderbufferStorageMultisampleEXT', enabled=False, type=None, inheritFrom='glNamedRenderbufferStorageMultisample',
arg1=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT'),
)

Function(name='glNamedStringARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='namelen', type='GLint'),
arg3=ArgDef(name='name', type='const GLchar*', wrapParams='name, \'\\0\', 1'),
arg4=ArgDef(name='stringlen', type='GLint'),
arg5=ArgDef(name='string', type='const GLchar*')
)

Function(name='glNewList', enabled=True, type=Param, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='list', type='GLuint'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glNewObjectBufferATI', enabled=False, type=None,
retV=RetDef(type='GLuint'),
arg1=ArgDef(name='size', type='GLsizei'),
arg2=ArgDef(name='pointer', type='const void*'),
arg3=ArgDef(name='usage', type='GLenum')
)

Function(name='glNormal3b', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLbyte'),
arg2=ArgDef(name='ny', type='GLbyte'),
arg3=ArgDef(name='nz', type='GLbyte')
)

Function(name='glNormal3bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*', wrapParams='3, v')
)

Function(name='glNormal3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLdouble'),
arg2=ArgDef(name='ny', type='GLdouble'),
arg3=ArgDef(name='nz', type='GLdouble')
)

Function(name='glNormal3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glNormal3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLfloat'),
arg2=ArgDef(name='ny', type='GLfloat'),
arg3=ArgDef(name='nz', type='GLfloat')
)

Function(name='glNormal3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLfloat'),
arg2=ArgDef(name='ny', type='GLfloat'),
arg3=ArgDef(name='nz', type='GLfloat'),
arg4=ArgDef(name='x', type='GLfloat'),
arg5=ArgDef(name='y', type='GLfloat'),
arg6=ArgDef(name='z', type='GLfloat')
)

Function(name='glNormal3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='const GLfloat*', wrapParams='3, v'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glNormal3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glNormal3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLhalfNV'),
arg2=ArgDef(name='ny', type='GLhalfNV'),
arg3=ArgDef(name='nz', type='GLhalfNV')
)

Function(name='glNormal3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glNormal3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLint'),
arg2=ArgDef(name='ny', type='GLint'),
arg3=ArgDef(name='nz', type='GLint')
)

Function(name='glNormal3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glNormal3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLshort'),
arg2=ArgDef(name='ny', type='GLshort'),
arg3=ArgDef(name='nz', type='GLshort')
)

Function(name='glNormal3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glNormal3x', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='nx', type='GLfixed'),
arg2=ArgDef(name='ny', type='GLfixed'),
arg3=ArgDef(name='nz', type='GLfixed')
)

Function(name='glNormal3xOES', enabled=True, type=Param, inheritFrom='glNormal3x')

Function(name='glNormal3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glNormalFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei')
)

Function(name='glNormalP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='GLuint')
)

Function(name='glNormalP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLuint*', wrapParams='3, coords')
)

Function(name='glNormalPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glNormalPointerBounds', enabled=True, type=Param, runWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const GLvoid*', wrapType='CAttribPtr'),
arg4=ArgDef(name='count', type='GLsizei')
)

Function(name='glNormalPointerEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glNormalPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLint'),
arg3=ArgDef(name='pointer', type='const void**'),
arg4=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glNormalPointervINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='pointer', type='const void**')
)

Function(name='glNormalStream3bATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='nx', type='GLbyte'),
arg3=ArgDef(name='ny', type='GLbyte'),
arg4=ArgDef(name='nz', type='GLbyte')
)

Function(name='glNormalStream3bvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glNormalStream3dATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='nx', type='GLdouble'),
arg3=ArgDef(name='ny', type='GLdouble'),
arg4=ArgDef(name='nz', type='GLdouble')
)

Function(name='glNormalStream3dvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLdouble*')
)

Function(name='glNormalStream3fATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='nx', type='GLfloat'),
arg3=ArgDef(name='ny', type='GLfloat'),
arg4=ArgDef(name='nz', type='GLfloat')
)

Function(name='glNormalStream3fvATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfloat*')
)

Function(name='glNormalStream3iATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='nx', type='GLint'),
arg3=ArgDef(name='ny', type='GLint'),
arg4=ArgDef(name='nz', type='GLint')
)

Function(name='glNormalStream3ivATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLint*')
)

Function(name='glNormalStream3sATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='nx', type='GLshort'),
arg3=ArgDef(name='ny', type='GLshort'),
arg4=ArgDef(name='nz', type='GLshort')
)

Function(name='glNormalStream3svATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLshort*')
)

Function(name='glObjectLabel', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='identifier', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='length', type='GLsizei'),
arg4=ArgDef(name='label', type='const GLchar*', wrapParams='label, \'\\0\', 1')
)

Function(name='glObjectLabelKHR', enabled=False, type=None, inheritFrom='glObjectLabel')

Function(name='glObjectPtrLabel', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='ptr', type='const void*'),
arg2=ArgDef(name='length', type='GLsizei'),
arg3=ArgDef(name='label', type='const GLchar*')
)

Function(name='glObjectPtrLabelKHR', enabled=False, type=None, inheritFrom='glObjectPtrLabel')

Function(name='glObjectPurgeableAPPLE', enabled=False, type=Param,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='objectType', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='option', type='GLenum')
)

Function(name='glObjectUnpurgeableAPPLE', enabled=False, type=Param,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='objectType', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint'),
arg3=ArgDef(name='option', type='GLenum')
)

Function(name='glOrtho', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='left', type='GLdouble'),
arg2=ArgDef(name='right', type='GLdouble'),
arg3=ArgDef(name='bottom', type='GLdouble'),
arg4=ArgDef(name='top', type='GLdouble'),
arg5=ArgDef(name='zNear', type='GLdouble'),
arg6=ArgDef(name='zFar', type='GLdouble')
)

Function(name='glOrthof', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='l', type='GLfloat'),
arg2=ArgDef(name='r', type='GLfloat'),
arg3=ArgDef(name='b', type='GLfloat'),
arg4=ArgDef(name='t', type='GLfloat'),
arg5=ArgDef(name='n', type='GLfloat'),
arg6=ArgDef(name='f', type='GLfloat')
)

Function(name='glOrthofOES', enabled=True, type=Param, inheritFrom='glOrthof')

Function(name='glOrthox', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='l', type='GLfixed'),
arg2=ArgDef(name='r', type='GLfixed'),
arg3=ArgDef(name='b', type='GLfixed'),
arg4=ArgDef(name='t', type='GLfixed'),
arg5=ArgDef(name='n', type='GLfixed'),
arg6=ArgDef(name='f', type='GLfixed')
)

Function(name='glOrthoxOES', enabled=True, type=Param, inheritFrom='glOrthox')

Function(name='glPNTrianglesfATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glPNTrianglesiATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glPassTexCoordATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='dst', type='GLuint'),
arg2=ArgDef(name='coord', type='GLuint'),
arg3=ArgDef(name='swizzle', type='GLenum')
)

Function(name='glPassThrough', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='token', type='GLfloat')
)

Function(name='glPassThroughxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='token', type='GLfixed')
)

Function(name='glPatchParameterfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='values', type='const GLfloat*', wrapParam='CGLfloat::CSParamArray', wrapParams='GetPatchParameterValuesCount(pname), values')
)

Function(name='glPatchParameterfvARB', enabled=True, type=Param, inheritFrom='glPatchParameterfv')

Function(name='glPatchParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='value', type='GLint')
)

Function(name='glPatchParameteriARB', enabled=True, type=Param, inheritFrom='glPatchParameteri')

Function(name='glPatchParameteriEXT', enabled=True, type=Param, inheritFrom='glPatchParameteri')

Function(name='glPatchParameteriOES', enabled=True, type=Param, inheritFrom='glPatchParameteri')

Function(name='glPathColorGenNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='color', type='GLenum'),
arg2=ArgDef(name='genMode', type='GLenum'),
arg3=ArgDef(name='colorFormat', type='GLenum'),
arg4=ArgDef(name='coeffs', type='const GLfloat*')
)

Function(name='glPathCommandsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='numCommands', type='GLsizei'),
arg3=ArgDef(name='commands', type='const GLubyte*'),
arg4=ArgDef(name='numCoords', type='GLsizei'),
arg5=ArgDef(name='coordType', type='GLenum'),
arg6=ArgDef(name='coords', type='const void*')
)

Function(name='glPathCoordsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='numCoords', type='GLsizei'),
arg3=ArgDef(name='coordType', type='GLenum'),
arg4=ArgDef(name='coords', type='const void*')
)

Function(name='glPathCoverDepthFuncNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum')
)

Function(name='glPathDashArrayNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='dashCount', type='GLsizei'),
arg3=ArgDef(name='dashArray', type='const GLfloat*')
)

Function(name='glPathFogGenNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='genMode', type='GLenum')
)

Function(name='glPathGlyphIndexArrayNV', enabled=False, type=None,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='firstPathName', type='GLuint'),
arg2=ArgDef(name='fontTarget', type='GLenum'),
arg3=ArgDef(name='fontName', type='const void*'),
arg4=ArgDef(name='fontStyle', type='GLbitfield'),
arg5=ArgDef(name='firstGlyphIndex', type='GLuint'),
arg6=ArgDef(name='numGlyphs', type='GLsizei'),
arg7=ArgDef(name='pathParameterTemplate', type='GLuint'),
arg8=ArgDef(name='emScale', type='GLfloat')
)

Function(name='glPathGlyphIndexRangeNV', enabled=False, type=None,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='fontTarget', type='GLenum'),
arg2=ArgDef(name='fontName', type='const void*'),
arg3=ArgDef(name='fontStyle', type='GLbitfield'),
arg4=ArgDef(name='pathParameterTemplate', type='GLuint'),
arg5=ArgDef(name='emScale', type='GLfloat'),
arg6=ArgDef(name='baseAndCount [2]', type='GLuint')
)

Function(name='glPathGlyphRangeNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='firstPathName', type='GLuint'),
arg2=ArgDef(name='fontTarget', type='GLenum'),
arg3=ArgDef(name='fontName', type='const void*'),
arg4=ArgDef(name='fontStyle', type='GLbitfield'),
arg5=ArgDef(name='firstGlyph', type='GLuint'),
arg6=ArgDef(name='numGlyphs', type='GLsizei'),
arg7=ArgDef(name='handleMissingGlyphs', type='GLenum'),
arg8=ArgDef(name='pathParameterTemplate', type='GLuint'),
arg9=ArgDef(name='emScale', type='GLfloat')
)

Function(name='glPathGlyphsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='firstPathName', type='GLuint'),
arg2=ArgDef(name='fontTarget', type='GLenum'),
arg3=ArgDef(name='fontName', type='const void*'),
arg4=ArgDef(name='fontStyle', type='GLbitfield'),
arg5=ArgDef(name='numGlyphs', type='GLsizei'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='charcodes', type='const void*'),
arg8=ArgDef(name='handleMissingGlyphs', type='GLenum'),
arg9=ArgDef(name='pathParameterTemplate', type='GLuint'),
arg10=ArgDef(name='emScale', type='GLfloat')
)

Function(name='glPathMemoryGlyphIndexArrayNV', enabled=False, type=None,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='firstPathName', type='GLuint'),
arg2=ArgDef(name='fontTarget', type='GLenum'),
arg3=ArgDef(name='fontSize', type='GLsizeiptr'),
arg4=ArgDef(name='fontData', type='const void*'),
arg5=ArgDef(name='faceIndex', type='GLsizei'),
arg6=ArgDef(name='firstGlyphIndex', type='GLuint'),
arg7=ArgDef(name='numGlyphs', type='GLsizei'),
arg8=ArgDef(name='pathParameterTemplate', type='GLuint'),
arg9=ArgDef(name='emScale', type='GLfloat')
)

Function(name='glPathParameterfNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLfloat')
)

Function(name='glPathParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='const GLfloat*')
)

Function(name='glPathParameteriNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLint')
)

Function(name='glPathParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='const GLint*')
)

Function(name='glPathStencilDepthOffsetNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat'),
arg2=ArgDef(name='units', type='GLfloat')
)

Function(name='glPathStencilFuncNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLint'),
arg3=ArgDef(name='mask', type='GLuint')
)

Function(name='glPathStringNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='length', type='GLsizei'),
arg4=ArgDef(name='pathString', type='const void*')
)

Function(name='glPathSubCommandsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='commandStart', type='GLsizei'),
arg3=ArgDef(name='commandsToDelete', type='GLsizei'),
arg4=ArgDef(name='numCommands', type='GLsizei'),
arg5=ArgDef(name='commands', type='const GLubyte*'),
arg6=ArgDef(name='numCoords', type='GLsizei'),
arg7=ArgDef(name='coordType', type='GLenum'),
arg8=ArgDef(name='coords', type='const void*')
)

Function(name='glPathSubCoordsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='coordStart', type='GLsizei'),
arg3=ArgDef(name='numCoords', type='GLsizei'),
arg4=ArgDef(name='coordType', type='GLenum'),
arg5=ArgDef(name='coords', type='const void*')
)

Function(name='glPathTexGenNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texCoordSet', type='GLenum'),
arg2=ArgDef(name='genMode', type='GLenum'),
arg3=ArgDef(name='components', type='GLint'),
arg4=ArgDef(name='coeffs', type='const GLfloat*')
)

Function(name='glPauseTransformFeedback', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPauseTransformFeedbackNV', enabled=True, type=Param, inheritFrom='glPauseTransformFeedback')

Function(name='glPixelDataRangeNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='length', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glPixelMapfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='mapsize', type='GLsizei'),
arg3=ArgDef(name='values', type='const GLfloat*', wrapParams='mapsize, values')
)

Function(name='glPixelMapuiv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='mapsize', type='GLsizei'),
arg3=ArgDef(name='values', type='const GLuint*')
)

Function(name='glPixelMapusv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='mapsize', type='GLsizei'),
arg3=ArgDef(name='values', type='const GLushort*')
)

Function(name='glPixelMapx', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='map', type='GLenum'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='values', type='const GLfixed*')
)

Function(name='glPixelStoref', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glPixelStorei', enabled=True, type=Param, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glPixelStorex', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfixed')
)

Function(name='glPixelTexGenParameterfSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glPixelTexGenParameterfvSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glPixelTexGenParameteriSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glPixelTexGenParameterivSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*')
)

Function(name='glPixelTexGenSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glPixelTransferf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glPixelTransferi', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glPixelTransferxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfixed')
)

Function(name='glPixelTransformParameterfEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glPixelTransformParameterfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glPixelTransformParameteriEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glPixelTransformParameterivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glPixelZoom', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfactor', type='GLfloat'),
arg2=ArgDef(name='yfactor', type='GLfloat')
)

Function(name='glPixelZoomxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfactor', type='GLfixed'),
arg2=ArgDef(name='yfactor', type='GLfixed')
)

Function(name='glPointAlongPathNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='startSegment', type='GLsizei'),
arg3=ArgDef(name='numSegments', type='GLsizei'),
arg4=ArgDef(name='distance', type='GLfloat'),
arg5=ArgDef(name='x', type='GLfloat*'),
arg6=ArgDef(name='y', type='GLfloat*'),
arg7=ArgDef(name='tangentX', type='GLfloat*'),
arg8=ArgDef(name='tangentY', type='GLfloat*')
)

Function(name='glPointParameterf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glPointParameterfARB', enabled=True, type=Param, inheritFrom='glPointParameterf')

Function(name='glPointParameterfEXT', enabled=True, type=Param, inheritFrom='glPointParameterf')

Function(name='glPointParameterfSGIS', enabled=True, type=Param, inheritFrom='glPointParameterf')

Function(name='glPointParameterfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glPointParameterfvARB', enabled=True, type=Param, inheritFrom='glPointParameterfv')

Function(name='glPointParameterfvEXT', enabled=True, type=Param, inheritFrom='glPointParameterfv')

Function(name='glPointParameterfvSGIS', enabled=True, type=Param, inheritFrom='glPointParameterfv')

Function(name='glPointParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glPointParameteriNV', enabled=True, type=Param, inheritFrom='glPointParameteri')

Function(name='glPointParameteriv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glPointParameterivNV', enabled=True, type=Param, inheritFrom='glPointParameteriv')

Function(name='glPointParameterx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfixed')
)

Function(name='glPointParameterxOES', enabled=True, type=Param, inheritFrom='glPointParameterx')

Function(name='glPointParameterxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glPointParameterxvOES', enabled=True, type=Param, inheritFrom='glPointParameterxv')

Function(name='glPointSize', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLfloat')
)

Function(name='glPointSizePointerAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const GLvoid*')
)

Function(name='glPointSizePointerOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glPointSizePointerOESBounds', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const GLvoid*'),
arg4=ArgDef(name='count', type='GLsizei')
)

Function(name='glPointSizex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLfixed')
)

Function(name='glPointSizexOES', enabled=True, type=Param, inheritFrom='glPointSizex')

Function(name='glPollAsyncSGIX', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='markerp', type='GLuint*')
)

Function(name='glPollInstrumentsSGIX', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='marker_p', type='GLint*')
)

Function(name='glPolygonMode', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glPolygonModeNV', enabled=False, type=None, inheritFrom='glPolygonMode')

Function(name='glPolygonOffset', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat'),
arg2=ArgDef(name='units', type='GLfloat')
)

Function(name='glPolygonOffsetClamp', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat'),
arg2=ArgDef(name='units', type='GLfloat'),
arg3=ArgDef(name='clamp', type='GLfloat')
)

Function(name='glPolygonOffsetClampEXT', enabled=True, type=Param, inheritFrom='glPolygonOffsetClamp')

Function(name='glPolygonOffsetEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat'),
arg2=ArgDef(name='bias', type='GLfloat')
)

Function(name='glPolygonOffsetx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfixed'),
arg2=ArgDef(name='units', type='GLfixed')
)

Function(name='glPolygonOffsetxOES', enabled=True, type=Param, inheritFrom='glPolygonOffsetx')

Function(name='glPolygonStipple', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='const GLubyte*', wrapParams='32 * 4, mask')
)

Function(name='glPopAttrib', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPopClientAttrib', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void')
)

Function(name='glPopDebugGroup', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPopDebugGroupKHR', enabled=False, type=None, inheritFrom='glPopDebugGroup')

Function(name='glPopGroupMarkerEXT', enabled=True, type=Param, interceptorExecOverride=True,
retV=RetDef(type='void')
)

Function(name='glPopMatrix', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPopName', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPresentFrameDualFillNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='minPresentTime', type='GLuint64EXT'),
arg3=ArgDef(name='beginPresentTimeId', type='GLuint'),
arg4=ArgDef(name='presentDurationId', type='GLuint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='target0', type='GLenum'),
arg7=ArgDef(name='fill0', type='GLuint'),
arg8=ArgDef(name='target1', type='GLenum'),
arg9=ArgDef(name='fill1', type='GLuint'),
arg10=ArgDef(name='target2', type='GLenum'),
arg11=ArgDef(name='fill2', type='GLuint'),
arg12=ArgDef(name='target3', type='GLenum'),
arg13=ArgDef(name='fill3', type='GLuint')
)

Function(name='glPresentFrameKeyedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_slot', type='GLuint'),
arg2=ArgDef(name='minPresentTime', type='GLuint64EXT'),
arg3=ArgDef(name='beginPresentTimeId', type='GLuint'),
arg4=ArgDef(name='presentDurationId', type='GLuint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='target0', type='GLenum'),
arg7=ArgDef(name='fill0', type='GLuint'),
arg8=ArgDef(name='key0', type='GLuint'),
arg9=ArgDef(name='target1', type='GLenum'),
arg10=ArgDef(name='fill1', type='GLuint'),
arg11=ArgDef(name='key1', type='GLuint')
)

Function(name='glPrimitiveBoundingBox', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='minX', type='GLfloat'),
arg2=ArgDef(name='minY', type='GLfloat'),
arg3=ArgDef(name='minZ', type='GLfloat'),
arg4=ArgDef(name='minW', type='GLfloat'),
arg5=ArgDef(name='maxX', type='GLfloat'),
arg6=ArgDef(name='maxY', type='GLfloat'),
arg7=ArgDef(name='maxZ', type='GLfloat'),
arg8=ArgDef(name='maxW', type='GLfloat')
)

Function(name='glPrimitiveBoundingBoxARB', enabled=False, type=None, inheritFrom='glPrimitiveBoundingBox')

Function(name='glPrimitiveBoundingBoxEXT', enabled=False, type=None, inheritFrom='glPrimitiveBoundingBox')

Function(name='glPrimitiveBoundingBoxOES', enabled=False, type=None, inheritFrom='glPrimitiveBoundingBox')

Function(name='glPrimitiveRestartIndex', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint')
)

Function(name='glPrimitiveRestartIndexNV', enabled=True, type=Param, inheritFrom='glPrimitiveRestartIndex')

Function(name='glPrimitiveRestartNV', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glPrioritizeTextures', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='textures', type='const GLuint*'),
arg3=ArgDef(name='priorities', type='const GLfloat*')
)

Function(name='glPrioritizeTexturesEXT', enabled=False, type=None, inheritFrom='glPrioritizeTextures')

Function(name='glPrioritizeTexturesxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='textures', type='const GLuint*'),
arg3=ArgDef(name='priorities', type='const GLfixed*')
)

Function(name='glProgramBinary', enabled=True, type=Resource, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='binaryFormat', type='GLenum'),
arg3=ArgDef(name='binary', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, binary, length'),
arg4=ArgDef(name='length', type='GLsizei')
)

Function(name='glProgramBinaryOES', enabled=True, type=Resource, interceptorExecOverride=True, inheritFrom='glProgramBinary')

Function(name='glProgramBufferParametersIivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='bindingIndex', type='GLuint'),
arg3=ArgDef(name='wordIndex', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLint*')
)

Function(name='glProgramBufferParametersIuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='bindingIndex', type='GLuint'),
arg3=ArgDef(name='wordIndex', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramBufferParametersfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='bindingIndex', type='GLuint'),
arg3=ArgDef(name='wordIndex', type='GLuint'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glProgramEnvParameter4dARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLdouble'),
arg4=ArgDef(name='y', type='GLdouble'),
arg5=ArgDef(name='z', type='GLdouble'),
arg6=ArgDef(name='w', type='GLdouble')
)

Function(name='glProgramEnvParameter4dvARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLdouble*', wrapParams='4, params')
)

Function(name='glProgramEnvParameter4fARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat'),
arg5=ArgDef(name='z', type='GLfloat'),
arg6=ArgDef(name='w', type='GLfloat')
)

Function(name='glProgramEnvParameter4fvARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapParams='4, params')
)

Function(name='glProgramEnvParameterI4iNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='z', type='GLint'),
arg6=ArgDef(name='w', type='GLint')
)

Function(name='glProgramEnvParameterI4ivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glProgramEnvParameterI4uiNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLuint'),
arg4=ArgDef(name='y', type='GLuint'),
arg5=ArgDef(name='z', type='GLuint'),
arg6=ArgDef(name='w', type='GLuint')
)

Function(name='glProgramEnvParameterI4uivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramEnvParameters4fvEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapParams='4 * count, params')
)

Function(name='glProgramEnvParametersI4ivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLint*')
)

Function(name='glProgramEnvParametersI4uivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramLocalParameter4dARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLdouble'),
arg4=ArgDef(name='y', type='GLdouble'),
arg5=ArgDef(name='z', type='GLdouble'),
arg6=ArgDef(name='w', type='GLdouble')
)

Function(name='glProgramLocalParameter4dvARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLdouble*', wrapParams='4, params')
)

Function(name='glProgramLocalParameter4fARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat'),
arg5=ArgDef(name='z', type='GLfloat'),
arg6=ArgDef(name='w', type='GLfloat')
)

Function(name='glProgramLocalParameter4fvARB', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapParams='4, params')
)

Function(name='glProgramLocalParameterI4iNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLint'),
arg4=ArgDef(name='y', type='GLint'),
arg5=ArgDef(name='z', type='GLint'),
arg6=ArgDef(name='w', type='GLint')
)

Function(name='glProgramLocalParameterI4ivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLint*')
)

Function(name='glProgramLocalParameterI4uiNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLuint'),
arg4=ArgDef(name='y', type='GLuint'),
arg5=ArgDef(name='z', type='GLuint'),
arg6=ArgDef(name='w', type='GLuint')
)

Function(name='glProgramLocalParameterI4uivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramLocalParameters4fvEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapParams='count * 4, params')
)

Function(name='glProgramLocalParametersI4ivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLint*')
)

Function(name='glProgramLocalParametersI4uivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramNamedParameter4dNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='x', type='GLdouble'),
arg5=ArgDef(name='y', type='GLdouble'),
arg6=ArgDef(name='z', type='GLdouble'),
arg7=ArgDef(name='w', type='GLdouble')
)

Function(name='glProgramNamedParameter4dvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='v', type='const GLdouble*')
)

Function(name='glProgramNamedParameter4fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='x', type='GLfloat'),
arg5=ArgDef(name='y', type='GLfloat'),
arg6=ArgDef(name='z', type='GLfloat'),
arg7=ArgDef(name='w', type='GLfloat')
)

Function(name='glProgramNamedParameter4fvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='len', type='GLsizei'),
arg3=ArgDef(name='name', type='const GLubyte*'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glProgramParameter4dNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLdouble'),
arg4=ArgDef(name='y', type='GLdouble'),
arg5=ArgDef(name='z', type='GLdouble'),
arg6=ArgDef(name='w', type='GLdouble')
)

Function(name='glProgramParameter4dvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='v', type='const GLdouble*')
)

Function(name='glProgramParameter4fNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat'),
arg5=ArgDef(name='z', type='GLfloat'),
arg6=ArgDef(name='w', type='GLfloat')
)

Function(name='glProgramParameter4fvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glProgramParameteri', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='value', type='GLint')
)

Function(name='glProgramParameteriARB', enabled=True, type=Param, inheritFrom='glProgramParameteri')

Function(name='glProgramParameteriEXT', enabled=True, type=Param, inheritFrom='glProgramParameteri')

Function(name='glProgramParameters4dvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLdouble*', wrapParams='count*4, v')
)

Function(name='glProgramParameters4fvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='v', type='const GLfloat*', wrapParams='count*4, v')
)

Function(name='glProgramPathFragmentInputGenNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='genMode', type='GLenum'),
arg4=ArgDef(name='components', type='GLint'),
arg5=ArgDef(name='coeffs', type='const GLfloat*')
)

Function(name='glProgramStringARB', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='format', type='GLenum'),
arg3=ArgDef(name='len', type='GLsizei'),
arg4=ArgDef(name='string', type='const void*', wrapType='CShaderSource', wrapParams='(const char *)string, len, CShaderSource::PROGRAM_STRING, target, format')
)

Function(name='glProgramSubroutineParametersuivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='params', type='const GLuint*')
)

Function(name='glProgramUniform1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLdouble')
)

Function(name='glProgramUniform1dEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1d')

Function(name='glProgramUniform1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count, value')
)

Function(name='glProgramUniform1dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1dv')

Function(name='glProgramUniform1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLfloat')
)

Function(name='glProgramUniform1fEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1f')

Function(name='glProgramUniform1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count, value')
)

Function(name='glProgramUniform1fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1fv')

Function(name='glProgramUniform1i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLint')
)

Function(name='glProgramUniform1i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64')
)

Function(name='glProgramUniform1i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64EXT')
)

Function(name='glProgramUniform1i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64*', wrapParams='count, value')
)

Function(name='glProgramUniform1i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count, value')
)

Function(name='glProgramUniform1iEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1i')

Function(name='glProgramUniform1iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint*', wrapParams='count, value')
)

Function(name='glProgramUniform1ivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1iv')

Function(name='glProgramUniform1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLuint')
)

Function(name='glProgramUniform1ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64')
)

Function(name='glProgramUniform1ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64EXT')
)

Function(name='glProgramUniform1ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64*', wrapParams='count, value')
)

Function(name='glProgramUniform1ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count, value')
)

Function(name='glProgramUniform1uiEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1ui')

Function(name='glProgramUniform1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='count, value')
)

Function(name='glProgramUniform1uivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform1uiv')

Function(name='glProgramUniform2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLdouble'),
arg4=ArgDef(name='v1', type='GLdouble')
)

Function(name='glProgramUniform2dEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2d')

Function(name='glProgramUniform2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2dv')

Function(name='glProgramUniform2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLfloat'),
arg4=ArgDef(name='v1', type='GLfloat')
)

Function(name='glProgramUniform2fEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2f')

Function(name='glProgramUniform2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2fv')

Function(name='glProgramUniform2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLint'),
arg4=ArgDef(name='v1', type='GLint')
)

Function(name='glProgramUniform2i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64'),
arg4=ArgDef(name='y', type='GLint64')
)

Function(name='glProgramUniform2i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64EXT'),
arg4=ArgDef(name='y', type='GLint64EXT')
)

Function(name='glProgramUniform2i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2iEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2i')

Function(name='glProgramUniform2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2ivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2iv')

Function(name='glProgramUniform2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLuint'),
arg4=ArgDef(name='v1', type='GLuint')
)

Function(name='glProgramUniform2ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64'),
arg4=ArgDef(name='y', type='GLuint64')
)

Function(name='glProgramUniform2ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64EXT'),
arg4=ArgDef(name='y', type='GLuint64EXT')
)

Function(name='glProgramUniform2ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2uiEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2ui')

Function(name='glProgramUniform2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='count * 2, value')
)

Function(name='glProgramUniform2uivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform2uiv')

Function(name='glProgramUniform3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLdouble'),
arg4=ArgDef(name='v1', type='GLdouble'),
arg5=ArgDef(name='v2', type='GLdouble')
)

Function(name='glProgramUniform3dEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3d')

Function(name='glProgramUniform3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3dv')

Function(name='glProgramUniform3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLfloat'),
arg4=ArgDef(name='v1', type='GLfloat'),
arg5=ArgDef(name='v2', type='GLfloat')
)

Function(name='glProgramUniform3fEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3f')

Function(name='glProgramUniform3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3fv')

Function(name='glProgramUniform3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLint'),
arg4=ArgDef(name='v1', type='GLint'),
arg5=ArgDef(name='v2', type='GLint')
)

Function(name='glProgramUniform3i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64'),
arg4=ArgDef(name='y', type='GLint64'),
arg5=ArgDef(name='z', type='GLint64')
)

Function(name='glProgramUniform3i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64EXT'),
arg4=ArgDef(name='y', type='GLint64EXT'),
arg5=ArgDef(name='z', type='GLint64EXT')
)

Function(name='glProgramUniform3i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3iEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3i')

Function(name='glProgramUniform3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3ivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3iv')

Function(name='glProgramUniform3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLuint'),
arg4=ArgDef(name='v1', type='GLuint'),
arg5=ArgDef(name='v2', type='GLuint')
)

Function(name='glProgramUniform3ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64'),
arg4=ArgDef(name='y', type='GLuint64'),
arg5=ArgDef(name='z', type='GLuint64')
)

Function(name='glProgramUniform3ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64EXT'),
arg4=ArgDef(name='y', type='GLuint64EXT'),
arg5=ArgDef(name='z', type='GLuint64EXT')
)

Function(name='glProgramUniform3ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3uiEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3ui')

Function(name='glProgramUniform3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='count * 3, value')
)

Function(name='glProgramUniform3uivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform3uiv')

Function(name='glProgramUniform4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLdouble'),
arg4=ArgDef(name='v1', type='GLdouble'),
arg5=ArgDef(name='v2', type='GLdouble'),
arg6=ArgDef(name='v3', type='GLdouble')
)

Function(name='glProgramUniform4dEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4d')

Function(name='glProgramUniform4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4dv')

Function(name='glProgramUniform4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLfloat'),
arg4=ArgDef(name='v1', type='GLfloat'),
arg5=ArgDef(name='v2', type='GLfloat'),
arg6=ArgDef(name='v3', type='GLfloat')
)

Function(name='glProgramUniform4fEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4f')

Function(name='glProgramUniform4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4fv')

Function(name='glProgramUniform4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLint'),
arg4=ArgDef(name='v1', type='GLint'),
arg5=ArgDef(name='v2', type='GLint'),
arg6=ArgDef(name='v3', type='GLint')
)

Function(name='glProgramUniform4i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64'),
arg4=ArgDef(name='y', type='GLint64'),
arg5=ArgDef(name='z', type='GLint64'),
arg6=ArgDef(name='w', type='GLint64')
)

Function(name='glProgramUniform4i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLint64EXT'),
arg4=ArgDef(name='y', type='GLint64EXT'),
arg5=ArgDef(name='z', type='GLint64EXT'),
arg6=ArgDef(name='w', type='GLint64EXT')
)

Function(name='glProgramUniform4i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4iEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4i')

Function(name='glProgramUniform4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLint*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4ivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4iv')

Function(name='glProgramUniform4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='v0', type='GLuint'),
arg4=ArgDef(name='v1', type='GLuint'),
arg5=ArgDef(name='v2', type='GLuint'),
arg6=ArgDef(name='v3', type='GLuint')
)

Function(name='glProgramUniform4ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64'),
arg4=ArgDef(name='y', type='GLuint64'),
arg5=ArgDef(name='z', type='GLuint64'),
arg6=ArgDef(name='w', type='GLuint64')
)

Function(name='glProgramUniform4ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='x', type='GLuint64EXT'),
arg4=ArgDef(name='y', type='GLuint64EXT'),
arg5=ArgDef(name='z', type='GLuint64EXT'),
arg6=ArgDef(name='w', type='GLuint64EXT')
)

Function(name='glProgramUniform4ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4uiEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4ui')

Function(name='glProgramUniform4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='count * 4, value')
)

Function(name='glProgramUniform4uivEXT', enabled=True, type=Param, inheritFrom='glProgramUniform4uiv')

Function(name='glProgramUniformHandleui64ARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='value', type='GLuint64', wrapType='CGLTextureHandle')
)

Function(name='glProgramUniformHandleui64IMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='value', type='GLuint64')
)

Function(name='glProgramUniformHandleui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='value', type='GLuint64')
)

Function(name='glProgramUniformHandleui64vARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='values', type='const GLuint64*', wrapType='CGLTextureHandle::CSArray', wrapParams='count, values')
)

Function(name='glProgramUniformHandleui64vIMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='values', type='const GLuint64*')
)

Function(name='glProgramUniformHandleui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='values', type='const GLuint64*', wrapParams='count, values')
)

Function(name='glProgramUniformMatrix2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 4, value')
)

Function(name='glProgramUniformMatrix2dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2dv')

Function(name='glProgramUniformMatrix2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 4, value')
)

Function(name='glProgramUniformMatrix2fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2fv')

Function(name='glProgramUniformMatrix2x3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 6, value')
)

Function(name='glProgramUniformMatrix2x3dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2x3dv')

Function(name='glProgramUniformMatrix2x3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 6, value')
)

Function(name='glProgramUniformMatrix2x3fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2x3fv')

Function(name='glProgramUniformMatrix2x4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 8, value')
)

Function(name='glProgramUniformMatrix2x4dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2x4dv')

Function(name='glProgramUniformMatrix2x4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 8, value')
)

Function(name='glProgramUniformMatrix2x4fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix2x4fv')

Function(name='glProgramUniformMatrix3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 9, value')
)

Function(name='glProgramUniformMatrix3dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3dv')

Function(name='glProgramUniformMatrix3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 9, value')
)

Function(name='glProgramUniformMatrix3fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3fv')

Function(name='glProgramUniformMatrix3x2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 6, value')
)

Function(name='glProgramUniformMatrix3x2dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3x2dv')

Function(name='glProgramUniformMatrix3x2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 6, value')
)

Function(name='glProgramUniformMatrix3x2fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3x2fv')

Function(name='glProgramUniformMatrix3x4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 12, value')
)

Function(name='glProgramUniformMatrix3x4dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3x4dv')

Function(name='glProgramUniformMatrix3x4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 12, value')
)

Function(name='glProgramUniformMatrix3x4fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix3x4fv')

Function(name='glProgramUniformMatrix4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 16, value')
)

Function(name='glProgramUniformMatrix4dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4dv')

Function(name='glProgramUniformMatrix4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 16, value')
)

Function(name='glProgramUniformMatrix4fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4fv')

Function(name='glProgramUniformMatrix4x2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 8, value')
)

Function(name='glProgramUniformMatrix4x2dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4x2dv')

Function(name='glProgramUniformMatrix4x2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 8, value')
)

Function(name='glProgramUniformMatrix4x2fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4x2fv')

Function(name='glProgramUniformMatrix4x3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 12, value')
)

Function(name='glProgramUniformMatrix4x3dvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4x3dv')

Function(name='glProgramUniformMatrix4x3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='transpose', type='GLboolean'),
arg5=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 12, value')
)

Function(name='glProgramUniformMatrix4x3fvEXT', enabled=True, type=Param, inheritFrom='glProgramUniformMatrix4x3fv')

Function(name='glProgramUniformui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='value', type='GLuint64EXT')
)

Function(name='glProgramUniformui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='program, location'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count, value')
)

Function(name='glProgramVertexLimitNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='limit', type='GLint')
)

Function(name='glProvokingVertex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glProvokingVertexEXT', enabled=True, type=Param, inheritFrom='glProvokingVertex')

Function(name='glPushAttrib', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glPushClientAttrib', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glPushClientAttribDefaultEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glPushDebugGroup', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='source', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='length', type='GLsizei'),
arg4=ArgDef(name='message', type='const GLchar*', wrapParams='message, \'\\0\', 1')
)

Function(name='glPushDebugGroupKHR', enabled=False, type=None, inheritFrom='glPushDebugGroup')

Function(name='glPushGroupMarkerEXT', enabled=True, type=Param, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='length', type='GLsizei'),
arg2=ArgDef(name='marker', type='const GLchar*', wrapParams='length, marker')
)

Function(name='glPushMatrix', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glPushName', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='name', type='GLuint')
)

Function(name='glQueryCounter', enabled=True, type=Query, runCond='ConditionQueries()',
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint', wrapType='CGLQuery'),
arg2=ArgDef(name='target', type='GLenum')
)

Function(name='glQueryCounterEXT', enabled=True, type=Query, inheritFrom='glQueryCounter')

Function(name='glQueryCounterINTEL', enabled=True, type=Query, inheritFrom='glQueryCounter')

Function(name='glQueryMatrixxOES', enabled=True, type=Param,
retV=RetDef(type='GLbitfield'),
arg1=ArgDef(name='mantissa', type='GLfixed*', wrapParams='16, mantissa'),
arg2=ArgDef(name='exponent', type='GLint*', wrapParams='16, exponent')
)

Function(name='glQueryObjectParameteruiAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='id', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLuint')
)

Function(name='glQueryResourceNV', enabled=False, type=None,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='queryType', type='GLenum'),
arg2=ArgDef(name='tagId', type='GLint'),
arg3=ArgDef(name='bufSize', type='GLuint'),
arg4=ArgDef(name='buffer', type='GLint*')
)

Function(name='glQueryResourceTagNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='tagId', type='GLint'),
arg2=ArgDef(name='tagString', type='const GLchar*')
)

Function(name='glRasterPos2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble')
)

Function(name='glRasterPos2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glRasterPos2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat')
)

Function(name='glRasterPos2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glRasterPos2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint')
)

Function(name='glRasterPos2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glRasterPos2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort')
)

Function(name='glRasterPos2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glRasterPos2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed')
)

Function(name='glRasterPos2xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glRasterPos3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble')
)

Function(name='glRasterPos3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glRasterPos3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat')
)

Function(name='glRasterPos3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glRasterPos3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint')
)

Function(name='glRasterPos3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glRasterPos3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort')
)

Function(name='glRasterPos3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glRasterPos3xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed')
)

Function(name='glRasterPos3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glRasterPos4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble'),
arg4=ArgDef(name='w', type='GLdouble')
)

Function(name='glRasterPos4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glRasterPos4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat'),
arg4=ArgDef(name='w', type='GLfloat')
)

Function(name='glRasterPos4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glRasterPos4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint'),
arg4=ArgDef(name='w', type='GLint')
)

Function(name='glRasterPos4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glRasterPos4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort'),
arg4=ArgDef(name='w', type='GLshort')
)

Function(name='glRasterPos4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glRasterPos4xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed'),
arg4=ArgDef(name='w', type='GLfixed')
)

Function(name='glRasterPos4xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glRasterSamplesEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='samples', type='GLuint'),
arg2=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glReadBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='src', type='GLenum')
)

Function(name='glReadBufferIndexedEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='src', type='GLenum'),
arg2=ArgDef(name='index', type='GLint')
)

Function(name='glReadBufferNV', enabled=True, type=Param, inheritFrom='glReadBuffer')

Function(name='glReadInstrumentsSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='marker', type='GLint')
)

Function(name='glReadPixels', enabled=True, type=Param, runWrap=True, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='pixels', type='void*', wrapType='CGLvoid_ptr')
)

Function(name='glReadnPixels', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='bufSize', type='GLsizei'),
arg8=ArgDef(name='data', type='void*')
)

Function(name='glReadnPixelsARB', enabled=False, type=None, inheritFrom='glReadnPixels')

Function(name='glReadnPixelsEXT', enabled=False, type=None, inheritFrom='glReadnPixels')

Function(name='glReadnPixelsKHR', enabled=False, type=None, inheritFrom='glReadnPixels')

Function(name='glRectd', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x1', type='GLdouble'),
arg2=ArgDef(name='y1', type='GLdouble'),
arg3=ArgDef(name='x2', type='GLdouble'),
arg4=ArgDef(name='y2', type='GLdouble')
)

Function(name='glRectdv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v1', type='const GLdouble*', wrapParams='2, v1'),
arg2=ArgDef(name='v2', type='const GLdouble*', wrapParams='2, v2')
)

Function(name='glRectf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x1', type='GLfloat'),
arg2=ArgDef(name='y1', type='GLfloat'),
arg3=ArgDef(name='x2', type='GLfloat'),
arg4=ArgDef(name='y2', type='GLfloat')
)

Function(name='glRectfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v1', type='const GLfloat*', wrapParams='2, v1'),
arg2=ArgDef(name='v2', type='const GLfloat*', wrapParams='2, v2')
)

Function(name='glRecti', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x1', type='GLint'),
arg2=ArgDef(name='y1', type='GLint'),
arg3=ArgDef(name='x2', type='GLint'),
arg4=ArgDef(name='y2', type='GLint')
)

Function(name='glRectiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v1', type='const GLint*', wrapParams='2, v1'),
arg2=ArgDef(name='v2', type='const GLint*', wrapParams='2, v2')
)

Function(name='glRects', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x1', type='GLshort'),
arg2=ArgDef(name='y1', type='GLshort'),
arg3=ArgDef(name='x2', type='GLshort'),
arg4=ArgDef(name='y2', type='GLshort')
)

Function(name='glRectsv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v1', type='const GLshort*', wrapParams='2, v1'),
arg2=ArgDef(name='v2', type='const GLshort*', wrapParams='2, v2')
)

Function(name='glRectxOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x1', type='GLfixed'),
arg2=ArgDef(name='y1', type='GLfixed'),
arg3=ArgDef(name='x2', type='GLfixed'),
arg4=ArgDef(name='y2', type='GLfixed')
)

Function(name='glRectxvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='v1', type='const GLfixed*'),
arg2=ArgDef(name='v2', type='const GLfixed*')
)

Function(name='glReferencePlaneSGIX', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='equation', type='const GLdouble*')
)

Function(name='glReleaseKeyedMutexWin32EXT', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='memory', type='GLuint'),
arg2=ArgDef(name='key', type='GLuint64')
)

Function(name='glReleaseShaderCompiler', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glRenderGpuMaskNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glRenderMode', enabled=True, type=Param,
retV=RetDef(type='GLint'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glRenderbufferStorage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei')
)

Function(name='glRenderbufferStorageEXT', enabled=True, type=Param, recWrap=True, inheritFrom='glRenderbufferStorage')

Function(name='glRenderbufferStorageMultisample', enabled=True, type=Param, runWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei')
)

Function(name='glRenderbufferStorageMultisampleANGLE', enabled=True, type=Param, runWrap=True, inheritFrom='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleAPPLE', enabled=False, type=Param, inheritFrom='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleCoverageNV', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='coverageSamples', type='GLsizei'),
arg3=ArgDef(name='colorSamples', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei')
)

Function(name='glRenderbufferStorageMultisampleEXT', enabled=True, type=Param, runWrap=True, stateTrack=True, recWrap=True, inheritFrom='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleIMG', enabled=True, type=Param, runWrap=True, inheritFrom='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleNV', enabled=False, type=None, inheritFrom='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageOES', enabled=True, type=Param, inheritFrom='glRenderbufferStorage')

Function(name='glReplacementCodePointerSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void**')
)

Function(name='glReplacementCodeubSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='GLubyte')
)

Function(name='glReplacementCodeubvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='const GLubyte*')
)

Function(name='glReplacementCodeuiColor3fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='r', type='GLfloat'),
arg3=ArgDef(name='g', type='GLfloat'),
arg4=ArgDef(name='b', type='GLfloat'),
arg5=ArgDef(name='x', type='GLfloat'),
arg6=ArgDef(name='y', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiColor3fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='c', type='const GLfloat*'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiColor4fNormal3fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='r', type='GLfloat'),
arg3=ArgDef(name='g', type='GLfloat'),
arg4=ArgDef(name='b', type='GLfloat'),
arg5=ArgDef(name='a', type='GLfloat'),
arg6=ArgDef(name='nx', type='GLfloat'),
arg7=ArgDef(name='ny', type='GLfloat'),
arg8=ArgDef(name='nz', type='GLfloat'),
arg9=ArgDef(name='x', type='GLfloat'),
arg10=ArgDef(name='y', type='GLfloat'),
arg11=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiColor4fNormal3fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='c', type='const GLfloat*'),
arg3=ArgDef(name='n', type='const GLfloat*'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiColor4ubVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='r', type='GLubyte'),
arg3=ArgDef(name='g', type='GLubyte'),
arg4=ArgDef(name='b', type='GLubyte'),
arg5=ArgDef(name='a', type='GLubyte'),
arg6=ArgDef(name='x', type='GLfloat'),
arg7=ArgDef(name='y', type='GLfloat'),
arg8=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiColor4ubVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='c', type='const GLubyte*'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiNormal3fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='nx', type='GLfloat'),
arg3=ArgDef(name='ny', type='GLfloat'),
arg4=ArgDef(name='nz', type='GLfloat'),
arg5=ArgDef(name='x', type='GLfloat'),
arg6=ArgDef(name='y', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiNormal3fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='n', type='const GLfloat*'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='GLuint')
)

Function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat'),
arg4=ArgDef(name='r', type='GLfloat'),
arg5=ArgDef(name='g', type='GLfloat'),
arg6=ArgDef(name='b', type='GLfloat'),
arg7=ArgDef(name='a', type='GLfloat'),
arg8=ArgDef(name='nx', type='GLfloat'),
arg9=ArgDef(name='ny', type='GLfloat'),
arg10=ArgDef(name='nz', type='GLfloat'),
arg11=ArgDef(name='x', type='GLfloat'),
arg12=ArgDef(name='y', type='GLfloat'),
arg13=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='tc', type='const GLfloat*'),
arg3=ArgDef(name='c', type='const GLfloat*'),
arg4=ArgDef(name='n', type='const GLfloat*'),
arg5=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat'),
arg4=ArgDef(name='nx', type='GLfloat'),
arg5=ArgDef(name='ny', type='GLfloat'),
arg6=ArgDef(name='nz', type='GLfloat'),
arg7=ArgDef(name='x', type='GLfloat'),
arg8=ArgDef(name='y', type='GLfloat'),
arg9=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='tc', type='const GLfloat*'),
arg3=ArgDef(name='n', type='const GLfloat*'),
arg4=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiTexCoord2fVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='s', type='GLfloat'),
arg3=ArgDef(name='t', type='GLfloat'),
arg4=ArgDef(name='x', type='GLfloat'),
arg5=ArgDef(name='y', type='GLfloat'),
arg6=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiTexCoord2fVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='tc', type='const GLfloat*'),
arg3=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuiVertex3fSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glReplacementCodeuiVertex3fvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='rc', type='const GLuint*'),
arg2=ArgDef(name='v', type='const GLfloat*')
)

Function(name='glReplacementCodeuivSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='const GLuint*')
)

Function(name='glReplacementCodeusSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='GLushort')
)

Function(name='glReplacementCodeusvSUN', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='code', type='const GLushort*')
)

Function(name='glRequestResidentProgramsNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='n', type='GLsizei'),
arg2=ArgDef(name='programs', type='const GLuint*', wrapParams='n, programs')
)

Function(name='glResetHistogram', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glResetHistogramEXT', enabled=True, type=Param, inheritFrom='glResetHistogram')

Function(name='glResetMinmax', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glResetMinmaxEXT', enabled=True, type=Param, inheritFrom='glResetMinmax')

Function(name='glResizeBuffersMESA', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glResolveDepthValuesNV', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glResolveMultisampleFramebufferAPPLE', enabled=False, type=Param,
retV=RetDef(type='void')
)

Function(name='glResumeTransformFeedback', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glResumeTransformFeedbackNV', enabled=True, type=Param, inheritFrom='glResumeTransformFeedback')

Function(name='glRotated', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='angle', type='GLdouble'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glRotatef', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='angle', type='GLfloat'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glRotatex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='angle', type='GLfixed'),
arg2=ArgDef(name='x', type='GLfixed'),
arg3=ArgDef(name='y', type='GLfixed'),
arg4=ArgDef(name='z', type='GLfixed')
)

Function(name='glRotatexOES', enabled=True, type=Param, inheritFrom='glRotatex')

Function(name='glSampleCoverage', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLfloat'),
arg2=ArgDef(name='invert', type='GLboolean')
)

Function(name='glSamplePass', enabled=False, type=Param, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glSampleCoverageARB', enabled=True, type=Param, inheritFrom='glSampleCoverage')

Function(name='glSampleCoverageOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLfixed'),
arg2=ArgDef(name='invert', type='GLboolean')
)

Function(name='glSampleCoveragex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLclampx'),
arg2=ArgDef(name='invert', type='GLboolean')
)

Function(name='glSampleCoveragexOES', enabled=True, type=Param, inheritFrom='glSampleCoveragex')

Function(name='glSampleMapATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='dst', type='GLuint'),
arg2=ArgDef(name='interp', type='GLuint'),
arg3=ArgDef(name='swizzle', type='GLenum')
)

Function(name='glSampleMaskEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLclampf'),
arg2=ArgDef(name='invert', type='GLboolean')
)

Function(name='glSampleMaskIndexedNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glSampleMaskSGIS', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='value', type='GLclampf'),
arg2=ArgDef(name='invert', type='GLboolean')
)

Function(name='glSampleMaski', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='maskNumber', type='GLuint'),
arg2=ArgDef(name='mask', type='GLbitfield')
)

Function(name='glSamplePatternEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pattern', type='GLenum')
)

Function(name='glSamplePatternSGIS', enabled=True, type=Param, inheritFrom='glSamplePatternEXT')

Function(name='glSamplerParameterIiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, param')
)

Function(name='glSamplerParameterIivEXT', enabled=False, type=None, inheritFrom='glSamplerParameterIiv')

Function(name='glSamplerParameterIivOES', enabled=False, type=None, inheritFrom='glSamplerParameterIiv')

Function(name='glSamplerParameterIuiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='pname, param')
)

Function(name='glSamplerParameterIuivEXT', enabled=False, type=None, inheritFrom='glSamplerParameterIuiv')

Function(name='glSamplerParameterIuivOES', enabled=False, type=None, inheritFrom='glSamplerParameterIuiv')

Function(name='glSamplerParameterf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glSamplerParameterfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, param')
)

Function(name='glSamplerParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glSamplerParameteriv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sampler', type='GLuint', wrapType='CGLSampler'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, param')
)

Function(name='glScaled', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble')
)

Function(name='glScalef', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat')
)

Function(name='glScalex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed')
)

Function(name='glScalexOES', enabled=True, type=Param, inheritFrom='glScalex')

Function(name='glScissor', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei')
)

Function(name='glScissorArrayv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLint*', wrapParams='4 * count, v')
)

Function(name='glScissorArrayvNV', enabled=False, type=None, inheritFrom='glScissorArrayv')

Function(name='glScissorArrayvOES', enabled=False, type=None, inheritFrom='glScissorArrayv')

Function(name='glScissorIndexed', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='left', type='GLint'),
arg3=ArgDef(name='bottom', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei')
)

Function(name='glScissorIndexedNV', enabled=False, type=None, inheritFrom='glScissorIndexed')

Function(name='glScissorIndexedOES', enabled=False, type=None, inheritFrom='glScissorIndexed')

Function(name='glScissorIndexedv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glScissorIndexedvNV', enabled=False, type=None, inheritFrom='glScissorIndexedv')

Function(name='glScissorIndexedvOES', enabled=False, type=None, inheritFrom='glScissorIndexedv')

Function(name='glSecondaryColor3b', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLbyte'),
arg2=ArgDef(name='green', type='GLbyte'),
arg3=ArgDef(name='blue', type='GLbyte')
)

Function(name='glSecondaryColor3bEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3b')

Function(name='glSecondaryColor3bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*', wrapParams='3, v')
)

Function(name='glSecondaryColor3bvEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3bv')

Function(name='glSecondaryColor3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLdouble'),
arg2=ArgDef(name='green', type='GLdouble'),
arg3=ArgDef(name='blue', type='GLdouble')
)

Function(name='glSecondaryColor3dEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3d')

Function(name='glSecondaryColor3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glSecondaryColor3dvEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3dv')

Function(name='glSecondaryColor3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLfloat'),
arg2=ArgDef(name='green', type='GLfloat'),
arg3=ArgDef(name='blue', type='GLfloat')
)

Function(name='glSecondaryColor3fEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3f')

Function(name='glSecondaryColor3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glSecondaryColor3fvEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3fv')

Function(name='glSecondaryColor3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLhalfNV'),
arg2=ArgDef(name='green', type='GLhalfNV'),
arg3=ArgDef(name='blue', type='GLhalfNV')
)

Function(name='glSecondaryColor3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glSecondaryColor3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLint'),
arg2=ArgDef(name='green', type='GLint'),
arg3=ArgDef(name='blue', type='GLint')
)

Function(name='glSecondaryColor3iEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3i')

Function(name='glSecondaryColor3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glSecondaryColor3ivEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3iv')

Function(name='glSecondaryColor3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLshort'),
arg2=ArgDef(name='green', type='GLshort'),
arg3=ArgDef(name='blue', type='GLshort')
)

Function(name='glSecondaryColor3sEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3s')

Function(name='glSecondaryColor3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glSecondaryColor3svEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3sv')

Function(name='glSecondaryColor3ub', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLubyte'),
arg2=ArgDef(name='green', type='GLubyte'),
arg3=ArgDef(name='blue', type='GLubyte')
)

Function(name='glSecondaryColor3ubEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3ub')

Function(name='glSecondaryColor3ubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLubyte*', wrapParams='3, v')
)

Function(name='glSecondaryColor3ubvEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3ubv')

Function(name='glSecondaryColor3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLuint'),
arg2=ArgDef(name='green', type='GLuint'),
arg3=ArgDef(name='blue', type='GLuint')
)

Function(name='glSecondaryColor3uiEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3ui')

Function(name='glSecondaryColor3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLuint*', wrapParams='3, v')
)

Function(name='glSecondaryColor3uivEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3uiv')

Function(name='glSecondaryColor3us', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLushort'),
arg2=ArgDef(name='green', type='GLushort'),
arg3=ArgDef(name='blue', type='GLushort')
)

Function(name='glSecondaryColor3usEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3us')

Function(name='glSecondaryColor3usv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLushort*', wrapParams='3, v')
)

Function(name='glSecondaryColor3usvEXT', enabled=True, type=Param, inheritFrom='glSecondaryColor3usv')

Function(name='glSecondaryColorFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei')
)

Function(name='glSecondaryColorP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='GLuint')
)

Function(name='glSecondaryColorP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='color', type='const GLuint*', wrapParams='1, color')
)

Function(name='glSecondaryColorPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glSecondaryColorPointerEXT', enabled=True, type=Param, inheritFrom='glSecondaryColorPointer')

Function(name='glSecondaryColorPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLint'),
arg4=ArgDef(name='pointer', type='const void**'),
arg5=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glSelectBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLsizei'),
arg2=ArgDef(name='buffer', type='GLuint*', wrapParams='size, buffer')
)

Function(name='glSelectPerfMonitorCountersAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='monitor', type='GLuint'),
arg2=ArgDef(name='enable', type='GLboolean'),
arg3=ArgDef(name='group', type='GLuint'),
arg4=ArgDef(name='numCounters', type='GLint'),
arg5=ArgDef(name='counterList', type='GLuint*', wrapParams='numCounters, counterList')
)

Function(name='glSemaphoreParameterui64vEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLuint64*')
)

Function(name='glSeparableFilter2D', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='row', type='const void*'),
arg8=ArgDef(name='column', type='const void*')
)

Function(name='glSeparableFilter2DEXT', enabled=False, type=None, inheritFrom='glSeparableFilter2D')

Function(name='glSetFenceAPPLE', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glSetFenceNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='fence', type='GLuint'),
arg2=ArgDef(name='condition', type='GLenum')
)

Function(name='glSetFragmentShaderConstantATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='dst', type='GLuint'),
arg2=ArgDef(name='value', type='const GLfloat*', wrapParams='4, value')
)

Function(name='glSetInvariantEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='addr', type='const void*')
)

Function(name='glSetLocalConstantEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='addr', type='const void*')
)

Function(name='glSetMultisamplefvAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='val', type='const GLfloat*', wrapParams='1, val')
)

Function(name='glShadeModel', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glShaderBinary', enabled=True, type=Param, interceptorExecOverride=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLsizei'),
arg2=ArgDef(name='shaders', type='const GLuint*', wrapParams='count, shaders'),
arg3=ArgDef(name='binaryformat', type='GLenum'),
arg4=ArgDef(name='binary', type='const void*', wrapType='CBinaryResource', wrapParams='RESOURCE_BUFFER, binary, length'),
arg5=ArgDef(name='length', type='GLsizei')
)

Function(name='glShaderOp1EXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='res', type='GLuint'),
arg3=ArgDef(name='arg1', type='GLuint')
)

Function(name='glShaderOp2EXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='res', type='GLuint'),
arg3=ArgDef(name='arg1', type='GLuint'),
arg4=ArgDef(name='arg2', type='GLuint')
)

Function(name='glShaderOp3EXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='op', type='GLenum'),
arg2=ArgDef(name='res', type='GLuint'),
arg3=ArgDef(name='arg1', type='GLuint'),
arg4=ArgDef(name='arg2', type='GLuint'),
arg5=ArgDef(name='arg3', type='GLuint')
)

Function(name='glShaderSource', enabled=True, type=Resource, stateTrack=True, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='count', type='GLsizei', wrapParams='1'),
arg3=ArgDef(name='string', type='const GLchar*const*', wrapType='CShaderSource', wrapParams='shader,count,string,length,_string.SHADER_SOURCE'),
arg4=ArgDef(name='length', type='const GLint*', wrapType='CGLintptrZero', wrapParams=' ')
)

Function(name='glShaderSourceARB', enabled=True, type=Resource, inheritFrom='glShaderSource', runWrap=True,
arg1=ArgDef(name='shaderObj', type='GLhandleARB', wrapType='CGLProgram'),
arg3=ArgDef(name='string', type='const GLcharARB*const*', wrapType='CShaderSource', wrapParams='shaderObj,count,string,length,_string.SHADER_SOURCE'),
)

Function(name='glShaderStorageBlockBinding', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='storageBlockIndex', type='GLuint'),
arg3=ArgDef(name='storageBlockBinding', type='GLuint')
)

Function(name='glShaderStorageBlockBinding', enabled=True, type=Param, version=1,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='storageBlockIndex', type='GLuint',wrapType='CGLStorageBlockIndex', wrapParams='program, storageBlockIndex'),
arg3=ArgDef(name='storageBlockBinding', type='GLuint')
)

Function(name='glSharpenTexFuncSGIS', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='points', type='const GLfloat*', wrapParams='n * 2, points')
)

Function(name='glSignalSemaphoreEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='numBufferBarriers', type='GLuint'),
arg3=ArgDef(name='buffers', type='const GLuint*'),
arg4=ArgDef(name='numTextureBarriers', type='GLuint'),
arg5=ArgDef(name='textures', type='const GLuint*'),
arg6=ArgDef(name='dstLayouts', type='const GLenum*')
)

Function(name='glSignalVkFenceNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vkFence', type='GLuint64')
)

Function(name='glSignalVkSemaphoreNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vkSemaphore', type='GLuint64')
)

Function(name='glSpecializeShader', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='shader', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='pEntryPoint', type='const GLchar*'),
arg3=ArgDef(name='numSpecializationConstants', type='GLuint'),
arg4=ArgDef(name='pConstantIndex', type='const GLuint*'),
arg5=ArgDef(name='pConstantValue', type='const GLuint*')
)

Function(name='glSpecializeShaderARB', enabled=False, type=None, inheritFrom='glSpecializeShader')

Function(name='glSpriteParameterfSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glSpriteParameterfvSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glSpriteParameteriSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glSpriteParameterivSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glStartInstrumentsSGIX', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glStartTilingQCOM', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLuint'),
arg2=ArgDef(name='y', type='GLuint'),
arg3=ArgDef(name='width', type='GLuint'),
arg4=ArgDef(name='height', type='GLuint'),
arg5=ArgDef(name='preserveMask', type='GLbitfield')
)

Function(name='glStateCaptureNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='state', type='GLuint'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glStencilClearTagEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stencilTagBits', type='GLsizei'),
arg2=ArgDef(name='stencilClearTag', type='GLuint')
)

Function(name='glStencilFillPathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='fillMode', type='GLenum'),
arg6=ArgDef(name='mask', type='GLuint'),
arg7=ArgDef(name='transformType', type='GLenum'),
arg8=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glStencilFillPathNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='fillMode', type='GLenum'),
arg3=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilFunc', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='func', type='GLenum'),
arg2=ArgDef(name='ref', type='GLint'),
arg3=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilFuncSeparate', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='func', type='GLenum'),
arg3=ArgDef(name='ref', type='GLint'),
arg4=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilFuncSeparateATI', enabled=True, type=Param, inheritFrom='glStencilFuncSeparate')

Function(name='glStencilMask', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilMaskSeparate', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilOp', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='fail', type='GLenum'),
arg2=ArgDef(name='zfail', type='GLenum'),
arg3=ArgDef(name='zpass', type='GLenum')
)

Function(name='glStencilOpSeparate', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='sfail', type='GLenum'),
arg3=ArgDef(name='dpfail', type='GLenum'),
arg4=ArgDef(name='dppass', type='GLenum')
)

Function(name='glStencilOpSeparateATI', enabled=True, type=Param, inheritFrom='glStencilOpSeparate')

Function(name='glStencilOpValueAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='value', type='GLuint')
)

Function(name='glStencilStrokePathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='reference', type='GLint'),
arg6=ArgDef(name='mask', type='GLuint'),
arg7=ArgDef(name='transformType', type='GLenum'),
arg8=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glStencilStrokePathNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='reference', type='GLint'),
arg3=ArgDef(name='mask', type='GLuint')
)

Function(name='glStencilThenCoverFillPathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='fillMode', type='GLenum'),
arg6=ArgDef(name='mask', type='GLuint'),
arg7=ArgDef(name='coverMode', type='GLenum'),
arg8=ArgDef(name='transformType', type='GLenum'),
arg9=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glStencilThenCoverFillPathNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='fillMode', type='GLenum'),
arg3=ArgDef(name='mask', type='GLuint'),
arg4=ArgDef(name='coverMode', type='GLenum')
)

Function(name='glStencilThenCoverStrokePathInstancedNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numPaths', type='GLsizei'),
arg2=ArgDef(name='pathNameType', type='GLenum'),
arg3=ArgDef(name='paths', type='const void*'),
arg4=ArgDef(name='pathBase', type='GLuint'),
arg5=ArgDef(name='reference', type='GLint'),
arg6=ArgDef(name='mask', type='GLuint'),
arg7=ArgDef(name='coverMode', type='GLenum'),
arg8=ArgDef(name='transformType', type='GLenum'),
arg9=ArgDef(name='transformValues', type='const GLfloat*')
)

Function(name='glStencilThenCoverStrokePathNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='path', type='GLuint'),
arg2=ArgDef(name='reference', type='GLint'),
arg3=ArgDef(name='mask', type='GLuint'),
arg4=ArgDef(name='coverMode', type='GLenum')
)

Function(name='glStopInstrumentsSGIX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='marker', type='GLint')
)

Function(name='glStringMarkerGREMEDY', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='len', type='GLsizei'),
arg2=ArgDef(name='string', type='const void*')
)

Function(name='glSubpixelPrecisionBiasNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xbits', type='GLuint'),
arg2=ArgDef(name='ybits', type='GLuint')
)

Function(name='glSwapAPPLE', enabled=False, type=Exec,
retV=RetDef(type='void')
)

Function(name='glSwizzleEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='res', type='GLuint'),
arg2=ArgDef(name='in', type='GLuint'),
arg3=ArgDef(name='outX', type='GLenum'),
arg4=ArgDef(name='outY', type='GLenum'),
arg5=ArgDef(name='outZ', type='GLenum'),
arg6=ArgDef(name='outW', type='GLenum')
)

Function(name='glSyncTextureINTEL', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture')
)

Function(name='glTagSampleBufferSGIX', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glTangent3bEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tx', type='GLbyte'),
arg2=ArgDef(name='ty', type='GLbyte'),
arg3=ArgDef(name='tz', type='GLbyte')
)

Function(name='glTangent3bvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLbyte*', wrapParams='3, v')
)

Function(name='glTangent3dEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tx', type='GLdouble'),
arg2=ArgDef(name='ty', type='GLdouble'),
arg3=ArgDef(name='tz', type='GLdouble')
)

Function(name='glTangent3dvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glTangent3fEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tx', type='GLfloat'),
arg2=ArgDef(name='ty', type='GLfloat'),
arg3=ArgDef(name='tz', type='GLfloat')
)

Function(name='glTangent3fvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTangent3iEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tx', type='GLint'),
arg2=ArgDef(name='ty', type='GLint'),
arg3=ArgDef(name='tz', type='GLint')
)

Function(name='glTangent3ivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glTangent3sEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tx', type='GLshort'),
arg2=ArgDef(name='ty', type='GLshort'),
arg3=ArgDef(name='tz', type='GLshort')
)

Function(name='glTangent3svEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glTangentPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='stride', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glTbufferMask3DFX', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mask', type='GLuint')
)

Function(name='glTessellationFactorAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='factor', type='GLfloat')
)

Function(name='glTessellationModeAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glTestFenceAPPLE', enabled=False, type=Param,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glTestFenceNV', enabled=True, type=Param,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='fence', type='GLuint')
)

Function(name='glTestObjectAPPLE', enabled=False, type=Param,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='object', type='GLenum'),
arg2=ArgDef(name='name', type='GLuint')
)

Function(name='glTexBuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glTexBufferARB', enabled=True, type=Param, inheritFrom='glTexBuffer')

Function(name='glTexBufferEXT', enabled=True, type=Param, inheritFrom='glTexBuffer')

Function(name='glTexBufferOES', enabled=False, type=None, inheritFrom='glTexBuffer')

Function(name='glTexBufferRange', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glTexBufferRangeEXT', enabled=False, type=None, inheritFrom='glTexBufferRange')

Function(name='glTexBufferRangeOES', enabled=False, type=None, inheritFrom='glTexBufferRange')

Function(name='glTexBumpParameterfvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, param')
)

Function(name='glTexBumpParameterivATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, param')
)

Function(name='glTexCoord1bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLbyte')
)

Function(name='glTexCoord1bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glTexCoord1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLdouble')
)

Function(name='glTexCoord1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='1, v')
)

Function(name='glTexCoord1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat')
)

Function(name='glTexCoord1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='1, v')
)

Function(name='glTexCoord1hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLhalfNV')
)

Function(name='glTexCoord1hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='1, v')
)

Function(name='glTexCoord1i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLint')
)

Function(name='glTexCoord1iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='1, v')
)

Function(name='glTexCoord1s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLshort')
)

Function(name='glTexCoord1sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='1, v')
)

Function(name='glTexCoord1xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfixed')
)

Function(name='glTexCoord1xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glTexCoord2bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLbyte'),
arg2=ArgDef(name='t', type='GLbyte')
)

Function(name='glTexCoord2bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glTexCoord2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLdouble'),
arg2=ArgDef(name='t', type='GLdouble')
)

Function(name='glTexCoord2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glTexCoord2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat')
)

Function(name='glTexCoord2fColor3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='r', type='GLfloat'),
arg4=ArgDef(name='g', type='GLfloat'),
arg5=ArgDef(name='b', type='GLfloat'),
arg6=ArgDef(name='x', type='GLfloat'),
arg7=ArgDef(name='y', type='GLfloat'),
arg8=ArgDef(name='z', type='GLfloat')
)

Function(name='glTexCoord2fColor3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='3, tc'),
arg2=ArgDef(name='c', type='const GLfloat*', wrapParams='3, c'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord2fColor4fNormal3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='r', type='GLfloat'),
arg4=ArgDef(name='g', type='GLfloat'),
arg5=ArgDef(name='b', type='GLfloat'),
arg6=ArgDef(name='a', type='GLfloat'),
arg7=ArgDef(name='nx', type='GLfloat'),
arg8=ArgDef(name='ny', type='GLfloat'),
arg9=ArgDef(name='nz', type='GLfloat'),
arg10=ArgDef(name='x', type='GLfloat'),
arg11=ArgDef(name='y', type='GLfloat'),
arg12=ArgDef(name='z', type='GLfloat')
)

Function(name='glTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='3, tc'),
arg2=ArgDef(name='c', type='const GLfloat*', wrapParams='3, c'),
arg3=ArgDef(name='n', type='const GLfloat*', wrapParams='3, n'),
arg4=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord2fColor4ubVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='r', type='GLubyte'),
arg4=ArgDef(name='g', type='GLubyte'),
arg5=ArgDef(name='b', type='GLubyte'),
arg6=ArgDef(name='a', type='GLubyte'),
arg7=ArgDef(name='x', type='GLfloat'),
arg8=ArgDef(name='y', type='GLfloat'),
arg9=ArgDef(name='z', type='GLfloat')
)

Function(name='glTexCoord2fColor4ubVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='3, tc'),
arg2=ArgDef(name='c', type='const GLubyte*', wrapParams='3, c'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord2fNormal3fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='nx', type='GLfloat'),
arg4=ArgDef(name='ny', type='GLfloat'),
arg5=ArgDef(name='nz', type='GLfloat'),
arg6=ArgDef(name='x', type='GLfloat'),
arg7=ArgDef(name='y', type='GLfloat'),
arg8=ArgDef(name='z', type='GLfloat')
)

Function(name='glTexCoord2fNormal3fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='3, tc'),
arg2=ArgDef(name='n', type='const GLfloat*', wrapParams='3, n'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord2fVertex3fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='x', type='GLfloat'),
arg4=ArgDef(name='y', type='GLfloat'),
arg5=ArgDef(name='z', type='GLfloat')
)

Function(name='glTexCoord2fVertex3fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='3, tc'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glTexCoord2hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLhalfNV'),
arg2=ArgDef(name='t', type='GLhalfNV')
)

Function(name='glTexCoord2hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='2, v')
)

Function(name='glTexCoord2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLint'),
arg2=ArgDef(name='t', type='GLint')
)

Function(name='glTexCoord2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glTexCoord2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLshort'),
arg2=ArgDef(name='t', type='GLshort')
)

Function(name='glTexCoord2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glTexCoord2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfixed'),
arg2=ArgDef(name='t', type='GLfixed')
)

Function(name='glTexCoord2xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glTexCoord3bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLbyte'),
arg2=ArgDef(name='t', type='GLbyte'),
arg3=ArgDef(name='r', type='GLbyte')
)

Function(name='glTexCoord3bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glTexCoord3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLdouble'),
arg2=ArgDef(name='t', type='GLdouble'),
arg3=ArgDef(name='r', type='GLdouble')
)

Function(name='glTexCoord3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glTexCoord3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='r', type='GLfloat')
)

Function(name='glTexCoord3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glTexCoord3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLhalfNV'),
arg2=ArgDef(name='t', type='GLhalfNV'),
arg3=ArgDef(name='r', type='GLhalfNV')
)

Function(name='glTexCoord3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glTexCoord3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLint'),
arg2=ArgDef(name='t', type='GLint'),
arg3=ArgDef(name='r', type='GLint')
)

Function(name='glTexCoord3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glTexCoord3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLshort'),
arg2=ArgDef(name='t', type='GLshort'),
arg3=ArgDef(name='r', type='GLshort')
)

Function(name='glTexCoord3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glTexCoord3xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfixed'),
arg2=ArgDef(name='t', type='GLfixed'),
arg3=ArgDef(name='r', type='GLfixed')
)

Function(name='glTexCoord3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glTexCoord4bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLbyte'),
arg2=ArgDef(name='t', type='GLbyte'),
arg3=ArgDef(name='r', type='GLbyte'),
arg4=ArgDef(name='q', type='GLbyte')
)

Function(name='glTexCoord4bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glTexCoord4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLdouble'),
arg2=ArgDef(name='t', type='GLdouble'),
arg3=ArgDef(name='r', type='GLdouble'),
arg4=ArgDef(name='q', type='GLdouble')
)

Function(name='glTexCoord4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glTexCoord4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='r', type='GLfloat'),
arg4=ArgDef(name='q', type='GLfloat')
)

Function(name='glTexCoord4fColor4fNormal3fVertex4fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='p', type='GLfloat'),
arg4=ArgDef(name='q', type='GLfloat'),
arg5=ArgDef(name='r', type='GLfloat'),
arg6=ArgDef(name='g', type='GLfloat'),
arg7=ArgDef(name='b', type='GLfloat'),
arg8=ArgDef(name='a', type='GLfloat'),
arg9=ArgDef(name='nx', type='GLfloat'),
arg10=ArgDef(name='ny', type='GLfloat'),
arg11=ArgDef(name='nz', type='GLfloat'),
arg12=ArgDef(name='x', type='GLfloat'),
arg13=ArgDef(name='y', type='GLfloat'),
arg14=ArgDef(name='z', type='GLfloat'),
arg15=ArgDef(name='w', type='GLfloat')
)

Function(name='glTexCoord4fColor4fNormal3fVertex4fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='4, tc'),
arg2=ArgDef(name='c', type='const GLfloat*', wrapParams='4, c'),
arg3=ArgDef(name='n', type='const GLfloat*', wrapParams='4, n'),
arg4=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glTexCoord4fVertex4fSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfloat'),
arg2=ArgDef(name='t', type='GLfloat'),
arg3=ArgDef(name='p', type='GLfloat'),
arg4=ArgDef(name='q', type='GLfloat'),
arg5=ArgDef(name='x', type='GLfloat'),
arg6=ArgDef(name='y', type='GLfloat'),
arg7=ArgDef(name='z', type='GLfloat'),
arg8=ArgDef(name='w', type='GLfloat')
)

Function(name='glTexCoord4fVertex4fvSUN', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='tc', type='const GLfloat*', wrapParams='4, tc'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glTexCoord4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glTexCoord4hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLhalfNV'),
arg2=ArgDef(name='t', type='GLhalfNV'),
arg3=ArgDef(name='r', type='GLhalfNV'),
arg4=ArgDef(name='q', type='GLhalfNV')
)

Function(name='glTexCoord4hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='4, v')
)

Function(name='glTexCoord4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLint'),
arg2=ArgDef(name='t', type='GLint'),
arg3=ArgDef(name='r', type='GLint'),
arg4=ArgDef(name='q', type='GLint')
)

Function(name='glTexCoord4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glTexCoord4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLshort'),
arg2=ArgDef(name='t', type='GLshort'),
arg3=ArgDef(name='r', type='GLshort'),
arg4=ArgDef(name='q', type='GLshort')
)

Function(name='glTexCoord4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glTexCoord4xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='s', type='GLfixed'),
arg2=ArgDef(name='t', type='GLfixed'),
arg3=ArgDef(name='r', type='GLfixed'),
arg4=ArgDef(name='q', type='GLfixed')
)

Function(name='glTexCoord4xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glTexCoordFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei')
)

Function(name='glTexCoordP1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='GLuint')
)

Function(name='glTexCoordP1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLuint*', wrapParams='1, coords')
)

Function(name='glTexCoordP2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='GLuint')
)

Function(name='glTexCoordP2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLuint*', wrapParams='1, coords')
)

Function(name='glTexCoordP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='GLuint')
)

Function(name='glTexCoordP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLuint*', wrapParams='1, coords')
)

Function(name='glTexCoordP4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='GLuint')
)

Function(name='glTexCoordP4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLuint*', wrapParams='1, coords')
)

Function(name='glTexCoordPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glTexCoordPointerBounds', enabled=True, type=Param, runWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const GLvoid*', wrapType='CAttribPtr'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glTexCoordPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*')
)

Function(name='glTexCoordPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLint'),
arg4=ArgDef(name='pointer', type='const void**'),
arg5=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glTexCoordPointervINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='pointer', type='const void**')
)

Function(name='glTexEnvf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glTexEnvfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexEnvi', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glTexEnviv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexEnvx', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glTexEnvxOES', enabled=True, type=Param, inheritFrom='glTexEnvx')

Function(name='glTexEnvxv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexEnvxvOES', enabled=True, type=Param, inheritFrom='glTexEnvxv')

Function(name='glTexFilterFuncSGIS', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='filter', type='GLenum'),
arg3=ArgDef(name='n', type='GLsizei'),
arg4=ArgDef(name='weights', type='const GLfloat*', wrapParams='n, weights')
)

Function(name='glTexGend', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLdouble')
)

Function(name='glTexGendv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLdouble*', wrapType='CGLdouble::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexGenf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glTexGenfOES', enabled=True, type=Param, inheritFrom='glTexGenf')

Function(name='glTexGenfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexGenfvOES', enabled=True, type=Param, inheritFrom='glTexGenfv')

Function(name='glTexGeni', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glTexGeniOES', enabled=True, type=Param, inheritFrom='glTexGeni')

Function(name='glTexGeniv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexGenivOES', enabled=True, type=Param, inheritFrom='glTexGeniv')

Function(name='glTexGenxOES', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glTexGenxvOES', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='coord', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexImage1D', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)', recCond='ConditionTexImageES(_recorder, format, type)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='border', type='GLint'),
arg6=ArgDef(name='format', type='GLenum'),
arg7=ArgDef(name='type', type='GLenum'),
arg8=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, pixels')
)

Function(name='glTexImage2D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTexImageES(_recorder, format, type)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='type', type='GLenum'),
arg9=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, pixels')
)

Function(name='glTexImage2DMultisample', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTexImage2DMultisampleCoverageNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='coverageSamples', type='GLsizei'),
arg3=ArgDef(name='colorSamples', type='GLsizei'),
arg4=ArgDef(name='internalFormat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTexImage3D', enabled=True, type=Resource, stateTrack=True, recCond='ConditionTexImageES(_recorder, format, type)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='type', type='GLenum'),
arg10=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, depth, pixels')
)

Function(name='glTexImage3DEXT', enabled=True, type=Resource, inheritFrom='glTexImage3D', recCond='ConditionTexImageES(_recorder, format, type)')

Function(name='glTexImage3DMultisample', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTexImage3DMultisampleCoverageNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='coverageSamples', type='GLsizei'),
arg3=ArgDef(name='colorSamples', type='GLsizei'),
arg4=ArgDef(name='internalFormat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTexImage3DOES', enabled=True, type=Resource, inheritFrom='glTexImage3D')

Function(name='glTexImage4DSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='size4d', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='pixels', type='const void*')
)

Function(name='glTexPageCommitmentARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='commit', type='GLboolean')
)

Function(name='glTexPageCommitmentEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='commit', type='GLboolean')
)

Function(name='glTexParameterIiv', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexParameterIivEXT', enabled=True, type=Param, inheritFrom='glTexParameterIiv')

Function(name='glTexParameterIivOES', enabled=False, type=None, inheritFrom='glTexParameterIiv')

Function(name='glTexParameterIuiv', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexParameterIuivEXT', enabled=True, type=Param, inheritFrom='glTexParameterIuiv')

Function(name='glTexParameterIuivOES', enabled=False, type=None, inheritFrom='glTexParameterIuiv')

Function(name='glTexParameterf', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glTexParameterfv', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexParameteri', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glTexParameteriv', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexParameterx', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfixed')
)

Function(name='glTexParameterxOES', enabled=True, type=Param, inheritFrom='glTexParameterx')

Function(name='glTexParameterxv', enabled=True, type=Param, recCond='ConditionTexParamES(_recorder, pname)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLfixed*', wrapType='CGLfixed::CSParamArray', wrapParams='pname, params')
)

Function(name='glTexParameterxvOES', enabled=True, type=Param, inheritFrom='glTexParameterxv')

Function(name='glTexRenderbufferNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbuffer')
)

Function(name='glTexStorage1D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei')
)

Function(name='glTexStorage1DEXT', enabled=True, type=Param, inheritFrom='glTexStorage1D')

Function(name='glTexStorage2D', enabled=True, type=Param, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei')
)

Function(name='glTexStorage2DEXT', enabled=True, type=Param, inheritFrom='glTexStorage2D')

Function(name='glTexStorage2DMultisample', enabled=True, type=Param, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTexStorage3D', enabled=True, type=Param, stateTrack=True, recCond='ConditionTextureES(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei')
)

Function(name='glTexStorage3DEXT', enabled=True, type=Param, inheritFrom='glTexStorage3D')

Function(name='glTexStorage3DMultisample', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTexStorage3DMultisampleOES', enabled=False, type=None, inheritFrom='glTexStorage3DMultisample')

Function(name='glTexStorageMem1DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='memory', type='GLuint'),
arg6=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTexStorageMem2DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='memory', type='GLuint'),
arg7=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTexStorageMem2DMultisampleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='fixedSampleLocations', type='GLboolean'),
arg7=ArgDef(name='memory', type='GLuint'),
arg8=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTexStorageMem3DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='memory', type='GLuint'),
arg8=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTexStorageMem3DMultisampleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='fixedSampleLocations', type='GLboolean'),
arg8=ArgDef(name='memory', type='GLuint'),
arg9=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTexStorageSparseAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='internalFormat', type='GLenum'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei'),
arg5=ArgDef(name='depth', type='GLsizei'),
arg6=ArgDef(name='layers', type='GLsizei'),
arg7=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glTexSubImage1D', enabled=True, type=Resource, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, pixels')
)

Function(name='glTexSubImage1DEXT', enabled=True, type=Resource, inheritFrom='glTexSubImage1D')

Function(name='glTexSubImage2D', enabled=True, type=Resource, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='type', type='GLenum'),
arg9=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, pixels')
)

Function(name='glTexSubImage2DEXT', enabled=True, type=Resource, recWrap=False, inheritFrom='glTexSubImage2D')

Function(name='glTexSubImage3D', enabled=True, type=Resource, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, depth, pixels')
)

Function(name='glTexSubImage3DEXT', enabled=True, type=Resource, inheritFrom='glTexSubImage3D', recWrap=False)

Function(name='glTexSubImage3DOES', enabled=True, type=Resource, inheritFrom='glTexSubImage3D')

Function(name='glTexSubImage4DSGIS', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='woffset', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='depth', type='GLsizei'),
arg10=ArgDef(name='size4d', type='GLsizei'),
arg11=ArgDef(name='format', type='GLenum'),
arg12=ArgDef(name='type', type='GLenum'),
arg13=ArgDef(name='pixels', type='const void*')
)

Function(name='glTextureBarrier', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glTextureBarrierNV', enabled=True, type=Param, inheritFrom='glTextureBarrier')

Function(name='glTextureBuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glTextureBufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glTextureBufferRange', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='internalformat', type='GLenum'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glTextureBufferRangeEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg5=ArgDef(name='offset', type='GLintptr'),
arg6=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glTextureColorMaskSGIS', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='red', type='GLboolean'),
arg2=ArgDef(name='green', type='GLboolean'),
arg3=ArgDef(name='blue', type='GLboolean'),
arg4=ArgDef(name='alpha', type='GLboolean')
)

Function(name='glTextureFoveationParametersQCOM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='layer', type='GLuint'),
arg3=ArgDef(name='focalPoint', type='GLuint'),
arg4=ArgDef(name='focalX', type='GLfloat'),
arg5=ArgDef(name='focalY', type='GLfloat'),
arg6=ArgDef(name='gainX', type='GLfloat'),
arg7=ArgDef(name='gainY', type='GLfloat'),
arg8=ArgDef(name='foveaArea', type='GLfloat')
)

Function(name='glTextureImage1DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='border', type='GLint'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='type', type='GLenum'),
arg9=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, pixels')
)

Function(name='glTextureImage2DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='border', type='GLint'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='type', type='GLenum'),
arg10=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, pixels')
)

Function(name='glTextureImage2DMultisampleCoverageNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='coverageSamples', type='GLsizei'),
arg4=ArgDef(name='colorSamples', type='GLsizei'),
arg5=ArgDef(name='internalFormat', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTextureImage2DMultisampleNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='samples', type='GLsizei'),
arg4=ArgDef(name='internalFormat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTextureImage3DEXT', enabled=True, type=Resource, stateTrack=True, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='internalformat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='border', type='GLint'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, depth, pixels')
)

Function(name='glTextureImage3DMultisampleCoverageNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='coverageSamples', type='GLsizei'),
arg4=ArgDef(name='colorSamples', type='GLsizei'),
arg5=ArgDef(name='internalFormat', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTextureImage3DMultisampleNV', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='samples', type='GLsizei'),
arg4=ArgDef(name='internalFormat', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='fixedSampleLocations', type='GLboolean')
)

Function(name='glTextureLightEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum')
)

Function(name='glTextureMaterialEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='face', type='GLenum'),
arg2=ArgDef(name='mode', type='GLenum')
)

Function(name='glTextureNormalEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum')
)

Function(name='glTexturePageCommitmentEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='commit', type='GLboolean')
)

Function(name='glTextureParameterIiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureParameterIivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureParameterIuiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='params', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureParameterIuivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLuint*', wrapType='CGLuint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureParameterf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLfloat')
)

Function(name='glTextureParameterfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLfloat')
)

Function(name='glTextureParameterfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, param')
)

Function(name='glTextureParameterfvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLfloat*', wrapType='CGLfloat::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureParameteri', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glTextureParameteriEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='param', type='GLint')
)

Function(name='glTextureParameteriv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, param')
)

Function(name='glTextureParameterivEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*', wrapType='CGLint::CSParamArray', wrapParams='pname, params')
)

Function(name='glTextureRangeAPPLE', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='length', type='GLsizei'),
arg3=ArgDef(name='pointer', type='const void*')
)

Function(name='glTextureRenderbufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='renderbuffer', type='GLuint', wrapType='CGLRenderbufferEXT')
)

Function(name='glTextureStorage1D', enabled=False, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei')
)

Function(name='glTextureStorage1DEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='levels', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei')
)

Function(name='glTextureStorage2D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei')
)

Function(name='glTextureStorage2DEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='levels', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei')
)

Function(name='glTextureStorage2DMultisample', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTextureStorage2DMultisampleEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='samples', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTextureStorage3D', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei')
)

Function(name='glTextureStorage3DEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='levels', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei')
)

Function(name='glTextureStorage3DMultisample', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalformat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTextureStorage3DMultisampleEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='samples', type='GLsizei'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='depth', type='GLsizei'),
arg8=ArgDef(name='fixedsamplelocations', type='GLboolean')
)

Function(name='glTextureStorageMem1DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='memory', type='GLuint'),
arg6=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTextureStorageMem2DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='memory', type='GLuint'),
arg7=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTextureStorageMem2DMultisampleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='fixedSampleLocations', type='GLboolean'),
arg7=ArgDef(name='memory', type='GLuint'),
arg8=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTextureStorageMem3DEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='levels', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='memory', type='GLuint'),
arg8=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTextureStorageMem3DMultisampleEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='samples', type='GLsizei'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='fixedSampleLocations', type='GLboolean'),
arg8=ArgDef(name='memory', type='GLuint'),
arg9=ArgDef(name='offset', type='GLuint64')
)

Function(name='glTextureStorageSparseAMD', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='internalFormat', type='GLenum'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='height', type='GLsizei'),
arg6=ArgDef(name='depth', type='GLsizei'),
arg7=ArgDef(name='layers', type='GLsizei'),
arg8=ArgDef(name='flags', type='GLbitfield')
)

Function(name='glTextureSubImage1D', enabled=False, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='width', type='GLsizei'),
arg5=ArgDef(name='format', type='GLenum'),
arg6=ArgDef(name='type', type='GLenum'),
arg7=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='GetTargetOfTextureOrCrash(texture), format, type, width, pixels')
)

Function(name='glTextureSubImage1DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='format', type='GLenum'),
arg7=ArgDef(name='type', type='GLenum'),
arg8=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, pixels')
)

Function(name='glTextureSubImage2D', enabled=True, type=Resource, recCond='ConditionTextureES(_recorder)', preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='width', type='GLsizei'),
arg6=ArgDef(name='height', type='GLsizei'),
arg7=ArgDef(name='format', type='GLenum'),
arg8=ArgDef(name='type', type='GLenum'),
arg9=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='GetTargetOfTextureOrCrash(texture), format, type, width, height, pixels')
)

Function(name='glTextureSubImage2DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='format', type='GLenum'),
arg9=ArgDef(name='type', type='GLenum'),
arg10=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, pixels')
)

Function(name='glTextureSubImage3D', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint'),
arg3=ArgDef(name='xoffset', type='GLint'),
arg4=ArgDef(name='yoffset', type='GLint'),
arg5=ArgDef(name='zoffset', type='GLint'),
arg6=ArgDef(name='width', type='GLsizei'),
arg7=ArgDef(name='height', type='GLsizei'),
arg8=ArgDef(name='depth', type='GLsizei'),
arg9=ArgDef(name='format', type='GLenum'),
arg10=ArgDef(name='type', type='GLenum'),
arg11=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='GetTargetOfTextureOrCrash(texture), format, type, width, height, depth, pixels')
)

Function(name='glTextureSubImage3DEXT', enabled=True, type=Resource, preSchedule='coherentBufferUpdate_PS(_recorder)',
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='level', type='GLint'),
arg4=ArgDef(name='xoffset', type='GLint'),
arg5=ArgDef(name='yoffset', type='GLint'),
arg6=ArgDef(name='zoffset', type='GLint'),
arg7=ArgDef(name='width', type='GLsizei'),
arg8=ArgDef(name='height', type='GLsizei'),
arg9=ArgDef(name='depth', type='GLsizei'),
arg10=ArgDef(name='format', type='GLenum'),
arg11=ArgDef(name='type', type='GLenum'),
arg12=ArgDef(name='pixels', type='const void*', wrapType='CGLTexResource', wrapParams='target, format, type, width, height, depth, pixels')
)

Function(name='glTextureView', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='origtexture', type='GLuint', wrapType='CGLTexture'),
arg4=ArgDef(name='internalformat', type='GLenum'),
arg5=ArgDef(name='minlevel', type='GLuint'),
arg6=ArgDef(name='numlevels', type='GLuint'),
arg7=ArgDef(name='minlayer', type='GLuint'),
arg8=ArgDef(name='numlayers', type='GLuint')
)

Function(name='glTextureViewEXT', enabled=False, type=None, inheritFrom='glTextureView')

Function(name='glTextureViewOES', enabled=False, type=None, inheritFrom='glTextureView')

Function(name='glTrackMatrixNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='target', type='GLenum'),
arg2=ArgDef(name='address', type='GLuint'),
arg3=ArgDef(name='matrix', type='GLenum'),
arg4=ArgDef(name='transform', type='GLenum')
)

Function(name='glTransformFeedbackAttribsNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLsizei'),
arg2=ArgDef(name='attribs', type='const GLint*'),
arg3=ArgDef(name='bufferMode', type='GLenum')
)

Function(name='glTransformFeedbackBufferBase', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfb', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glTransformFeedbackBufferRange', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='xfb', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='size', type='GLsizeiptr')
)

Function(name='glTransformFeedbackStreamAttribsNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLsizei'),
arg2=ArgDef(name='attribs', type='const GLint*', wrapParams='count, attribs'),
arg3=ArgDef(name='nbuffers', type='GLsizei'),
arg4=ArgDef(name='bufstreams', type='const GLint*', wrapParams='nbuffers, bufstreams'),
arg5=ArgDef(name='bufferMode', type='GLenum')
)

Function(name='glTransformFeedbackVaryings', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='varyings', type='const GLchar*const*', wrapType='CStringArray', wrapParams='varyings, count'),
arg4=ArgDef(name='bufferMode', type='GLenum')
)

Function(name='glTransformFeedbackVaryingsEXT', enabled=True, type=Param, inheritFrom='glTransformFeedbackVaryings')

Function(name='glTransformFeedbackVaryingsNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='locations', type='const GLint*'),
arg4=ArgDef(name='bufferMode', type='GLenum')
)

Function(name='glTransformPathNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='resultPath', type='GLuint'),
arg2=ArgDef(name='srcPath', type='GLuint'),
arg3=ArgDef(name='transformType', type='GLenum'),
arg4=ArgDef(name='transformValues', type='const GLfloat*', wrapParams='1, transformValues')
)

Function(name='glTranslated', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble')
)

Function(name='glTranslatef', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat')
)

Function(name='glTranslatex', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed')
)

Function(name='glTranslatexOES', enabled=True, type=Param, inheritFrom='glTranslatex')

Function(name='glUniform1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLdouble')
)

Function(name='glUniform1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLdouble*', wrapParams='count, value')
)

Function(name='glUniform1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLfloat')
)

Function(name='glUniform1fARB', enabled=True, type=Param, inheritFrom='glUniform1f')

Function(name='glUniform1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLfloat*', wrapParams='count, value')
)

Function(name='glUniform1fvARB', enabled=True, type=Param, inheritFrom='glUniform1fv')

Function(name='glUniform1i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLint')
)

Function(name='glUniform1i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64')
)

Function(name='glUniform1i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation', wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64EXT')
)

Function(name='glUniform1i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64*', wrapParams='count, value')
)

Function(name='glUniform1i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count, value')
)

Function(name='glUniform1iARB', enabled=True, type=Param, inheritFrom='glUniform1i')

Function(name='glUniform1iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint*', wrapParams='count, value')
)

Function(name='glUniform1ivARB', enabled=True, type=Param, inheritFrom='glUniform1iv')

Function(name='glUniform1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLuint')
)

Function(name='glUniform1ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64')
)

Function(name='glUniform1ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64EXT')
)

Function(name='glUniform1ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapParams='count, value')
)

Function(name='glUniform1ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count, value')
)

Function(name='glUniform1uiEXT', enabled=True, type=Param, inheritFrom='glUniform1ui')

Function(name='glUniform1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint*', wrapParams='count, value')
)

Function(name='glUniform1uivEXT', enabled=True, type=Param, inheritFrom='glUniform1uiv')

Function(name='glUniform2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble')
)

Function(name='glUniform2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 2, value')
)

Function(name='glUniform2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLfloat'),
arg3=ArgDef(name='v1', type='GLfloat')
)

Function(name='glUniform2fARB', enabled=True, type=Param, inheritFrom='glUniform2f')

Function(name='glUniform2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLfloat*', wrapParams='count* 2, value')
)

Function(name='glUniform2fvARB', enabled=True, type=Param, inheritFrom='glUniform2fv')

Function(name='glUniform2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLint'),
arg3=ArgDef(name='v1', type='GLint')
)

Function(name='glUniform2i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64'),
arg3=ArgDef(name='y', type='GLint64')
)

Function(name='glUniform2i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT')
)

Function(name='glUniform2i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64*', wrapParams='count * 2, value')
)

Function(name='glUniform2i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 2, value')
)

Function(name='glUniform2iARB', enabled=True, type=Param, inheritFrom='glUniform2i')

Function(name='glUniform2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint*', wrapParams='count * 2, value')
)

Function(name='glUniform2ivARB', enabled=True, type=Param, inheritFrom='glUniform2iv')

Function(name='glUniform2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLuint'),
arg3=ArgDef(name='v1', type='GLuint')
)

Function(name='glUniform2ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64'),
arg3=ArgDef(name='y', type='GLuint64')
)

Function(name='glUniform2ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT')
)

Function(name='glUniform2ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 2, value')
)

Function(name='glUniform2ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 2, value')
)

Function(name='glUniform2uiEXT', enabled=True, type=Param, inheritFrom='glUniform2ui')

Function(name='glUniform2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint*', wrapParams='count * 2, value')
)

Function(name='glUniform2uivEXT', enabled=True, type=Param, inheritFrom='glUniform2uiv')

Function(name='glUniform3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glUniform3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 3, value')
)

Function(name='glUniform3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLfloat'),
arg3=ArgDef(name='v1', type='GLfloat'),
arg4=ArgDef(name='v2', type='GLfloat')
)

Function(name='glUniform3fARB', enabled=True, type=Param, inheritFrom='glUniform3f')

Function(name='glUniform3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 3, value')
)

Function(name='glUniform3fvARB', enabled=True, type=Param, inheritFrom='glUniform3fv')

Function(name='glUniform3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLint'),
arg3=ArgDef(name='v1', type='GLint'),
arg4=ArgDef(name='v2', type='GLint')
)

Function(name='glUniform3i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64'),
arg3=ArgDef(name='y', type='GLint64'),
arg4=ArgDef(name='z', type='GLint64')
)

Function(name='glUniform3i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT'),
arg4=ArgDef(name='z', type='GLint64EXT')
)

Function(name='glUniform3i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64*', wrapParams='count * 3, value')
)

Function(name='glUniform3i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 3, value')
)

Function(name='glUniform3iARB', enabled=True, type=Param, inheritFrom='glUniform3i')

Function(name='glUniform3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint*', wrapParams='count * 3, value')
)

Function(name='glUniform3ivARB', enabled=True, type=Param, inheritFrom='glUniform3iv')

Function(name='glUniform3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLuint'),
arg3=ArgDef(name='v1', type='GLuint'),
arg4=ArgDef(name='v2', type='GLuint')
)

Function(name='glUniform3ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64'),
arg3=ArgDef(name='y', type='GLuint64'),
arg4=ArgDef(name='z', type='GLuint64')
)

Function(name='glUniform3ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT'),
arg4=ArgDef(name='z', type='GLuint64EXT')
)

Function(name='glUniform3ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 3, value')
)

Function(name='glUniform3ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 3, value')
)

Function(name='glUniform3uiEXT', enabled=True, type=Param, inheritFrom='glUniform3ui')

Function(name='glUniform3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint*', wrapParams='count * 3, value')
)

Function(name='glUniform3uivEXT', enabled=True, type=Param, inheritFrom='glUniform3uiv')

Function(name='glUniform4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble'),
arg5=ArgDef(name='w', type='GLdouble')
)

Function(name='glUniform4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 4, value')
)

Function(name='glUniform4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLfloat'),
arg3=ArgDef(name='v1', type='GLfloat'),
arg4=ArgDef(name='v2', type='GLfloat'),
arg5=ArgDef(name='v3', type='GLfloat')
)

Function(name='glUniform4fARB', enabled=True, type=Param, inheritFrom='glUniform4f')

Function(name='glUniform4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 4, value')
)

Function(name='glUniform4fvARB', enabled=True, type=Param, inheritFrom='glUniform4fv')

Function(name='glUniform4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLint'),
arg3=ArgDef(name='v1', type='GLint'),
arg4=ArgDef(name='v2', type='GLint'),
arg5=ArgDef(name='v3', type='GLint')
)

Function(name='glUniform4i64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64'),
arg3=ArgDef(name='y', type='GLint64'),
arg4=ArgDef(name='z', type='GLint64'),
arg5=ArgDef(name='w', type='GLint64')
)

Function(name='glUniform4i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT'),
arg4=ArgDef(name='z', type='GLint64EXT'),
arg5=ArgDef(name='w', type='GLint64EXT')
)

Function(name='glUniform4i64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64*', wrapParams='count * 4, value')
)

Function(name='glUniform4i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint64EXT*', wrapParams='count * 4, value')
)

Function(name='glUniform4iARB', enabled=True, type=Param, inheritFrom='glUniform4i')

Function(name='glUniform4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLint*', wrapParams='count * 4, value')
)

Function(name='glUniform4ivARB', enabled=True, type=Param, inheritFrom='glUniform4iv')

Function(name='glUniform4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='v0', type='GLuint'),
arg3=ArgDef(name='v1', type='GLuint'),
arg4=ArgDef(name='v2', type='GLuint'),
arg5=ArgDef(name='v3', type='GLuint')
)

Function(name='glUniform4ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64'),
arg3=ArgDef(name='y', type='GLuint64'),
arg4=ArgDef(name='z', type='GLuint64'),
arg5=ArgDef(name='w', type='GLuint64')
)

Function(name='glUniform4ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT'),
arg4=ArgDef(name='z', type='GLuint64EXT'),
arg5=ArgDef(name='w', type='GLuint64EXT')
)

Function(name='glUniform4ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapParams='count * 4, value')
)

Function(name='glUniform4ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count * 4, value')
)

Function(name='glUniform4uiEXT', enabled=True, type=Param, inheritFrom='glUniform4ui')

Function(name='glUniform4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint*', wrapParams='count * 4, value')
)

Function(name='glUniform4uivEXT', enabled=True, type=Param, inheritFrom='glUniform4uiv')

Function(name='glUniformBlockBinding', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='uniformBlockIndex', type='GLuint', wrapType='CGLUniformBlockIndex', wrapParams='program, uniformBlockIndex'),
arg3=ArgDef(name='uniformBlockBinding', type='GLuint')
)

Function(name='glUniformBufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram'),
arg2=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glUniformHandleui64ARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='value', type='GLuint64', wrapType='CGLTextureHandle')
)

Function(name='glUniformHandleui64IMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint'),
arg2=ArgDef(name='value', type='GLuint64')
)

Function(name='glUniformHandleui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='value', type='GLuint64')
)

Function(name='glUniformHandleui64vARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapType='CGLTextureHandle::CSArray', wrapParams='count, value')
)

Function(name='glUniformHandleui64vIMG', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*')
)

Function(name='glUniformHandleui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64*', wrapParams='count, value')
)

Function(name='glUniformMatrix2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 4, value')
)

Function(name='glUniformMatrix2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 4, value')
)

Function(name='glUniformMatrix2fvARB', enabled=True, type=Param, inheritFrom='glUniformMatrix2fv')

Function(name='glUniformMatrix2x3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 6, value')
)

Function(name='glUniformMatrix2x3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 6, value')
)

Function(name='glUniformMatrix2x3fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix2x3fv')

Function(name='glUniformMatrix2x4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 8, value')
)

Function(name='glUniformMatrix2x4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 8, value')
)

Function(name='glUniformMatrix2x4fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix2x4fv')

Function(name='glUniformMatrix3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 9, value')
)

Function(name='glUniformMatrix3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 9, value')
)

Function(name='glUniformMatrix3fvARB', enabled=True, type=Param, inheritFrom='glUniformMatrix3fv')

Function(name='glUniformMatrix3x2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 6, value')
)

Function(name='glUniformMatrix3x2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 6, value')
)

Function(name='glUniformMatrix3x2fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix3x2fv')

Function(name='glUniformMatrix3x4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 12, value')
)

Function(name='glUniformMatrix3x4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 12, value')
)

Function(name='glUniformMatrix3x4fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix3x4fv')

Function(name='glUniformMatrix4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 16, value')
)

Function(name='glUniformMatrix4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 16, value')
)

Function(name='glUniformMatrix4fvARB', enabled=True, type=Param, inheritFrom='glUniformMatrix4fv')

Function(name='glUniformMatrix4x2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 8, value')
)

Function(name='glUniformMatrix4x2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 8, value')
)

Function(name='glUniformMatrix4x2fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix4x2fv')

Function(name='glUniformMatrix4x3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLdouble*', wrapParams='count * 12, value')
)

Function(name='glUniformMatrix4x3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='transpose', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLfloat*', wrapParams='count * 12, value')
)

Function(name='glUniformMatrix4x3fvNV', enabled=False, type=None, inheritFrom='glUniformMatrix4x3fv')

Function(name='glUniformSubroutinesuiv', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='shadertype', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='indices', type='const GLuint*', wrapParams='count, indices')
)

Function(name='glUniformui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='value', type='GLuint64EXT')
)

Function(name='glUniformui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='location', type='GLint', wrapType='CGLUniformLocation' , wrapParams='current_program_tag, location'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='value', type='const GLuint64EXT*', wrapParams='count, value')
)

Function(name='glUnlockArraysEXT', enabled=True, type=Param,
retV=RetDef(type='void')
)

Function(name='glUnmapBuffer', enabled=True, type=Param, stateTrack=True, preToken='CgitsUnmapBuffer(target)', recCond='ConditionBufferES(_recorder)', execPostRecWrap=True,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='target', type='GLenum')
)

Function(name='glUnmapBufferARB', enabled=True, type=Param, inheritFrom='glUnmapBuffer')

Function(name='glUnmapBufferOES', enabled=True, type=Param, inheritFrom='glUnmapBuffer')

Function(name='glUnmapNamedBuffer', enabled=True, type=Param, stateTrack=True, preToken='CgitsUnmapBuffer(0, buffer)', execPostRecWrap=True,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glUnmapNamedBufferEXT', enabled=True, type=Param, preToken='CgitsUnmapBuffer(0, buffer)', inheritFrom='glUnmapNamedBuffer')

Function(name='glUnmapObjectBufferATI', enabled=True, type=Param, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glUnmapTexture2DINTEL', enabled=True, type=Param, ccodeWrap=True, stateTrack=True, execPostRecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='texture', type='GLuint', wrapType='CGLTexture'),
arg2=ArgDef(name='level', type='GLint')
)

Function(name='glUpdateObjectBufferATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg2=ArgDef(name='offset', type='GLuint'),
arg3=ArgDef(name='size', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*'),
arg5=ArgDef(name='preserve', type='GLenum')
)

Function(name='glUseProgram', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glUseProgramObjectARB', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glUseProgramStages', enabled=True, type=Bind, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline'),
arg2=ArgDef(name='stages', type='GLbitfield'),
arg3=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glUseProgramStagesEXT', enabled=True, type=Bind, inheritFrom='glUseProgramStages')

Function(name='glUseShaderProgramEXT', enabled=True, type=Bind,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glVDPAUFiniNV', enabled=False, type=None,
retV=RetDef(type='void')
)

Function(name='glVDPAUGetSurfaceivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='surface', type='GLvdpauSurfaceNV'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='bufSize', type='GLsizei'),
arg4=ArgDef(name='length', type='GLsizei*'),
arg5=ArgDef(name='values', type='GLint*')
)

Function(name='glVDPAUInitNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vdpDevice', type='const void*'),
arg2=ArgDef(name='getProcAddress', type='const void*')
)

Function(name='glVDPAUIsSurfaceNV', enabled=False, type=None,
retV=RetDef(type='GLboolean'),
arg1=ArgDef(name='surface', type='GLvdpauSurfaceNV')
)

Function(name='glVDPAUMapSurfacesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numSurfaces', type='GLsizei'),
arg2=ArgDef(name='surfaces', type='const GLvdpauSurfaceNV*')
)

Function(name='glVDPAURegisterOutputSurfaceNV', enabled=False, type=None,
retV=RetDef(type='GLvdpauSurfaceNV'),
arg1=ArgDef(name='vdpSurface', type='const void*'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='numTextureNames', type='GLsizei'),
arg4=ArgDef(name='textureNames', type='const GLuint*')
)

Function(name='glVDPAURegisterVideoSurfaceNV', enabled=False, type=None,
retV=RetDef(type='GLvdpauSurfaceNV'),
arg1=ArgDef(name='vdpSurface', type='const void*'),
arg2=ArgDef(name='target', type='GLenum'),
arg3=ArgDef(name='numTextureNames', type='GLsizei'),
arg4=ArgDef(name='textureNames', type='const GLuint*')
)

Function(name='glVDPAUSurfaceAccessNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='surface', type='GLvdpauSurfaceNV'),
arg2=ArgDef(name='access', type='GLenum')
)

Function(name='glVDPAUUnmapSurfacesNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='numSurface', type='GLsizei'),
arg2=ArgDef(name='surfaces', type='const GLvdpauSurfaceNV*')
)

Function(name='glVDPAUUnregisterSurfaceNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='surface', type='GLvdpauSurfaceNV')
)

Function(name='glValidateProgram', enabled=True, type=Resource,
retV=RetDef(type='void'),
arg1=ArgDef(name='program', type='GLuint', wrapType='CGLProgram')
)

Function(name='glValidateProgramARB', enabled=True, type=Resource, inheritFrom='glValidateProgram',
arg1=ArgDef(name='programObj', type='GLhandleARB', wrapType='CGLProgram')
)

Function(name='glValidateProgramPipeline', enabled=True, type=Resource,
retV=RetDef(type='void'),
arg1=ArgDef(name='pipeline', type='GLuint', wrapType='CGLPipeline')
)

Function(name='glValidateProgramPipelineEXT', enabled=True, type=Resource, inheritFrom='glValidateProgramPipeline')

Function(name='glVariantArrayObjectATI', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg5=ArgDef(name='offset', type='GLuint')
)

Function(name='glVariantPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLuint'),
arg4=ArgDef(name='addr', type='const void*')
)

Function(name='glVariantbvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLbyte*')
)

Function(name='glVariantdvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLdouble*')
)

Function(name='glVariantfvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLfloat*')
)

Function(name='glVariantivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLint*')
)

Function(name='glVariantsvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLshort*')
)

Function(name='glVariantubvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLubyte*')
)

Function(name='glVariantuivEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLuint*')
)

Function(name='glVariantusvEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='id', type='GLuint'),
arg2=ArgDef(name='addr', type='const GLushort*')
)

Function(name='glVertex2bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLbyte'),
arg2=ArgDef(name='y', type='GLbyte')
)

Function(name='glVertex2bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glVertex2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble')
)

Function(name='glVertex2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glVertex2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat')
)

Function(name='glVertex2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glVertex2hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLhalfNV'),
arg2=ArgDef(name='y', type='GLhalfNV')
)

Function(name='glVertex2hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='2, v')
)

Function(name='glVertex2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint')
)

Function(name='glVertex2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glVertex2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort')
)

Function(name='glVertex2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glVertex2xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed')
)

Function(name='glVertex2xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glVertex3bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLbyte'),
arg2=ArgDef(name='y', type='GLbyte'),
arg3=ArgDef(name='z', type='GLbyte')
)

Function(name='glVertex3bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glVertex3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble')
)

Function(name='glVertex3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glVertex3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat')
)

Function(name='glVertex3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glVertex3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLhalfNV'),
arg2=ArgDef(name='y', type='GLhalfNV'),
arg3=ArgDef(name='z', type='GLhalfNV')
)

Function(name='glVertex3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glVertex3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint')
)

Function(name='glVertex3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glVertex3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort')
)

Function(name='glVertex3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glVertex3xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed')
)

Function(name='glVertex3xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glVertex4bOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLbyte'),
arg2=ArgDef(name='y', type='GLbyte'),
arg3=ArgDef(name='z', type='GLbyte'),
arg4=ArgDef(name='w', type='GLbyte')
)

Function(name='glVertex4bvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLbyte*')
)

Function(name='glVertex4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble'),
arg4=ArgDef(name='w', type='GLdouble')
)

Function(name='glVertex4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glVertex4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat'),
arg4=ArgDef(name='w', type='GLfloat')
)

Function(name='glVertex4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glVertex4hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLhalfNV'),
arg2=ArgDef(name='y', type='GLhalfNV'),
arg3=ArgDef(name='z', type='GLhalfNV'),
arg4=ArgDef(name='w', type='GLhalfNV')
)

Function(name='glVertex4hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLhalfNV*', wrapParams='4, v')
)

Function(name='glVertex4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint'),
arg4=ArgDef(name='w', type='GLint')
)

Function(name='glVertex4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glVertex4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort'),
arg4=ArgDef(name='w', type='GLshort')
)

Function(name='glVertex4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glVertex4xOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfixed'),
arg2=ArgDef(name='y', type='GLfixed'),
arg3=ArgDef(name='z', type='GLfixed')
)

Function(name='glVertex4xvOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='coords', type='const GLfixed*')
)

Function(name='glVertexArrayAttribBinding', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='bindingindex', type='GLuint')
)

Function(name='glVertexArrayAttribFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='normalized', type='GLboolean'),
arg6=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayAttribIFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayAttribLFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayBindVertexBufferEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='bindingindex', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexArrayBindingDivisor', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='bindingindex', type='GLuint'),
arg3=ArgDef(name='divisor', type='GLuint')
)

Function(name='glVertexArrayColorOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayEdgeFlagOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayElementBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer')
)

Function(name='glVertexArrayFogCoordOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayIndexOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayMultiTexCoordOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='texunit', type='GLenum'),
arg4=ArgDef(name='size', type='GLint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='stride', type='GLsizei'),
arg7=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayNormalOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayParameteriAPPLE', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glVertexArrayRangeAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='length', type='GLsizei'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glVertexArrayRangeNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='length', type='GLsizei'),
arg2=ArgDef(name='pointer', type='const void*')
)

Function(name='glVertexArraySecondaryColorOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayTexCoordOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayVertexAttribBindingEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='bindingindex', type='GLuint')
)

Function(name='glVertexArrayVertexAttribDivisorEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='index', type='GLuint'),
arg3=ArgDef(name='divisor', type='GLuint')
)

Function(name='glVertexArrayVertexAttribFormatEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='normalized', type='GLboolean'),
arg6=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayVertexAttribIFormatEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayVertexAttribIOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='size', type='GLint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='stride', type='GLsizei'),
arg7=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayVertexAttribLFormatEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='attribindex', type='GLuint'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexArrayVertexAttribLOffsetEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='size', type='GLint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='stride', type='GLsizei'),
arg7=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayVertexAttribOffsetEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='index', type='GLuint'),
arg4=ArgDef(name='size', type='GLint'),
arg5=ArgDef(name='type', type='GLenum'),
arg6=ArgDef(name='normalized', type='GLboolean'),
arg7=ArgDef(name='stride', type='GLsizei'),
arg8=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexArrayVertexBindingDivisorEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='bindingindex', type='GLuint'),
arg3=ArgDef(name='divisor', type='GLuint')
)

Function(name='glVertexArrayVertexBuffer', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='bindingindex', type='GLuint'),
arg3=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg4=ArgDef(name='offset', type='GLintptr'),
arg5=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexArrayVertexBuffers', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='first', type='GLuint'),
arg3=ArgDef(name='count', type='GLsizei'),
arg4=ArgDef(name='buffers', type='const GLuint*'),
arg5=ArgDef(name='offsets', type='const GLintptr*'),
arg6=ArgDef(name='strides', type='const GLsizei*')
)

Function(name='glVertexArrayVertexOffsetEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vaobj', type='GLuint'),
arg2=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg3=ArgDef(name='size', type='GLint'),
arg4=ArgDef(name='type', type='GLenum'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='offset', type='GLintptr')
)

Function(name='glVertexAttrib1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble')
)

Function(name='glVertexAttrib1dARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1d')

Function(name='glVertexAttrib1dNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1d')

Function(name='glVertexAttrib1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='1, v')
)

Function(name='glVertexAttrib1dvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1dv')

Function(name='glVertexAttrib1dvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1dv')

Function(name='glVertexAttrib1f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat')
)

Function(name='glVertexAttrib1fARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1f')

Function(name='glVertexAttrib1fNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1f')

Function(name='glVertexAttrib1fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='1, v')
)

Function(name='glVertexAttrib1fvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1fv')

Function(name='glVertexAttrib1fvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1fv')

Function(name='glVertexAttrib1hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLhalfNV')
)

Function(name='glVertexAttrib1hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLhalfNV*', wrapParams='1, v')
)

Function(name='glVertexAttrib1s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLshort')
)

Function(name='glVertexAttrib1sARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1s')

Function(name='glVertexAttrib1sNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1s')

Function(name='glVertexAttrib1sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='1, v')
)

Function(name='glVertexAttrib1svARB', enabled=True, type=Param, inheritFrom='glVertexAttrib1sv')

Function(name='glVertexAttrib1svNV', enabled=True, type=Param, inheritFrom='glVertexAttrib1sv')

Function(name='glVertexAttrib2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble')
)

Function(name='glVertexAttrib2dARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2d')

Function(name='glVertexAttrib2dNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2d')

Function(name='glVertexAttrib2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glVertexAttrib2dvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2dv')

Function(name='glVertexAttrib2dvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2dv')

Function(name='glVertexAttrib2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat')
)

Function(name='glVertexAttrib2fARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2f')

Function(name='glVertexAttrib2fNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2f')

Function(name='glVertexAttrib2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glVertexAttrib2fvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2fv')

Function(name='glVertexAttrib2fvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2fv')

Function(name='glVertexAttrib2hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLhalfNV'),
arg3=ArgDef(name='y', type='GLhalfNV')
)

Function(name='glVertexAttrib2hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLhalfNV*', wrapParams='2, v')
)

Function(name='glVertexAttrib2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort')
)

Function(name='glVertexAttrib2sARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2s')

Function(name='glVertexAttrib2sNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2s')

Function(name='glVertexAttrib2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glVertexAttrib2svARB', enabled=True, type=Param, inheritFrom='glVertexAttrib2sv')

Function(name='glVertexAttrib2svNV', enabled=True, type=Param, inheritFrom='glVertexAttrib2sv')

Function(name='glVertexAttrib3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glVertexAttrib3dARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3d')

Function(name='glVertexAttrib3dNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3d')

Function(name='glVertexAttrib3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glVertexAttrib3dvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3dv')

Function(name='glVertexAttrib3dvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3dv')

Function(name='glVertexAttrib3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glVertexAttrib3fARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3f')

Function(name='glVertexAttrib3fNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3f')

Function(name='glVertexAttrib3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glVertexAttrib3fvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3fv')

Function(name='glVertexAttrib3fvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3fv')

Function(name='glVertexAttrib3hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLhalfNV'),
arg3=ArgDef(name='y', type='GLhalfNV'),
arg4=ArgDef(name='z', type='GLhalfNV')
)

Function(name='glVertexAttrib3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3, v')
)

Function(name='glVertexAttrib3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort'),
arg4=ArgDef(name='z', type='GLshort')
)

Function(name='glVertexAttrib3sARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3s')

Function(name='glVertexAttrib3sNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3s')

Function(name='glVertexAttrib3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glVertexAttrib3svARB', enabled=True, type=Param, inheritFrom='glVertexAttrib3sv')

Function(name='glVertexAttrib3svNV', enabled=True, type=Param, inheritFrom='glVertexAttrib3sv')

Function(name='glVertexAttrib4Nbv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLbyte*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NbvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nbv')

Function(name='glVertexAttrib4Niv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NivARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Niv')

Function(name='glVertexAttrib4Nsv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NsvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nsv')

Function(name='glVertexAttrib4Nub', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLubyte'),
arg3=ArgDef(name='y', type='GLubyte'),
arg4=ArgDef(name='z', type='GLubyte'),
arg5=ArgDef(name='w', type='GLubyte')
)

Function(name='glVertexAttrib4NubARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nub')

Function(name='glVertexAttrib4Nubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLubyte*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NubvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nubv')

Function(name='glVertexAttrib4Nuiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NuivARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nuiv')

Function(name='glVertexAttrib4Nusv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLushort*', wrapParams='4, v')
)

Function(name='glVertexAttrib4NusvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4Nusv')

Function(name='glVertexAttrib4bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLbyte*', wrapParams='4, v')
)

Function(name='glVertexAttrib4bvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4bv')

Function(name='glVertexAttrib4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble'),
arg5=ArgDef(name='w', type='GLdouble')
)

Function(name='glVertexAttrib4dARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4d')

Function(name='glVertexAttrib4dNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4d')

Function(name='glVertexAttrib4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glVertexAttrib4dvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4dv')

Function(name='glVertexAttrib4dvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4dv')

Function(name='glVertexAttrib4f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat'),
arg5=ArgDef(name='w', type='GLfloat')
)

Function(name='glVertexAttrib4fARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4f')

Function(name='glVertexAttrib4fNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4f')

Function(name='glVertexAttrib4fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glVertexAttrib4fvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4fv')

Function(name='glVertexAttrib4fvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4fv')

Function(name='glVertexAttrib4hNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLhalfNV'),
arg3=ArgDef(name='y', type='GLhalfNV'),
arg4=ArgDef(name='z', type='GLhalfNV'),
arg5=ArgDef(name='w', type='GLhalfNV')
)

Function(name='glVertexAttrib4hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLhalfNV*', wrapParams='4, v')
)

Function(name='glVertexAttrib4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glVertexAttrib4ivARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4iv')

Function(name='glVertexAttrib4s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort'),
arg4=ArgDef(name='z', type='GLshort'),
arg5=ArgDef(name='w', type='GLshort')
)

Function(name='glVertexAttrib4sARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4s')

Function(name='glVertexAttrib4sNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4s')

Function(name='glVertexAttrib4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glVertexAttrib4svARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4sv')

Function(name='glVertexAttrib4svNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4sv')

Function(name='glVertexAttrib4ubNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLubyte'),
arg3=ArgDef(name='y', type='GLubyte'),
arg4=ArgDef(name='z', type='GLubyte'),
arg5=ArgDef(name='w', type='GLubyte')
)

Function(name='glVertexAttrib4ubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLubyte*', wrapParams='4, v')
)

Function(name='glVertexAttrib4ubvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4ubv')

Function(name='glVertexAttrib4ubvNV', enabled=True, type=Param, inheritFrom='glVertexAttrib4ubv')

Function(name='glVertexAttrib4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='4, v')
)

Function(name='glVertexAttrib4uivARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4uiv')

Function(name='glVertexAttrib4usv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLushort*', wrapParams='4, v')
)

Function(name='glVertexAttrib4usvARB', enabled=True, type=Param, inheritFrom='glVertexAttrib4usv')

Function(name='glVertexAttribArrayObjectATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='normalized', type='GLboolean'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='buffer', type='GLuint', wrapType='CGLBuffer'),
arg7=ArgDef(name='offset', type='GLuint')
)

Function(name='glVertexAttribBinding', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='attribindex', type='GLuint'),
arg2=ArgDef(name='bindingindex', type='GLuint')
)

Function(name='glVertexAttribDivisor', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='divisor', type='GLuint')
)

Function(name='glVertexAttribDivisorANGLE', enabled=True, type=Param, inheritFrom='glVertexAttribDivisor', runWrap=False)

Function(name='glVertexAttribDivisorARB', enabled=True, type=Param, inheritFrom='glVertexAttribDivisor', runWrap=False)

Function(name='glVertexAttribDivisorEXT', enabled=False, type=None, inheritFrom='glVertexAttribDivisor', runWrap=False)

Function(name='glVertexAttribDivisorNV', enabled=False, type=None, inheritFrom='glVertexAttribDivisor', runWrap=False)

Function(name='glVertexAttribFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='attribindex', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='normalized', type='GLboolean'),
arg5=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexAttribFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='normalized', type='GLboolean'),
arg5=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexAttribI1i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint')
)

Function(name='glVertexAttribI1iEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI1i')

Function(name='glVertexAttribI1iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='1, v')
)

Function(name='glVertexAttribI1ivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI1iv')

Function(name='glVertexAttribI1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint')
)

Function(name='glVertexAttribI1uiEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI1ui')

Function(name='glVertexAttribI1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='1, v')
)

Function(name='glVertexAttribI1uivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI1uiv')

Function(name='glVertexAttribI2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint')
)

Function(name='glVertexAttribI2iEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI2i')

Function(name='glVertexAttribI2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glVertexAttribI2ivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI2iv')

Function(name='glVertexAttribI2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint'),
arg3=ArgDef(name='y', type='GLuint')
)

Function(name='glVertexAttribI2uiEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI2ui')

Function(name='glVertexAttribI2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='2, v')
)

Function(name='glVertexAttribI2uivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI2uiv')

Function(name='glVertexAttribI3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint'),
arg4=ArgDef(name='z', type='GLint')
)

Function(name='glVertexAttribI3iEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI3i')

Function(name='glVertexAttribI3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glVertexAttribI3ivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI3iv')

Function(name='glVertexAttribI3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint'),
arg3=ArgDef(name='y', type='GLuint'),
arg4=ArgDef(name='z', type='GLuint')
)

Function(name='glVertexAttribI3uiEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI3ui')

Function(name='glVertexAttribI3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='3, v')
)

Function(name='glVertexAttribI3uivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI3uiv')

Function(name='glVertexAttribI4bv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLbyte*', wrapParams='4, v')
)

Function(name='glVertexAttribI4bvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4bv')

Function(name='glVertexAttribI4i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint'),
arg4=ArgDef(name='z', type='GLint'),
arg5=ArgDef(name='w', type='GLint')
)

Function(name='glVertexAttribI4iEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4i')

Function(name='glVertexAttribI4iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glVertexAttribI4ivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4iv')

Function(name='glVertexAttribI4sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glVertexAttribI4svEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4sv')

Function(name='glVertexAttribI4ubv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLubyte*', wrapParams='4, v')
)

Function(name='glVertexAttribI4ubvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4ubv')

Function(name='glVertexAttribI4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint'),
arg3=ArgDef(name='y', type='GLuint'),
arg4=ArgDef(name='z', type='GLuint'),
arg5=ArgDef(name='w', type='GLuint')
)

Function(name='glVertexAttribI4uiEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4ui')

Function(name='glVertexAttribI4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint*', wrapParams='4, v')
)

Function(name='glVertexAttribI4uivEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4uiv')

Function(name='glVertexAttribI4usv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLushort*', wrapParams='4, v')
)

Function(name='glVertexAttribI4usvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribI4usv')

Function(name='glVertexAttribIFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='attribindex', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexAttribIFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexAttribIPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glVertexAttribIPointerEXT', enabled=True, type=Param, inheritFrom='glVertexAttribIPointer')

Function(name='glVertexAttribL1d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble')
)

Function(name='glVertexAttribL1dEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL1d')

Function(name='glVertexAttribL1dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='1, v')
)

Function(name='glVertexAttribL1dvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL1dv')

Function(name='glVertexAttribL1i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint64EXT')
)

Function(name='glVertexAttribL1i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint64EXT*', wrapParams='1, v')
)

Function(name='glVertexAttribL1ui64ARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint64EXT')
)

Function(name='glVertexAttribL1ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint64EXT')
)

Function(name='glVertexAttribL1ui64vARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint64EXT*')
)

Function(name='glVertexAttribL1ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint64EXT*', wrapParams='1, v')
)

Function(name='glVertexAttribL2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble')
)

Function(name='glVertexAttribL2dEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL2d')

Function(name='glVertexAttribL2dv', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glVertexAttribL2dvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL2dv')

Function(name='glVertexAttribL2i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT')
)

Function(name='glVertexAttribL2i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint64EXT*', wrapParams='2, v')
)

Function(name='glVertexAttribL2ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT')
)

Function(name='glVertexAttribL2ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint64EXT*', wrapParams='2, v')
)

Function(name='glVertexAttribL3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glVertexAttribL3dEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL3d')

Function(name='glVertexAttribL3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glVertexAttribL3dvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL3dv')

Function(name='glVertexAttribL3i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT'),
arg4=ArgDef(name='z', type='GLint64EXT')
)

Function(name='glVertexAttribL3i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint64EXT*', wrapParams='3, v')
)

Function(name='glVertexAttribL3ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT'),
arg4=ArgDef(name='z', type='GLuint64EXT')
)

Function(name='glVertexAttribL3ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint64EXT*', wrapParams='3, v')
)

Function(name='glVertexAttribL4d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble'),
arg5=ArgDef(name='w', type='GLdouble')
)

Function(name='glVertexAttribL4dEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL4d')

Function(name='glVertexAttribL4dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glVertexAttribL4dvEXT', enabled=True, type=Param, inheritFrom='glVertexAttribL4dv')

Function(name='glVertexAttribL4i64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLint64EXT'),
arg3=ArgDef(name='y', type='GLint64EXT'),
arg4=ArgDef(name='z', type='GLint64EXT'),
arg5=ArgDef(name='w', type='GLint64EXT')
)

Function(name='glVertexAttribL4i64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLint64EXT*', wrapParams='4, v')
)

Function(name='glVertexAttribL4ui64NV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLuint64EXT'),
arg3=ArgDef(name='y', type='GLuint64EXT'),
arg4=ArgDef(name='z', type='GLuint64EXT'),
arg5=ArgDef(name='w', type='GLuint64EXT')
)

Function(name='glVertexAttribL4ui64vNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLuint64EXT*', wrapParams='4, v')
)

Function(name='glVertexAttribLFormat', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='attribindex', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='relativeoffset', type='GLuint')
)

Function(name='glVertexAttribLFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexAttribLPointer', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*')
)

Function(name='glVertexAttribLPointerEXT', enabled=False, type=None, inheritFrom='glVertexAttribLPointer')

Function(name='glVertexAttribP1ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexAttribP1uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='1, value')
)

Function(name='glVertexAttribP2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexAttribP2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='2, value')
)

Function(name='glVertexAttribP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexAttribP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='3, value')
)

Function(name='glVertexAttribP4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexAttribP4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='normalized', type='GLboolean'),
arg4=ArgDef(name='value', type='const GLuint*', wrapParams='4, value')
)

Function(name='glVertexAttribParameteriAMD', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='pname', type='GLenum'),
arg3=ArgDef(name='param', type='GLint')
)

Function(name='glVertexAttribPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='size', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='normalized', type='GLboolean'),
arg5=ArgDef(name='stride', type='GLsizei'),
arg6=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glVertexAttribPointerARB', enabled=True, type=Param, inheritFrom='glVertexAttribPointer')

Function(name='glVertexAttribPointerNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='fsize', type='GLint'),
arg3=ArgDef(name='type', type='GLenum'),
arg4=ArgDef(name='stride', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*')
)

Function(name='glVertexAttribs1dvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLdouble*', wrapParams='count, v')
)

Function(name='glVertexAttribs1fvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='count, v')
)

Function(name='glVertexAttribs1hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLhalfNV*', wrapParams='n, v')
)

Function(name='glVertexAttribs1svNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLshort*', wrapParams='count, v')
)

Function(name='glVertexAttribs2dvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLdouble*', wrapParams='2 * count, v')
)

Function(name='glVertexAttribs2fvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='2 * count, v')
)

Function(name='glVertexAttribs2hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLhalfNV*', wrapParams='2 * n, v')
)

Function(name='glVertexAttribs2svNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLshort*', wrapParams='2 * count, v')
)

Function(name='glVertexAttribs3dvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLdouble*', wrapParams='3 * count, v')
)

Function(name='glVertexAttribs3fvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='3 * count, v')
)

Function(name='glVertexAttribs3hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLhalfNV*', wrapParams='3 * n, v')
)

Function(name='glVertexAttribs3svNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLshort*', wrapParams='3 * count, v')
)

Function(name='glVertexAttribs4dvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLdouble*', wrapParams='4 * count, v')
)

Function(name='glVertexAttribs4fvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='4 * count, v')
)

Function(name='glVertexAttribs4hvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='n', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLhalfNV*', wrapParams='4 * n, v')
)

Function(name='glVertexAttribs4svNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLshort*', wrapParams='4 * count, v')
)

Function(name='glVertexAttribs4ubvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLubyte*', wrapParams='4 * count, v')
)

Function(name='glVertexBindingDivisor', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='bindingindex', type='GLuint'),
arg2=ArgDef(name='divisor', type='GLuint')
)

Function(name='glVertexBlendARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='count', type='GLint')
)

Function(name='glVertexBlendEnvfATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLfloat')
)

Function(name='glVertexBlendEnviATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='pname', type='GLenum'),
arg2=ArgDef(name='param', type='GLint')
)

Function(name='glVertexFormatNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei')
)

Function(name='glVertexP2ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexP2uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='const GLuint*', wrapParams='1, value')
)

Function(name='glVertexP3ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexP3uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='const GLuint*', wrapParams='1, value')
)

Function(name='glVertexP4ui', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='GLuint')
)

Function(name='glVertexP4uiv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='type', type='GLenum'),
arg2=ArgDef(name='value', type='const GLuint*', wrapParams='1, value')
)

Function(name='glVertexPointSizefAPPLE', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLfloat')
)

Function(name='glVertexPointer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glVertexPointerBounds', enabled=True, type=Param, runWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const GLvoid*', wrapType='CAttribPtr'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glVertexPointerEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='count', type='GLsizei'),
arg5=ArgDef(name='pointer', type='const void*', wrapType='CAttribPtr')
)

Function(name='glVertexPointerListIBM', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLint'),
arg4=ArgDef(name='pointer', type='const void**'),
arg5=ArgDef(name='ptrstride', type='GLint')
)

Function(name='glVertexPointervINTEL', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='pointer', type='const void**')
)

Function(name='glVertexStream1dATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble')
)

Function(name='glVertexStream1dvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLdouble*', wrapParams='1, coords')
)

Function(name='glVertexStream1fATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat')
)

Function(name='glVertexStream1fvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfloat*', wrapParams='1, coords')
)

Function(name='glVertexStream1iATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLint')
)

Function(name='glVertexStream1ivATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLint*', wrapParams='1, coords')
)

Function(name='glVertexStream1sATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLshort')
)

Function(name='glVertexStream1svATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLshort*', wrapParams='1, coords')
)

Function(name='glVertexStream2dATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble')
)

Function(name='glVertexStream2dvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLdouble*', wrapParams='2, coords')
)

Function(name='glVertexStream2fATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat')
)

Function(name='glVertexStream2fvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfloat*', wrapParams='2, coords')
)

Function(name='glVertexStream2iATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint')
)

Function(name='glVertexStream2ivATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLint*', wrapParams='2, coords')
)

Function(name='glVertexStream2sATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort')
)

Function(name='glVertexStream2svATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLshort*', wrapParams='2, coords')
)

Function(name='glVertexStream3dATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble')
)

Function(name='glVertexStream3dvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLdouble*', wrapParams='3, coords')
)

Function(name='glVertexStream3fATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat')
)

Function(name='glVertexStream3fvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfloat*', wrapParams='3, coords')
)

Function(name='glVertexStream3iATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint'),
arg4=ArgDef(name='z', type='GLint')
)

Function(name='glVertexStream3ivATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLint*', wrapParams='3, coords')
)

Function(name='glVertexStream3sATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort'),
arg4=ArgDef(name='z', type='GLshort')
)

Function(name='glVertexStream3svATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLshort*', wrapParams='3, coords')
)

Function(name='glVertexStream4dATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLdouble'),
arg3=ArgDef(name='y', type='GLdouble'),
arg4=ArgDef(name='z', type='GLdouble'),
arg5=ArgDef(name='w', type='GLdouble')
)

Function(name='glVertexStream4dvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLdouble*', wrapParams='4, coords')
)

Function(name='glVertexStream4fATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='z', type='GLfloat'),
arg5=ArgDef(name='w', type='GLfloat')
)

Function(name='glVertexStream4fvATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLfloat*', wrapParams='4, coords')
)

Function(name='glVertexStream4iATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLint'),
arg3=ArgDef(name='y', type='GLint'),
arg4=ArgDef(name='z', type='GLint'),
arg5=ArgDef(name='w', type='GLint')
)

Function(name='glVertexStream4ivATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLint*', wrapParams='4, coords')
)

Function(name='glVertexStream4sATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='x', type='GLshort'),
arg3=ArgDef(name='y', type='GLshort'),
arg4=ArgDef(name='z', type='GLshort'),
arg5=ArgDef(name='w', type='GLshort')
)

Function(name='glVertexStream4svATI', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='stream', type='GLenum'),
arg2=ArgDef(name='coords', type='const GLshort*', wrapParams='4, coords')
)

Function(name='glVertexWeightPointerEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glVertexWeightfEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='weight', type='GLfloat')
)

Function(name='glVertexWeightfvEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='weight', type='const GLfloat*', wrapParams='1, weight')
)

Function(name='glVertexWeighthNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='weight', type='GLhalfNV')
)

Function(name='glVertexWeighthvNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='weight', type='const GLhalfNV*', wrapParams='1, weight')
)

Function(name='glVideoCaptureNV', enabled=False, type=None,
retV=RetDef(type='GLenum'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='sequence_num', type='GLuint*'),
arg3=ArgDef(name='capture_time', type='GLuint64EXT*')
)

Function(name='glVideoCaptureStreamParameterdvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLdouble*')
)

Function(name='glVideoCaptureStreamParameterfvNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLfloat*')
)

Function(name='glVideoCaptureStreamParameterivNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='video_capture_slot', type='GLuint'),
arg2=ArgDef(name='stream', type='GLuint'),
arg3=ArgDef(name='pname', type='GLenum'),
arg4=ArgDef(name='params', type='const GLint*')
)

Function(name='glViewport', enabled=True, type=Param, runWrap=True, ccodeWrap=True, preToken='CgitsViewportSettings(x, y, width, height)', recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='width', type='GLsizei'),
arg4=ArgDef(name='height', type='GLsizei')
)

Function(name='glViewportArrayv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='first', type='GLuint'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='v', type='const GLfloat*', wrapParams='4 * count, v')
)

Function(name='glViewportArrayvNV', enabled=False, type=None, inheritFrom='glViewportArrayv')

Function(name='glViewportArrayvOES', enabled=False, type=None, inheritFrom='glViewportArrayv')

Function(name='glViewportIndexedf', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='x', type='GLfloat'),
arg3=ArgDef(name='y', type='GLfloat'),
arg4=ArgDef(name='w', type='GLfloat'),
arg5=ArgDef(name='h', type='GLfloat')
)

Function(name='glViewportIndexedfNV', enabled=False, type=None, inheritFrom='glViewportIndexedf')

Function(name='glViewportIndexedfOES', enabled=False, type=None, inheritFrom='glViewportIndexedf')

Function(name='glViewportIndexedfv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glViewportIndexedfvNV', enabled=False, type=None, inheritFrom='glViewportIndexedfv')

Function(name='glViewportIndexedfvOES', enabled=False, type=None, inheritFrom='glViewportIndexedfv')

Function(name='glViewportPositionWScaleNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='xcoeff', type='GLfloat'),
arg3=ArgDef(name='ycoeff', type='GLfloat')
)

Function(name='glViewportSwizzleNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='index', type='GLuint'),
arg2=ArgDef(name='swizzlex', type='GLenum'),
arg3=ArgDef(name='swizzley', type='GLenum'),
arg4=ArgDef(name='swizzlez', type='GLenum'),
arg5=ArgDef(name='swizzlew', type='GLenum')
)

Function(name='glWaitSemaphoreEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='semaphore', type='GLuint'),
arg2=ArgDef(name='numBufferBarriers', type='GLuint'),
arg3=ArgDef(name='buffers', type='const GLuint*'),
arg4=ArgDef(name='numTextureBarriers', type='GLuint'),
arg5=ArgDef(name='textures', type='const GLuint*'),
arg6=ArgDef(name='srcLayouts', type='const GLenum*')
)

Function(name='glWaitSync', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='sync', type='GLsync', wrapType='CGLsync'),
arg2=ArgDef(name='flags', type='GLbitfield'),
arg3=ArgDef(name='timeout', type='GLuint64')
)

Function(name='glWaitSyncAPPLE', enabled=False, type=Param, inheritFrom='glWaitSync')

Function(name='glWaitVkSemaphoreNV', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='vkSemaphore', type='GLuint64')
)

Function(name='glWeightPathsNV', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='resultPath', type='GLuint'),
arg2=ArgDef(name='numPaths', type='GLsizei'),
arg3=ArgDef(name='paths', type='const GLuint*', wrapParams='1, paths'),
arg4=ArgDef(name='weights', type='const GLfloat*', wrapParams='1, weights')
)

Function(name='glWeightPointerARB', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glWeightPointerOES', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const void*')
)

Function(name='glWeightPointerOESBounds', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='type', type='GLenum'),
arg3=ArgDef(name='stride', type='GLsizei'),
arg4=ArgDef(name='pointer', type='const GLvoid*'),
arg5=ArgDef(name='count', type='GLsizei')
)

Function(name='glWeightbvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLbyte*', wrapParams='size, weights')
)

Function(name='glWeightdvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLdouble*', wrapParams='size, weights')
)

Function(name='glWeightfvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLfloat*', wrapParams='size, weights')
)

Function(name='glWeightivARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLint*', wrapParams='size, weights')
)

Function(name='glWeightsvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLshort*', wrapParams='size, weights')
)

Function(name='glWeightubvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLubyte*', wrapParams='size, weights')
)

Function(name='glWeightuivARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLuint*', wrapParams='size, weights')
)

Function(name='glWeightusvARB', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='size', type='GLint'),
arg2=ArgDef(name='weights', type='const GLushort*', wrapParams='size, weights')
)

Function(name='glWindowPos2d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble')
)

Function(name='glWindowPos2dARB', enabled=True, type=Param, inheritFrom='glWindowPos2d')

Function(name='glWindowPos2dMESA', enabled=True, type=Param, inheritFrom='glWindowPos2d')

Function(name='glWindowPos2dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='2, v')
)

Function(name='glWindowPos2dvARB', enabled=True, type=Param, inheritFrom='glWindowPos2dv')

Function(name='glWindowPos2dvMESA', enabled=True, type=Param, inheritFrom='glWindowPos2dv')

Function(name='glWindowPos2f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat')
)

Function(name='glWindowPos2fARB', enabled=True, type=Param, inheritFrom='glWindowPos2f')

Function(name='glWindowPos2fMESA', enabled=True, type=Param, inheritFrom='glWindowPos2f')

Function(name='glWindowPos2fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='2, v')
)

Function(name='glWindowPos2fvARB', enabled=True, type=Param, inheritFrom='glWindowPos2fv')

Function(name='glWindowPos2fvMESA', enabled=True, type=Param, inheritFrom='glWindowPos2fv')

Function(name='glWindowPos2i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint')
)

Function(name='glWindowPos2iARB', enabled=True, type=Param, inheritFrom='glWindowPos2i')

Function(name='glWindowPos2iMESA', enabled=True, type=Param, inheritFrom='glWindowPos2i')

Function(name='glWindowPos2iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='2, v')
)

Function(name='glWindowPos2ivARB', enabled=True, type=Param, inheritFrom='glWindowPos2iv')

Function(name='glWindowPos2ivMESA', enabled=True, type=Param, inheritFrom='glWindowPos2iv')

Function(name='glWindowPos2s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort')
)

Function(name='glWindowPos2sARB', enabled=True, type=Param, inheritFrom='glWindowPos2s')

Function(name='glWindowPos2sMESA', enabled=True, type=Param, inheritFrom='glWindowPos2s')

Function(name='glWindowPos2sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='2, v')
)

Function(name='glWindowPos2svARB', enabled=True, type=Param, inheritFrom='glWindowPos2sv')

Function(name='glWindowPos2svMESA', enabled=True, type=Param, inheritFrom='glWindowPos2sv')

Function(name='glWindowPos3d', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble')
)

Function(name='glWindowPos3dARB', enabled=True, type=Param, inheritFrom='glWindowPos3d')

Function(name='glWindowPos3dMESA', enabled=True, type=Param, inheritFrom='glWindowPos3d')

Function(name='glWindowPos3dv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='3, v')
)

Function(name='glWindowPos3dvARB', enabled=True, type=Param, inheritFrom='glWindowPos3dv')

Function(name='glWindowPos3dvMESA', enabled=True, type=Param, inheritFrom='glWindowPos3dv')

Function(name='glWindowPos3f', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat')
)

Function(name='glWindowPos3fARB', enabled=True, type=Param, inheritFrom='glWindowPos3f')

Function(name='glWindowPos3fMESA', enabled=True, type=Param, inheritFrom='glWindowPos3f')

Function(name='glWindowPos3fv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='3, v')
)

Function(name='glWindowPos3fvARB', enabled=True, type=Param, inheritFrom='glWindowPos3fv')

Function(name='glWindowPos3fvMESA', enabled=True, type=Param, inheritFrom='glWindowPos3fv')

Function(name='glWindowPos3i', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint')
)

Function(name='glWindowPos3iARB', enabled=True, type=Param, inheritFrom='glWindowPos3i')

Function(name='glWindowPos3iMESA', enabled=True, type=Param, inheritFrom='glWindowPos3i')

Function(name='glWindowPos3iv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='3, v')
)

Function(name='glWindowPos3ivARB', enabled=True, type=Param, inheritFrom='glWindowPos3iv')

Function(name='glWindowPos3ivMESA', enabled=True, type=Param, inheritFrom='glWindowPos3iv')

Function(name='glWindowPos3s', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort')
)

Function(name='glWindowPos3sARB', enabled=True, type=Param, inheritFrom='glWindowPos3s')

Function(name='glWindowPos3sMESA', enabled=True, type=Param, inheritFrom='glWindowPos3s')

Function(name='glWindowPos3sv', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='3, v')
)

Function(name='glWindowPos3svARB', enabled=True, type=Param, inheritFrom='glWindowPos3sv')

Function(name='glWindowPos3svMESA', enabled=True, type=Param, inheritFrom='glWindowPos3sv')

Function(name='glWindowPos4dMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLdouble'),
arg2=ArgDef(name='y', type='GLdouble'),
arg3=ArgDef(name='z', type='GLdouble'),
arg4=ArgDef(name='w', type='GLdouble')
)

Function(name='glWindowPos4dvMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLdouble*', wrapParams='4, v')
)

Function(name='glWindowPos4fMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLfloat'),
arg2=ArgDef(name='y', type='GLfloat'),
arg3=ArgDef(name='z', type='GLfloat'),
arg4=ArgDef(name='w', type='GLfloat')
)

Function(name='glWindowPos4fvMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLfloat*', wrapParams='4, v')
)

Function(name='glWindowPos4iMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLint'),
arg2=ArgDef(name='y', type='GLint'),
arg3=ArgDef(name='z', type='GLint'),
arg4=ArgDef(name='w', type='GLint')
)

Function(name='glWindowPos4ivMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLint*', wrapParams='4, v')
)

Function(name='glWindowPos4sMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='x', type='GLshort'),
arg2=ArgDef(name='y', type='GLshort'),
arg3=ArgDef(name='z', type='GLshort'),
arg4=ArgDef(name='w', type='GLshort')
)

Function(name='glWindowPos4svMESA', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='v', type='const GLshort*', wrapParams='4, v')
)

Function(name='glWindowRectanglesEXT', enabled=False, type=None,
retV=RetDef(type='void'),
arg1=ArgDef(name='mode', type='GLenum'),
arg2=ArgDef(name='count', type='GLsizei'),
arg3=ArgDef(name='box', type='const GLint*')
)

Function(name='glWriteMaskEXT', enabled=True, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='res', type='GLuint'),
arg2=ArgDef(name='in', type='GLuint'),
arg3=ArgDef(name='outX', type='GLenum'),
arg4=ArgDef(name='outY', type='GLenum'),
arg5=ArgDef(name='outZ', type='GLenum'),
arg6=ArgDef(name='outW', type='GLenum')
)
