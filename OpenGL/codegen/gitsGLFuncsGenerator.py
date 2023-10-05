#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
from generator import *
import operator

import os
import shutil

special_types = ['CIndexPtr', 'CAttribPtr', 'CGLTexResource', 'CGLCompressedTexResource']

copyright_header = """// ====================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ====================== end_copyright_notice ==============================

"""

# Header for .def files requires a different comment style.
copyright_header_def = copyright_header.replace('//', ';')


egl_function_names = '''
	eglGetError
	eglGetDisplay
	eglInitialize
	eglTerminate
	eglQueryString
	eglGetConfigs
	eglChooseConfig
	eglGetConfigAttrib
	eglCreateWindowSurface
	eglCreatePbufferSurface
	eglCreatePixmapSurface
	eglDestroySurface
	eglQuerySurface
	eglBindAPI
	eglQueryAPI
	eglWaitClient
	eglReleaseThread
	eglCreatePbufferFromClientBuffer
	eglSurfaceAttrib
	eglBindTexImage
	eglReleaseTexImage
	eglSwapInterval
	eglCreateContext
	eglDestroyContext
	eglMakeCurrent
	eglGetCurrentContext
	eglGetCurrentSurface
	eglGetCurrentDisplay
	eglQueryContext
	eglWaitGL
	eglWaitNative
	eglSwapBuffers
	eglCopyBuffers
	eglGetProcAddress
	eglClientWaitSyncKHR
	eglClientWaitSyncNV
	eglCreateDRMImageMESA
	eglCreateFenceSyncNV
	eglCreateImageKHR
	eglCreatePixmapSurfaceHI
	eglCreateStreamFromFileDescriptorKHR
	eglCreateStreamKHR
	eglCreateStreamProducerSurfaceKHR
	eglCreateSyncKHR
	eglDestroyImageKHR
	eglDestroyStreamKHR
	eglDestroySyncKHR
	eglDestroySyncNV
	eglExportDRMImageMESA
	eglFenceNV
	eglGetStreamFileDescriptorKHR
	eglGetSyncAttribKHR
	eglGetSyncAttribNV
;	eglGetSystemTimeFrequencyNV
;	eglGetSystemTimeNV
	eglLockSurfaceKHR
	eglPostSubBufferNV
	eglQueryStreamKHR
	eglQueryStreamTimeKHR
	eglQueryStreamu64KHR
	eglQuerySurfacePointerANGLE
	eglSignalSyncKHR
	eglSignalSyncNV
	eglStreamAttribKHR
	eglStreamConsumerAcquireKHR
	eglStreamConsumerGLTextureExternalKHR
	eglStreamConsumerReleaseKHR
	eglUnlockSurfaceKHR
	eglWaitSyncKHR
	eglSetSwapRectangleANDROID
	eglGetRenderBufferANDROID
	eglDupNativeFenceFDANDROID
	eglWaitSyncANDROID
'''.strip('\r\n')

wgl_function_names = '''
	wglCopyContext
	wglCreateContext
	wglCreateLayerContext
	wglDeleteContext
	wglGetCurrentContext
	wglGetCurrentDC
	wglGetDefaultProcAddress
	wglGetPixelFormat
	wglGetProcAddress
	wglMakeCurrent
	wglRealizeLayerPalette
	wglShareLists
	wglSwapBuffers
	wglSwapMultipleBuffers
	wglSwapLayerBuffers
	wglUseFontBitmapsA
	wglUseFontBitmapsW
	wglChoosePixelFormat
	wglDescribeLayerPlane
	wglDescribePixelFormat
	wglGetLayerPaletteEntries
	wglSetLayerPaletteEntries
	wglSetPixelFormat
	wglUseFontOutlinesA
	wglUseFontOutlinesW
	wglCreateBufferRegionARB
	wglDeleteBufferRegionARB
	wglSaveBufferRegionARB
	wglRestoreBufferRegionARB
	wglGetExtensionsStringARB
	wglGetPixelFormatAttribivARB
	wglGetPixelFormatAttribfvARB
	wglChoosePixelFormatARB
	wglMakeContextCurrentARB
	wglGetCurrentReadDCARB
	wglCreatePbufferARB
	wglGetPbufferDCARB
	wglReleasePbufferDCARB
	wglDestroyPbufferARB
	wglQueryPbufferARB
	wglBindTexImageARB
	wglReleaseTexImageARB
	wglSetPbufferAttribARB
	wglCreateContextAttribsARB
	wglCreateDisplayColorTableEXT
	wglLoadDisplayColorTableEXT
	wglBindDisplayColorTableEXT
	wglDestroyDisplayColorTableEXT
	wglGetExtensionsStringEXT
	wglMakeContextCurrentEXT
	wglGetCurrentReadDCEXT
	wglCreatePbufferEXT
	wglGetPbufferDCEXT
	wglReleasePbufferDCEXT
	wglDestroyPbufferEXT
	wglQueryPbufferEXT
	wglGetPixelFormatAttribivEXT
	wglGetPixelFormatAttribfvEXT
	wglChoosePixelFormatEXT
	wglSwapIntervalEXT
	wglGetSwapIntervalEXT
	wglAllocateMemoryNV
	wglFreeMemoryNV
	wglGetSyncValuesOML
	wglGetMscRateOML
	wglSwapBuffersMscOML
	wglSwapLayerBuffersMscOML
	wglWaitForMscOML
	wglWaitForSbcOML
	wglGetDigitalVideoParametersI3D
	wglSetDigitalVideoParametersI3D
	wglGetGammaTableParametersI3D
	wglSetGammaTableParametersI3D
	wglGetGammaTableI3D
	wglSetGammaTableI3D
	wglEnableGenlockI3D
	wglDisableGenlockI3D
	wglIsEnabledGenlockI3D
	wglGenlockSourceI3D
	wglGetGenlockSourceI3D
	wglGenlockSourceEdgeI3D
	wglGetGenlockSourceEdgeI3D
	wglGenlockSampleRateI3D
	wglGetGenlockSampleRateI3D
	wglGenlockSourceDelayI3D
	wglGetGenlockSourceDelayI3D
	wglQueryGenlockMaxSourceDelayI3D
	wglCreateImageBufferI3D
	wglDestroyImageBufferI3D
	wglAssociateImageBufferEventsI3D
	wglReleaseImageBufferEventsI3D
	wglEnableFrameLockI3D
	wglDisableFrameLockI3D
	wglIsEnabledFrameLockI3D
	wglQueryFrameLockMasterI3D
	wglGetFrameUsageI3D
	wglBeginFrameTrackingI3D
	wglEndFrameTrackingI3D
	wglQueryFrameTrackingI3D
	wglSetStereoEmitterState3DL
	wglEnumerateVideoDevicesNV
	wglBindVideoDeviceNV
	wglQueryCurrentContextNV
	wglGetVideoDeviceNV
	wglReleaseVideoDeviceNV
	wglBindVideoImageNV
	wglReleaseVideoImageNV
	wglSendPbufferToVideoNV
	wglGetVideoInfoNV
	wglJoinSwapGroupNV
	wglBindSwapBarrierNV
	wglQuerySwapGroupNV
	wglQueryMaxSwapGroupsNV
	wglQueryFrameCountNV
	wglResetFrameCountNV
	wglEnumGpusNV
	wglEnumGpuDevicesNV
	wglCreateAffinityDCNV
	wglEnumGpusFromAffinityDCNV
	wglDeleteDCNV
;	wglGetGPUIDsAMD
	wglGetGPUInfoAMD
	wglGetContextGPUIDAMD
	wglCreateAssociatedContextAMD
	wglCreateAssociatedContextAttribsAMD
	wglDeleteAssociatedContextAMD
	wglMakeAssociatedContextCurrentAMD
	wglGetCurrentAssociatedContextAMD
	wglBlitContextFramebufferAMD
	wglBindVideoCaptureDeviceNV
	wglEnumerateVideoCaptureDevicesNV
	wglLockVideoCaptureDeviceNV
	wglQueryVideoCaptureDeviceNV
	wglReleaseVideoCaptureDeviceNV
	wglCopyImageSubDataNV
'''.strip('\r\n')


def make_id(name, version):
  id_ = re.sub('([a-z])([A-Z])', '\g<1>_\g<2>', name)
  id_ = re.sub('([0-9])D', '_\g<1>D_', id_)

  id_final = 'ID_' + id_.upper().strip('_')
  if (version > 0):
    id_final = id_final + '_V' + str(version)
  return id_final


def make_type(fdata):
  type = ""
  if fdata['functionType'] & Bind:
    type += "GITS_GL_BIND_APITYPE | "
  if fdata['functionType'] & Create:
    type += "GITS_GL_CREATE_APITYPE | "
  if fdata['functionType'] & Delete:
    type += "GITS_GL_DELETE_APITYPE | "
  if fdata['functionType'] & Fill:
    type += "GITS_GL_FILL_APITYPE | "
  if fdata['functionType'] & Gen:
    type += "GITS_GL_GEN_APITYPE | "
  if fdata['functionType'] & Get:
    type += "GITS_GL_GET_APITYPE | "
  if fdata['functionType'] & Map:
    type += "GITS_GL_MAP_APITYPE | "
  if fdata['functionType'] & Param:
    type += "GITS_GL_PARAM_APITYPE | "
  if fdata['functionType'] & Render:
    type += "GITS_GL_RENDER_APITYPE | "
  if fdata['functionType'] & Resource:
    type += "GITS_GL_RESOURCE_APITYPE | "
  if fdata['functionType'] & Query:
    type += "GITS_GL_QUERY_APITYPE | "
  if fdata['functionType'] & Copy:
    type += "GITS_GL_COPY_APITYPE | "
  if fdata['functionType'] & Exec:
    type += "GITS_GL_EXEC_APITYPE | "
  if fdata['functionType'] & GetEssential:
    type += "GITS_GL_GET_ESSENTIAL_APITYPE | "
  type = type.strip(" | ")
  return type


def arg_decl(fdata, name=False, gldrivers=False, mode='default'):
  args = fdata['args']
  content = '('
  if (fdata['type'] != 'void') and not gldrivers:
    content += fdata['type'] + ' return_value, '
  if mode == 'default':
    for arg in args:
      content += arg['type']
      if name:
        content += ' ' + arg['name']
      content += ', '
  elif mode in ('CAttribPtr', 'CIndexPtr'):
    for arg in args:
      content += arg['type']
      if name:
        content += ' ' + arg['name']
      content += ', '
    content += 'GLint boundBuffer, '
  elif mode in ('CGLTexResource', 'CGLCompressedTexResource'):
    for arg in args:
      if arg.get('wrapType') in ('CGLTexResource', 'CGLCompressedTexResource'):
        content += 'hash_t hash, '
      else:
        content += arg['type']
        if name:
          content += ' ' + arg['name']
        content += ', '
  content = content.strip(', ')
  content += ')'
  return content


def arg_call(fdata, recording=False, gldrivers=False, wrap=False):
  args = fdata['args']
  content = '('
  if (fdata['type'] != 'void') and not gldrivers:
    content += 'return_value, '
  for arg in args:
    name = arg['name']
    # remove brackets (for example baseAndCount [2]) from attribute name in a function call
    for brackets in re.finditer(re.compile("\[.+?\]", re.DOTALL), name):
      name = name.replace(brackets.group(0), "")
    content += name + ', '
  if recording:
    content += 'Recording(_recorder), '
  if wrap:
    content += '_recorder, '
  content = content.strip(', ')
  content += ')'
  return content


def make_drivers(functions):
  glDrivers_h = open('glDrivers.h', 'w')
  glDrivers_h.write(copyright_header)
  content = '#define DRAW_FUNCTIONS(a) \\\n'
  for key in sorted(functions.keys()):
    value = functions[key][-1]
    type = ""
    if str(value['type']) == 'void':
      type = 'void_t'
    else:
      type = value['type']
    if value.get('functionType'):
      if (value.get('functionType') & Render) or (value.get('functionType') & Fill) or (value.get('functionType') & Copy):
        content += '  DRAW_FUNCTION(a, ' + type + ', '
        content += key + ', ' + arg_decl(value, True, True) + ', ' + arg_call(value, False, True) + ') \\\n'
  content += "\n"
  glDrivers_h.write(content)
  content = '#define GL_FUNCTIONS(a) \\\n'
  for key in sorted(functions.keys()):
    value = functions[key][-1]
    type = ""
    if value['type'] == 'void':
      type = 'void_t'
    else:
      type = value['type']
    if value.get('functionType'):
      if (value.get('functionType') & Render == 0) and (value.get('functionType') & Fill == 0) and (value.get('functionType') & Copy == 0):
        content += '  GL_FUNCTION(a, ' + type + ', '
        content += key + ', ' + arg_decl(value, True, True) + ', ' + arg_call(value, False, True) + ') \\\n'
    else:
      content += '  GL_FUNCTION(a, ' + type + ', '
      content += key + ', ' + arg_decl(value, True, True) + ', ' + arg_call(value, False, True) + ') \\\n'
  content += '\n'
  glDrivers_h.write(content)


def generate_gits_wrapper(functions):
  wrap_h = open('openglRecorderWrapperAuto.h', 'w')
  wrap_c = open('openglRecorderWrapperAuto.cpp', 'w')
  wrap_i = open('openglRecorderWrapperIfaceAuto.h', 'w')
  wrap_h.write(copyright_header)
  wrap_c.write(copyright_header)
  wrap_i.write(copyright_header)

  include_c = """
#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "openglRecorderWrapper.h"
#include "openglLibraryRecorder.h"
#include "recorder.h"
#include "gits.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "exception.h"
#include "log.h"
#include "config.h"
#include "tools.h"

#include "gitsFunctions.h"

#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"
#include "glFunctions.h"
#include "openglCommon.h"
#include "openglTools.h"
#include "stateTracking.h"
#include "openglState.h"
#include "openglRecorderConditions.h"
#include "openglRecorderSubWrappers.h"
#include "openglRecorderPreSchedule.h"

#include <string>
#include <sstream>
#include <algorithm>

DISABLE_WARNINGS
#include <boost/lexical_cast.hpp>
ENABLE_WARNINGS

#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <GL/glx.h>
#undef True
#undef XVisualInfo
#endif

namespace gits {
namespace OpenGL {
"""

  end_c = """
} // namespace OpenGL
} // namespace gits
"""
  wrap_c.write(include_c)
  for key in sorted(functions.keys()):
    value = functions[key][-1]
    if value.get('custom') is not True:
      if value.get('enabled') is True:
        wrapper_pre_post = ""
        if value.get('functionType'):
          if (value.get('functionType') & Render) or (value.get('functionType') & Fill) or (value.get('functionType') & Copy):
            wrapper_pre_post = "DRAWCALL_WRAPPER_PRE_POST\n  "
        pre_token = ""
        if value.get('preToken') is not None and (value.get('recWrap') is not True) and value.get('preToken') is not False:
          pre_token = "_recorder.Schedule(new %(name)s);\n    " % {'name': value.get('preToken')}
        pre_schedule = ""
        if value.get('preSchedule') is not None and (value.get('recWrap') is not True) and value.get('preSchedule') is not False:
          pre_schedule = "%(name)s;\n    " % {'name': value.get('preSchedule')}
        post_token = ""
        if value.get('postToken') is not None and (value.get('recWrap') is not True):
          post_token = "\n    _recorder.Schedule(new %(name)s);\n" % {'name': value.get('postToken')}
        rec_cond = ""
        if value.get('recCond') and (value.get('recWrap') is not True):
          rec_cond = "\n  if(%(recCond)s)\n  {" % {'recCond': value.get('recCond')}
        elif (value.get('recCond') is not True) and (value.get('recWrap') is not True):
          rec_cond = "\n  if(Recording(_recorder))\n  {"
        state_track = ""
        if value.get('stateTrack') is True and (value.get('recWrap') is not True):
          state_track = "\n  }\n  %(name)s_SD%(argsCall)s;" % {'name': value.get('stateTrackName'), 'argsCall': arg_call(value, True)}
        elif value.get('stateTrack') is not True and (value.get('recWrap') is not True):
          state_track = "\n  }"
        rec_wrap = ""
        if (value.get('recWrap') is True):
          rec_wrap = "  %(name)s_RECWRAP%(argsCall)s;" % {'name': value.get('recWrapName'), 'argsCall': arg_call(value, False, False, True)}
        schedule = ""
        key_name = key
        if (value.get('version') > 0):
          key_name = key + '_V' + str(value.get('version'))

        if (value.get('recWrap') is not True):
          schedule = "_recorder.Schedule(new C%(name)s%(arg_call)s);" % {'name': key_name, 'arg_call': arg_call(value)}
        wc = """
void CRecorderWrapper::%(name)s%(arg_decl)s const
{
  GITS_REC_ENTRY_GL
  %(wrapper_pre_post)s%(rec_cond)s%(rec_wrap)s
    %(pre_schedule)s%(pre_token)s%(schedule)s%(post_token)s%(state_track)s
}""" % {'name': key, 'arg_decl': arg_decl(value, True, False), 'pre_schedule': pre_schedule, 'pre_token': pre_token, 'post_token': post_token, 'rec_cond': rec_cond, 'state_track': state_track, 'wrapper_pre_post': wrapper_pre_post, 'rec_wrap': rec_wrap, 'schedule': schedule}
        wrap_c.write(wc + '\n')
        wrap_h.write('void ' + key + arg_decl(value, True, False) + ' const override;\n')
        wrap_i.write('virtual void ' + key + arg_decl(value, True, False) + ' const = 0;\n')
      else:
        wc = "\nvoid CRecorderWrapper::%(name)s%(arg_decl)s const\n{\n" % {'name': key, 'arg_decl': arg_decl(value, True, False)}
        if (value.get('recWrap') is True):
          wc = wc + "  GITS_REC_ENTRY_GL"
          wc = wc + """
  if(Recording(_recorder))
  {
    %(name)s_RECWRAP%(argsCall)s;
  }""" % {'name': value.get('recWrapName'), 'argsCall': arg_call(value, False, False, True)}
        else:
          wc = wc + "  CALL_ONCE [] { Log(WARN) << \"function " + key + " not implemented\"; };"

        wrap_c.write(wc + '\n}\n')
        wrap_h.write('void ' + key + arg_decl(value, True, False) + ' const override;\n')
        wrap_i.write('virtual void ' + key + arg_decl(value, True, False) + ' const = 0;\n')
  wrap_c.write(end_c)


def generate_tokens(functions):
  tokens_h = open('glFunctions.h', 'w')
  tokens_c = open('glFunctions.cpp', 'w')
  tokens_h.write(copyright_header)
  tokens_c.write(copyright_header)

  token_c_include = """
#include "glFunctions.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "streams.h"
#include "platform.h"
#include "openglTools.h"
#include "tools.h"
#include "stateTracking.h"
#include "playerRunWrap.h"
#include "playerRunWrapConditions.h"

DISABLE_WARNINGS
#include <boost/lexical_cast.hpp>
ENABLE_WARNINGS

#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

"""
  token_h_include = """
#pragma once

#include "gits.h"
#include "log.h"
#include "openglTypes.h"
#include "openglFunction.h"
#include "openglArguments.h"
#include "config.h"
#include "wglArguments.h"
#include "wglFunctions.h"
#include "platform.h"
#include "mapping.h"
#include "clientArrays.h"
#include "stateTracking.h"

#include "openglLibrary.h"
#include "stateDynamic.h"

namespace gits {
  namespace OpenGL {
"""
  token_h_end = """
  } // namespace OpenGL
} // namespace gits
"""
  tokens_c.write(token_c_include)
  tokens_h.write(token_h_include)

  for key in sorted(functions.keys()):
    for elem in functions[key]:
      Ctypes = []
      Cnames = []
      Cwraps = []
      counter = 0
      special_token = False
      special_token_type = ''
      key_name = key
      if (elem.get('version') > 0):
        key_name = key + '_V' + str(elem.get('version'))

      if elem['custom'] is not True:
        if (elem['type'] != 'void'):
          Cnames.append('_return_value')
          typename = elem['type']
          typename = typename.replace('const ', '')
          if elem.get('retVwrapType'):
            Ctypes.append(elem.get('retVwrapType'))
          elif '*' in typename:
            typename = typename.strip(' *')
            if typename in ('void', 'GLvoid'):
              Ctypes.append('CGLvoid_ptr')
            else:
              Ctypes.append('C' + typename + '::CSArray')
          else:
            Ctypes.append('C' + typename)
          if (elem.get('retVwrapParams')):
            Cwraps.append(elem.get('retVwrapParams'))
          else:
            Cwraps.append('')
        remove_mapping = ""
        remove_mapping_constructor = ""
        for arg in elem['args']:
          Cnames.append('_' + arg['name'])
          typename = arg['type']#.strip(' *')
          typename = typename.replace('const ', '')
          if arg.get('removeMapping') is True:
            remove_mapping_constructor += "\n  %(name)s.RemoveMapping();" % {'name': '_' + arg['name']}
            if elem.get('runWrap') is not True:
              remove_mapping += "\n  %(name)s.RemoveMapping();" % {'name': '_' + arg['name']}
          if arg.get('wrapType'):
            Ctypes.append(arg['wrapType'])
            if arg.get('wrapType') in special_types:
              special_token = True
              special_token_type = arg.get('wrapType')
          elif '*' in typename:
            typename = typename.strip(' *')
            Ctypes.append('C' + typename + '::CSArray')
          else:
            Ctypes.append('C' + typename)
          if arg.get('wrapParams'):
            Cwraps.append(arg['wrapParams'])
          else:
            Cwraps.append('')
          counter += 1

        func = elem
        argd = arg_decl(elem, True, False)
        special_token_decl = ''
        init_special = ''
        special_token_impl = ''
        if special_token:
          argd_special = arg_decl(elem, True, False, special_token_type)
          special_token_decl = "\n      C%(name)s%(argd_special)s;" % {'name': key_name, 'argd_special': argd_special}
          init_special = ":\n"
          if special_token_type in ('CGLTexResource', 'CGLCompressedTexResource'):
            for n, w, t in zip(Cnames, Cwraps, Ctypes):
              if t != 'COutArgument':
                if t in special_types:
                  init_special += n + '(hash), '
                else:
                  if w == '':
                    init_special += n + '(' + n.strip('_') + '), '
                  else:
                    init_special += n + '(' + w + '), '
            init_special = init_special.strip(', ')
          elif special_token_type in ('CAttribPtr', 'CIndexPtr'):
            for n, w, t in zip(Cnames, Cwraps, Ctypes):
              if t != 'COutArgument':
                if t in special_types:
                  if w == '':
                    init_special += n + '(boundBuffer, ' + n.strip('_') + '), '
                  else:
                    init_special += n + '(boundBuffer, ' + w+'), '
                else:
                  if w == '':
                    init_special += n + '(' + n.strip('_') + '), '
                  else:
                    init_special += n + '(' + w + '), '
            init_special = init_special.strip(', ')
          special_token_impl = """
gits::OpenGL::C%(name)s::C%(name)s%(argd_special)s%(init_special)s
{
}
""" % {'name': key_name, 'argd_special': argd_special, 'init_special': init_special}

        # instantiate COutArgument template for multiple output arguments
        outArgIndex = 0
        CtypesInst = []
        for t in Ctypes:
          if t == 'COutArgument':
            CtypesInst.append(t + '<' + str(outArgIndex) + '>')
            outArgIndex += 1
          else:
            CtypesInst.append(t)

        argsDecl = ""
        for n, t in zip(Cnames, CtypesInst):
          argsDecl += '      ' + t + ' ' + n + ';\n'

        argsCall = ""
        wrapCall = ""
        stateTrackCall = ""
        for n in Cnames:
          if n != '_return_value':
            argsCall += '*' + n + ', '
          wrapCall += n + ', '
          stateTrackCall += '*' + n + ', '
        argsCall = argsCall.strip(', ')
        wrapCall = wrapCall.strip(', ')
        stateTrackCall = stateTrackCall.strip(', ')

        inherit_type = ''
        run_name = ''
        if (func['functionType'] & Render) or (func['functionType'] & Fill) or (func['functionType'] & Copy):
          inherit_type = 'CDrawFunction'
          run_name = 'RunImpl'
        else:
          inherit_type = 'CFunction'
          run_name = 'Run'

        suffix = ''
        if func.get('ccodeWrap') is True:
          suffix = '\n      virtual const char* Suffix() const { return "_wrap"; }'

        write_wrap_decl = ''
        write_wrap_def = ''
        if func.get('ccodeWriteWrap') is True:
          # We use this when ccodeWraps aren't sufficient, e.g. because the
          # arguments of our *_wrap differ from arguments of the apicall.
          write_wrap_decl += '\n      virtual void Write(CCodeOStream& stream) const override;'
          write_wrap_decl += '\n      friend void C' + key_name + '_CCODEWRITEWRAP(CCodeOStream& stream, const C' + key_name + '& function);'
          write_wrap_def += '\n\nvoid gits::OpenGL::C' + key_name + '::Write(CCodeOStream& stream) const {'
          write_wrap_def += '\n  stream.select(stream.selectCCodeFile());'
          write_wrap_def += '\n  C' + key_name + '_CCODEWRITEWRAP(stream, *this);'
          write_wrap_def += '\n}'
          if func.get('ccodeWrap') is True:
            raise RuntimeError("If ccodeWriteWrap is enabled, ccodeWrap does "
                               "not do anything. Having both enabled "
                               "indicates a logic error.")

        arg_ref = ""
        if func.get('type') != 'void':
          arg_ref = "      virtual MaybeConstCArgRef Return() const { return _return_value; }\n"

        c = ""
        if len(func['args']) > 0 or func['type'] != 'void':
          c = """
    class C%(name)s : public %(inherit_type)s {
%(argsDecl)s
      virtual CArgument &Argument(unsigned idx);
      virtual unsigned ArgumentCount() const { return %(argc)s; }
%(arg_ref)s
    public:
      C%(name)s();
      C%(name)s%(argd)s;%(special_token_decl)s
      virtual unsigned Id() const { return %(id)s; }
      virtual unsigned Type() const { return %(type)s;}
      virtual const char* Name() const { return "%(func_name)s"; }%(suffix)s
      virtual void %(run_name)s();%(write_wrap_decl)s
      };""" % {'name': key_name, 'id': make_id(key, func['version']), 'argc': len(func['args']), 'argd': argd, 'argsDecl': argsDecl, 'inherit_type': inherit_type, 'run_name': run_name, 'write_wrap_decl': write_wrap_decl, 'special_token_decl': special_token_decl, 'suffix': suffix, 'arg_ref': arg_ref, 'type': make_type(func), 'func_name': key}
        else:
          c = """
    class C%(name)s : public %(inherit_type)s {
%(argsDecl)s
      virtual CArgument &Argument(unsigned idx);
      virtual unsigned ArgumentCount() const { return %(argc)s; }

    public:
      C%(name)s();
      virtual unsigned Id() const { return %(id)s; }
      virtual unsigned Type() const { return %(type)s;}
      virtual const char* Name() const { return "%(func_name)s"; }%(suffix)s
      virtual void %(run_name)s();%(write_wrap_decl)s
      };""" % {'name': key_name, 'id': make_id(key, func['version']), 'argc': len(func['args']), 'argsDecl': argsDecl, 'inherit_type': inherit_type, 'run_name': run_name, 'write_wrap_decl': write_wrap_decl, 'suffix': suffix, 'type': make_type(func), 'func_name': key}
        tokens_h.write(c + '\n')

        cargument = 'return get_cargument(__FUNCTION__, idx, '
        count = 0
        for arg in Cnames:
          if arg != '_return_value':
            cargument += arg + ', '
            count = count + 1
        cargument = cargument.strip(', ')
        cargument += ');'
        if count == 0:
          cargument = 'report_cargument_error(__FUNCTION__, idx);'
        init = ''
        if len(func['args']) > 0 or func['type'] != 'void':
          init = ':\n  '
        counter = 0
        for n, w, t in zip(Cnames, Cwraps, Ctypes):
          if t != 'COutArgument':
            counter += 1
            if w == '':
              init += n + '(' + n.strip('_') + '), '
            else:
              init += n + '(' + w + '), '
        if counter > 0:
          init = init.strip(', ')
        else:
          init = ''

        if func.get('runWrap') is True:
          runWrapName = func.get('runWrapName')

          if elem.get('version') > 0:
            runWrapName = str(func.get('runWrapName')) + '_V' + str(elem.get('version'))
          if func.get('passToken') is True:
            gl_cmd = "%(name)s_WRAPRUN(this, %(wrapCall)s)" % {'name': runWrapName,  'wrapCall': wrapCall}
          else:
            gl_cmd = "%(name)s_WRAPRUN(%(wrapCall)s)" % {'name': runWrapName,  'wrapCall': wrapCall}
        else:
          gl_cmd = "drv.gl.%(name)s(%(argsCall)s)" % {'name': key,  'argsCall': argsCall}
        state_track = ""
        if func.get('stateTrack') is True and (func.get('runWrap') is not True):
          state_track = "\n  %(name)s_SD(%(argsCall)s);" % {'name': func.get('stateTrackName'), 'argsCall': stateTrackCall}
        return_value = ""
        return_value_end = ";"
        if func.get('type') != 'void' and (func.get('runWrap') is not True):
          return_value = "_return_value.Assign("
          return_value_end = ");"
        if func.get('runCond') and (func.get('runWrap') is not True):
           if remove_mapping != '':
             remove_mapping = "\n  if (" + func.get('runCond') + " )\n  {" + remove_mapping + "\n  }"
           run = """if (%(func_name)s)
    %(return_value)s%(gl_cmd)s%(return_value_end)s%(state_track)s%(remove_mapping)s""" % {'func_name': func.get('runCond'), 'gl_cmd': gl_cmd, 'state_track': state_track, 'return_value': return_value, 'return_value_end': return_value_end, 'remove_mapping': remove_mapping}
        else:
           run = """%(return_value)s%(gl_cmd)s%(return_value_end)s%(state_track)s%(remove_mapping)s""" % {'gl_cmd': gl_cmd, 'state_track': state_track, 'return_value': return_value, 'return_value_end': return_value_end, 'remove_mapping': remove_mapping}
        c = ""
        if len(func['args']) > 0 or func['type'] != 'void':
          c = """
/* ***************************** %(id)s *************************** */


gits::OpenGL::C%(name)s::C%(name)s()
{
}


gits::OpenGL::C%(name)s::C%(name)s%(argd)s%(init)s
{%(remove_mapping_constructor)s
}
%(special_token_impl)s

gits::CArgument &gits::OpenGL::C%(name)s::Argument(unsigned idx)
{
  %(cargument)s
}


void gits::OpenGL::C%(name)s::%(run_name)s()
{
  %(run)s
}%(write_wrap_def)s""" % {'id': make_id(key, func['version']), 'name': key_name, 'cargument': cargument, 'argd': argd, 'init': init, 'run': run, 'run_name': run_name, 'write_wrap_def': write_wrap_def, 'special_token_impl': special_token_impl, 'remove_mapping_constructor': remove_mapping_constructor}
        else:
          c = """
/* ***************************** %(id)s *************************** */


gits::OpenGL::C%(name)s::C%(name)s()
{
}

gits::CArgument &gits::OpenGL::C%(name)s::Argument(unsigned idx)
{
  %(cargument)s
}


void gits::OpenGL::C%(name)s::%(run_name)s()
{
  %(run)s
}%(write_wrap_def)s""" % {'id': make_id(key, func['version']), 'name': key_name, 'cargument': cargument,  'run': run, 'run_name': run_name, 'write_wrap_def': write_wrap_def}
        tokens_c.write(c + '\n')
  tokens_h.write(token_h_end)


def make_c_decl(fname, fdata):
  content = 'typedef ' + fdata['type'] + ' (*' + fname + '_type)' + arg_decl(fdata) + ';'
  return content


def generate_ptr_decls():
  for key in sorted(functions.keys()):
    print(make_c_decl(key, functions[key]))


def generate_create_switch(functions):
  glfunction_cpp = open('glIDswitch.h', 'w')
  glfunction_cpp.write(copyright_header)
  output = ""
  for key in sorted(functions.keys()):
    for elem in functions[key]:
      output += 'case ' + make_id(key, elem['version']) + ':\n'
      if elem['version'] == 0:
        output += '  return new C' + key + ';\n'
      else:
        output += '  return new C' + key + '_V' + str(elem['version']) + ';\n'
  glfunction_cpp.write(output)
  glfunction_cpp.close()


def generate_openglFunction_header(functions):
  gl_functions_generated = open('glIDs.h', 'r')
  functions_ids = []
  try:
    for line in gl_functions_generated:
      elem = line.strip(",\n")
      if 'ID' in elem:
        functions_ids.append(elem)
  finally:
    gl_functions_generated.close()

  glfunction_h = open('glIDs.h', 'a')
  generated_id = ""
  for key in sorted(functions.keys()):
    for func in functions[key]:
      if make_id(key, func['version']) not in functions_ids:
        generated_id += make_id(key, func['version']) + ',\n'

  glfunction_h.write(generated_id)
  glfunction_h.close()


def generate_prepost(functions):
  pre_post_auto = open('gitsPluginPrePostAuto.cpp', 'w')
  pre_post_auto.write(copyright_header)
  pre_post_auto.write("""
/**
 * @file   gitsPluginPrePostAuto.cpp
 *
 * @brief
 */

#include "openglInterceptorExecOverride.h"

extern "C" {
""")

  for key in sorted(functions.keys()):
    value = functions[key][-1]
    pre_ret = ''
    post_ret = ''
    success_value = '('
    arguments = arg_call(value, False, True).replace('(', '')
    if value['type'] != 'void':
      pre_ret = 'auto return_value = '
      post_ret = '\n  return return_value;'
      success_value = '(return_value'

    prefix = ''
    if value.get('prefix') is not None:
      prefix = '\n' + value.get('prefix')

    if value.get('interceptorExecOverride') is True:
      drv_call_normal = """\n    %(pre_ret)sexecWrap_%(name)s(%(arguments)s;""" % {'pre_ret': pre_ret,
                                                                                   'name': key,
                                                                                   'arguments': arguments}
    else:
      drv_call_normal = """\n    %(pre_ret)swrapper.Drivers().gl.%(name)s(%(arguments)s;""" % {'pre_ret': pre_ret,
                                                                                               'name': key,
                                                                                               'arguments': arguments}

    drv_call_late = ''
    if value.get('execPostRecWrap') is True:
      drv_call_late = drv_call_normal
      drv_call_normal = ''
      if len(success_value) > 1:
        success_value = '(true'
    if len(success_value) > 1 and len(arguments) > 1:
      success_value = success_value + ', '

    wrapper = """wrapper.%(name)s%(success_value)s%(arguments)s;""" % {'name': key,
                                                                       'success_value': success_value,
                                                                       'arguments': arguments}

    suffix = ''
    if value.get('suffix') is not None:
      suffix = '  ' + value.get('suffix') + '\n'

    pre_post_auto.write("""
GLAPI %(type)s GLAPIENTRY %(name)s%(arg_decl)s
{%(prefix)s
  GITS_ENTRY_GL%(drv_call_normal)s
  GITS_WRAPPER_PRE
    %(wrapper)s
  GITS_WRAPPER_POST%(drv_call_late)s%(post_ret)s
%(suffix)s}
""" % {'name': key,
       'type': value['type'],
       'arg_decl': arg_decl(value, True, True),
       'prefix': prefix,
       'drv_call_normal': drv_call_normal,
       'wrapper': wrapper,
       'drv_call_late': drv_call_late,
       'post_ret': post_ret,
       'suffix': suffix})

  pre_post_auto.write("\n}\n")


def generate_prepost_h(functions):
  pre_post_auto_h = open('gitsPluginPrePostAuto.h', 'w')
  pre_post_auto_h.write(copyright_header)
  pre_post_auto_h.write("""
/**
 * @file   gitsPluginPrePostAuto.h
 *
 * @brief
 */

#include "platform.h"
#include "openglTypes.h"

#if defined GITS_PLATFORM_WINDOWS
#define GLAPI
#else
#define GLAPI __attribute__((visibility("default")))
#endif
#define GLAPIENTRY STDCALL

extern "C" {
""")

  for key in sorted(functions.keys()):
    value = functions[key][-1]

    pre_post_auto_h.write("""
GLAPI %(type)s GLAPIENTRY %(name)s%(arg_decl)s;""" % {'type': value['type'],
                                                      'name': key,
                                                      'arg_decl': arg_decl(value, True, True)})

  pre_post_auto_h.write("\n}\n")


def generate_gliplugin_def(functions):
  str_replacements = {'copyright_header_def': copyright_header_def,
                      'egl_function_names': egl_function_names,
                      'wgl_function_names': wgl_function_names}
  gliplugin_def_64 = open('GLIPlugin64.def', 'w')  # No EGL on 64-bit Windows.
  gliplugin_def_32 = open('GLIPlugin32.def', 'w')  # 32-bit DLL exports EGL.
  gliplugin_def_64.write("""{copyright_header_def}

LIBRARY OpenGL32.dll
EXPORTS
	GITSIdentificationToken
; WGL
{wgl_function_names}
; GL
""".format(**str_replacements))

  gliplugin_def_32.write("""{copyright_header_def}

LIBRARY OpenGL32.dll
EXPORTS
	GITSIdentificationToken
; EGL
{egl_function_names}
; WGL
{wgl_function_names}
; GL
""".format(**str_replacements))
  for key in sorted(functions.keys()):
    gliplugin_def_64.write("\t" + key + "\n")
    gliplugin_def_32.write("\t" + key + "\n")


table = GetFunctions()
try:
  black = open('blacklist.txt').readlines()
  black = list(map(lambda a: a.strip('\n'), black))
  print('Using blacklist file')
except:
  pass
#output_tokens = open('outputTokens.cpp', 'w')
functions_all = {}
functions_enabled = {}
#output_tokens_library = open('outputTokens.h', 'w')
content = ''
for f in table:
  function = {}
  function['args'] = []
  function['type'] = ''
  function['functionType'] = None
  function['enabled'] = f.get('enabled')
  function['custom'] = False
  function['version'] = 0
  if f.get('inheritFrom'):
    inherit_name = f.get('inheritFrom')
    function['inheritName'] = inherit_name
    for g in table:
      if g.get('name') == inherit_name:
        if g.get('retV'):
          function['type'] = g.get('retV').get('type')
          if g.get('retV').get('wrapType'):
            function['retVwrapType'] = g.get('retV').get('wrapType')
          if g.get('retV').get('wrapParams'):
            function['retVwrapParams'] = g.get('retV').get('wrapParams')
        if g.get('type'):
          function['functionType'] = g.get('type')
        if g.get('custom'):
          function['custom'] = g.get('custom')
        if g.get('version'):
          function['version'] = g.get('version')
        i = 1
        while g.get('arg'+str(i)):
          arg = {}
          arg['type'] = g.get('arg'+str(i)).get('type')
          arg['name'] = g.get('arg'+str(i)).get('name')
          if g.get('arg'+str(i)).get('wrapType'):
            arg['wrapType'] = g.get('arg'+str(i)).get('wrapType')
          if g.get('arg'+str(i)).get('wrapParams'):
            arg['wrapParams'] = g.get('arg'+str(i)).get('wrapParams')
          if g.get('arg'+str(i)).get('removeMapping'):
            arg['removeMapping'] = g.get('arg'+str(i)).get('removeMapping')
          function['args'].append(arg)
          i += 1
        if g.get('preToken') is not None:
          function['preToken'] = g.get('preToken')
          function['preTokenName'] = inherit_name
        if g.get('postToken') is not None:
          function['postToken'] = g.get('postToken')
          function['postTokenName'] = inherit_name
        if g.get('stateTrack') is not None:
          function['stateTrack'] = g.get('stateTrack')
          function['stateTrackName'] = inherit_name
        if g.get('recCond') is not None:
          function['recCond'] = g.get('recCond')
        if g.get('runCond') is not None:
          function['runCond'] = g.get('runCond')
        if g.get('preSchedule') is not None:
          function['preSchedule'] = g.get('preSchedule')
        if g.get('recWrap') is not None:
          function['recWrap'] = g.get('recWrap')
          function['recWrapName'] = inherit_name
        if g.get('runWrap') is not None:
          function['runWrap'] = g.get('runWrap')
          function['runWrapName'] = inherit_name
        if g.get('ccodeWrap') is not None:
          function['ccodeWrap'] = g.get('ccodeWrap')
          function['ccodeWrapName'] = inherit_name
        if g.get('ccodeWriteWrap') is not None:
          function['ccodeWriteWrap'] = g.get('ccodeWriteWrap')
          function['ccodeWriteWrapName'] = inherit_name
        if g.get('execPostRecWrap') is not None:
          function['execPostRecWrap'] = g.get('execPostRecWrap')
          function['execPostRecWrapName'] = inherit_name
        if g.get('prefix') is not None:
          function['prefix'] = g.get('prefix')
          function['prefixName'] = inherit_name
        if g.get('interceptorExecOverride') is not None:
          function['interceptorExecOverride'] = g.get('interceptorExecOverride')
          function['interceptorExecOverrideName'] = inherit_name
        if g.get('suffix') is not None:
          function['suffix'] = g.get('suffix')
          function['suffixName'] = inherit_name
  if f.get('retV'):
    function['type'] = f.get('retV').get('type')
    if f.get('retV').get('wrapType'):
      function['retVwrapType'] = f.get('retV').get('wrapType')
    if f.get('retV').get('wrapParams'):
      function['retVwrapParams'] = f.get('retV').get('wrapParams')
  if f.get('type'):
    function['functionType'] = f.get('type')
  if f.get('custom'):
    function['custom'] = f.get('custom')
  if f.get('version'):
    function['version'] = f.get('version')
  i = 1

  while f.get('arg'+str(i)) or (i <= len(function['args'])):
    if f.get('arg'+str(i)):
      arg = {}
      arg['type'] = f.get('arg'+str(i)).get('type')
      arg['name'] = f.get('arg'+str(i)).get('name')
      if f.get('arg'+str(i)).get('wrapType'):
        arg['wrapType'] = f.get('arg'+str(i)).get('wrapType')
      if f.get('arg'+str(i)).get('wrapParams'):
        arg['wrapParams'] = f.get('arg'+str(i)).get('wrapParams')
      if f.get('arg'+str(i)).get('removeMapping'):
        arg['removeMapping'] = f.get('arg'+str(i)).get('removeMapping')
      if i <= len(function['args']):
        function['args'][i-1] = arg
      else:
        function['args'].append(arg)
    i += 1
  if f.get('preToken') is not None:
    function['preToken'] = f.get('preToken')
    function['preTokenName'] = f.get('name')
  if f.get('postToken') is not None:
    function['postToken'] = f.get('postToken')
    function['postTokenName'] = f.get('name')
  if f.get('stateTrack') is not None:
    function['stateTrack'] = f.get('stateTrack')
    function['stateTrackName'] = f.get('name')
  if f.get('recCond') is not None:
    function['recCond'] = f.get('recCond')
  if f.get('runCond') is not None:
    function['runCond'] = f.get('runCond')
  if f.get('preSchedule') is not None:
    function['preSchedule'] = f.get('preSchedule')
  if f.get('recWrap') is not None:
    function['recWrap'] = f.get('recWrap')
    function['recWrapName'] = f.get('name')
  if f.get('runWrap') is not None:
    function['runWrap'] = f.get('runWrap')
    function['runWrapName'] = f.get('name')
    if f.get('passToken') is not None:
      function['passToken'] = f.get('passToken')
  if f.get('ccodeWrap') is not None:
    function['ccodeWrap'] = f.get('ccodeWrap')
    function['ccodeWrapName'] = f.get('name')
  if f.get('ccodeWriteWrap') is not None:
    function['ccodeWriteWrap'] = f.get('ccodeWriteWrap')
    function['ccodeWriteWrapName'] = f.get('name')
  if f.get('execPostRecWrap') is not None:
    function['execPostRecWrap'] = f.get('execPostRecWrap')
    function['execPostRecWrapName'] = f.get('name')
  if f.get('prefix') is not None:
    function['prefix'] = f.get('prefix')
    function['prefixName'] = f.get('name')
  if f.get('interceptorExecOverride') is not None:
    function['interceptorExecOverride'] = f.get('interceptorExecOverride')
    function['interceptorExecOverrideName'] = f.get('name')
  if f.get('suffix') is not None:
    function['suffix'] = f.get('suffix')
    function['suffixName'] = f.get('name')
  if (not f.get('name') in black):
    if functions_all.get(f.get('name')) is None:
      functions_all[f.get('name')] = []
    if (functions_enabled.get(f.get('name')) is None) and (function['enabled'] is True):
      functions_enabled[f.get('name')] = []
    functions_all[f.get('name')].append(function)
    functions_all[f.get('name')].sort(key=operator.itemgetter('version'))
    if function['enabled'] is True:
      functions_enabled[f.get('name')].append(function)
      functions_enabled[f.get('name')].sort(key=operator.itemgetter('version'))

make_drivers(functions_all)
generate_gits_wrapper(functions_all)
generate_tokens(functions_enabled)
#generate_ptr_decls()
generate_create_switch(functions_enabled)
#generate_prepost(functions)
generate_openglFunction_header(functions_enabled)
generate_prepost(functions_all)
generate_prepost_h(functions_all)
generate_gliplugin_def(functions_all)


def move_file(filename, subdir):
    path = os.path.join('../' + subdir, filename)
    print('Moving {} to {}...'.format(filename, path))
    shutil.move(filename, path)


def copy_file(filename, subdir):
    path = os.path.join('../' + subdir, filename)
    print('Copying {} to {}...'.format(filename, path))
    shutil.copy2(filename, path)


copy_file('glIDs.h', '../common/include')
move_file('glIDswitch.h', '../common/include')
move_file('glDrivers.h', 'common/include')
move_file('glFunctions.h', 'common/include')
move_file('glFunctions.cpp', 'common')
move_file('openglRecorderWrapperAuto.cpp', 'recorder')
move_file('openglRecorderWrapperAuto.h', 'recorder/include')
move_file('openglRecorderWrapperIfaceAuto.h', 'recorder/include')
move_file('GLIPlugin64.def', 'GLIPlugin')
move_file('GLIPlugin32.def', 'GLIPlugin')
move_file('gitsPluginPrePostAuto.cpp', 'GLIPlugin')
move_file('gitsPluginPrePostAuto.h', 'GLIPlugin/include')
