// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include <algorithm>
#include "openglDrivers.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#include <atlimage.h>
#elif defined GITS_PLATFORM_X11
#include <X11/Xlib.h>
#endif

#include "openglTools.h"
#include "openglEnums.h"
#include "gits.h"
#ifndef BUILD_FOR_CCODE
#include "stateDynamic.h"
#include "openglLibrary.h"
#endif
#include "config.h"
#include "log.h"
#include "timer.h"
#include "pragmas.h"
#include "openglCommon.h"
#include <cassert>
#ifndef BUILD_FOR_CCODE
#include "ptblLibrary.h"
#include "ptbl_wglLibrary.h"
#include "ptbl_glxLibrary.h"
#include "ptbl_eglLibrary.h"
#include "windowContextState.h"
#include "windowing.h"
#endif
#include <filesystem>

namespace gits {
namespace OpenGL {
void get_capture_viewport(std::vector<GLint>& dims) {
#if defined GITS_PLATFORM_WINDOWS
  if (Configurator::Get().opengl.player.captureWholeWindow) {
    HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
    RECT rect;
    GetClientRect(WindowFromDC(hdc), &rect);
    dims[0] = 0;
    dims[1] = 0;
    dims[2] = rect.right - rect.left;
    dims[3] = rect.bottom - rect.top;
    return;
  }
#elif defined GITS_PLATFORM_X11
  if (Configurator::Get().opengl.player.captureWholeWindow) {
    Window window;
    int x, y;
    unsigned int w, h, b, d;
    XGetGeometry(XOpenDisplay(nullptr), drv.glx.glXGetCurrentDrawable(), &window, &x, &y, &w, &h,
                 &b, &d);
    dims[0] = 0;
    dims[1] = 0;
    dims[2] = w;
    dims[3] = h;
    LOG_INFO << "Window info: " << window << " " << x << " " << y << " " << w << " " << h << " "
             << b << " " << d;
    return;
  }
#endif
  drv.gl.glGetIntegerv(GL_VIEWPORT, &dims[0]);
}

GLenum GetTextureTarget(GLint name) {
  GLenum targets[] = {GL_TEXTURE_1D,
                      GL_TEXTURE_2D,
                      GL_TEXTURE_3D,
                      GL_TEXTURE_1D_ARRAY,
                      GL_TEXTURE_2D_ARRAY,
                      GL_TEXTURE_RECTANGLE,
                      GL_TEXTURE_CUBE_MAP,
                      GL_TEXTURE_CUBE_MAP_ARRAY,
                      GL_TEXTURE_BUFFER,
                      GL_TEXTURE_2D_MULTISAMPLE,
                      GL_TEXTURE_2D_MULTISAMPLE_ARRAY};
  drv.gl.glGetError(); //Clean Errors
  for (const auto& target : targets) {
    GLint boundTex = 0;
    drv.gl.glGetIntegerv(GetBindingEnum(target), &boundTex);
    drv.gl.glBindTexture(target, name);
    GLenum glerror = drv.gl.glGetError();
    if (glerror == 0) {
      drv.gl.glBindTexture(target, boundTex);
      return target;
    }
  }
  LOG_ERROR << "Texture name bound to unknown texture target.";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

bool IsFboAttachmentMsaa(GLenum attachment) {
  if (!curctx::IsOgl()) {
    return false;
  }
  GLint samples = 0;
  drv.gl.glGetIntegerv(GL_SAMPLES, &samples);
  return samples > 1;
}

//This function wraps glReadPixels adding support for multisampled FBOs
bool ReadPixelsWrapper(GLint x,
                       GLint y,
                       GLint width,
                       GLint height,
                       GLenum format,
                       GLenum type,
                       GLvoid* data,
                       GLenum readbuffer,
                       bool msaa) {
  PackPixelStoreStateStash pixelStoreStateStash;
  std::map<GLenum, GLint> pixelStoreSetup;
  pixelStoreSetup[GL_PACK_ALIGNMENT] = 1;
  if (curctx::IsOgl()) {
    pixelStoreSetup[GL_PACK_SWAP_BYTES] = 0;
    pixelStoreSetup[GL_PACK_LSB_FIRST] = 0;
    pixelStoreSetup[GL_PACK_ROW_LENGTH] = 0;
    pixelStoreSetup[GL_PACK_IMAGE_HEIGHT] = 0;
    pixelStoreSetup[GL_PACK_SKIP_ROWS] = 0;
    pixelStoreSetup[GL_PACK_SKIP_IMAGES] = 0;
  }
  for (auto& elem : pixelStoreSetup) {
    drv.gl.glPixelStorei(elem.first, elem.second);
  }

  PackPixelBufferBindStateStash packPixelBufferStateStash;
  if (packPixelBufferStateStash.Data() != 0) {
    drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  }

  RboBindStateStash rboStateStash;
  FboBindStateStash fboStateStash;

  GLint tmpFbo = 0;
  GLint tmpRbo = 0;
  if (fboStateStash.DrawName() > 0 && msaa) {
    //If FBO is multisampled prepare blit to non multisampled temporary FBO
    GLbitfield blitMask;
    drv.gl.glGenFramebuffers(1, (GLuint*)&tmpFbo);
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
    drv.gl.glGenRenderbuffers(1, (GLuint*)&tmpRbo);
    drv.gl.glBindRenderbuffer(GL_RENDERBUFFER, tmpRbo);
    if (readbuffer == GL_DEPTH_ATTACHMENT) {
      drv.gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, x + width, y + height);
      drv.gl.glDrawBuffer(GL_NONE);
      drv.gl.glReadBuffer(GL_NONE);
      blitMask = GL_DEPTH_BUFFER_BIT;
    } else if (readbuffer == GL_DEPTH_STENCIL_ATTACHMENT) {
      drv.gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, x + width, y + height);
      drv.gl.glDrawBuffer(GL_NONE);
      drv.gl.glReadBuffer(GL_NONE);
      blitMask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    } else {
      drv.gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, x + width, y + height);
      blitMask = GL_COLOR_BUFFER_BIT;
    }

    drv.gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, readbuffer, GL_RENDERBUFFER, tmpRbo);

    drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tmpFbo);
    drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, fboStateStash.DrawName());
    drv.gl.glBlitFramebuffer(0, 0, x + width, y + height, 0, 0, x + width, y + height, blitMask,
                             GL_NEAREST);
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
  } else if (fboStateStash.DrawTarget() == GL_DRAW_FRAMEBUFFER) {
    if (fboStateStash.DrawName() != fboStateStash.ReadName()) { //WA for GFXBench Manhattan ES 3.1
      //Rebind READ with DRAW buffer (to keep same size of golden images do not do it when capturing screenshots after swap - it is particularly important when capturing screenshots per drawcall)
      drv.gl.glBindFramebuffer(fboStateStash.ReadTarget(), fboStateStash.DrawName());
      drv.gl.glBindFramebuffer(fboStateStash.DrawTarget(), fboStateStash.ReadName());
    }
  }

  ReadBufferStateStash readBufferStateStash;
  if (readbuffer != GL_DEPTH_ATTACHMENT && readbuffer != GL_DEPTH_STENCIL_ATTACHMENT) {
    drv.gl.glReadBuffer(readbuffer);
  }

  while (drv.gl.glGetError() != GL_NO_ERROR) {
    ;
  }

  drv.gl.glReadPixels(x, y, width, height, format, type, data);
  const bool readSuccessful = (drv.gl.glGetError() == GL_NO_ERROR);

  //Restore
  readBufferStateStash.Restore();
  rboStateStash.Restore();
  fboStateStash.Restore();
  packPixelBufferStateStash.Restore();
  pixelStoreStateStash.Restore();

  //Delete temporary objects
  if (tmpRbo != 0 || tmpFbo != 0) {
    drv.gl.glDeleteFramebuffers(1, (GLuint*)&tmpFbo);
    drv.gl.glDeleteRenderbuffers(1, (GLuint*)&tmpRbo);
  }

  return readSuccessful;
}

#ifdef BUILD_FOR_CCODE
bool IsEsProfile() {
  const char* ver = (const char*)drv.gl.glGetString(GL_VERSION);
  if (ver == nullptr) {
    LOG_WARNING << "glGetString(GL_VERSION) failed. Using legacy method of detecting OGL/GLES.";
    return drv.gl.Api() != CGlDriver::API_GL;
  }

  std::string version(ver);
  auto found = version.find("OpenGL ES");
  return found != std::string::npos;
}
#endif

void capture_drawbuffer(
    const std::filesystem::path& directory,
    const std::string& file_name,
    bool force_back_buffer,
    bool dump_depth,
    bool dump_stencil) //force_back_buffer forces back buffer capture only (used per swap captures)
{
  //On desktop capture directly RGB, on ES we are guaranteed only RGBA format, so capture that
#ifndef BUILD_FOR_CCODE
  GLenum format = curctx::IsOgl() ? GL_RGB : GL_RGBA;
#else
  GLenum format = IsEsProfile() ? GL_RGBA : GL_RGB; //IsOGL() in CCode is limited
#endif

  std::vector<GLint> capture_dims(4);
  get_capture_viewport(capture_dims);

  auto SaveImage = [&](GLint fbo, GLenum drawBuff, bool msaa, std::vector<uint8_t>& data) {
    //Prepare string
    std::stringstream suffix;
    std::string msaaStr;
    if (msaa) {
      msaaStr = "_msaa";
    } else {
      msaaStr = "";
    }
    if (fbo > 0) {
      suffix << "_fbo" << fbo << DrawBufferToSuffixStr(drawBuff) << msaaStr << ".png";
    } else if (drawBuff != 0 && Configurator::Get().common.player.captureFrames.empty()) {
      suffix << DrawBufferToSuffixStr(drawBuff) << msaaStr << ".png";
    } else {
      suffix << ".png";
    }
    // Create path.
    std::filesystem::path path_color = directory;
    std::filesystem::create_directories(path_color);
    path_color /= file_name + suffix.str();
#ifndef BUILD_FOR_CCODE
    // The image will be deleted in ImageWriter class after it is consumed

    if (!Configurator::Get().common.player.captureFrames.empty() &&
        Configurator::Get().opengl.player.captureFramesHashes) {
      LOG_INFO << file_name + suffix.str() << " CRC: " << HwCrc32ishHash(&data[0], data.size(), 0);
    } else {
      CGits::Instance().WriteImage(path_color.string(), capture_dims[2], capture_dims[3],
                                   !curctx::IsOgl(), data);
    }
#else
    SavePng(path_color.string(), capture_dims[2], capture_dims[3], IsEsProfile(), &data[0]);
#endif
  };

  if (capture_dims[2] == 0 || capture_dims[3] == 0) {
    LOG_WARNING << "Incorrect image dimensions for frame " << file_name << std::endl;
    return;
  }

  DrawBuffersStateStash drawBuffersStateStash;
  FboBindStateStash fboStateStash;
  if (force_back_buffer) {
    //CAPTURE BACK BUFFER CONTENT (used to capture frame on buffer swap)
    std::vector<uint8_t> colorData(capture_dims[2] * capture_dims[3] * 4);
    ReadPixelsWrapper(capture_dims[0], capture_dims[1], capture_dims[2], capture_dims[3], format,
                      GL_UNSIGNED_BYTE, &colorData[0], GL_BACK, false);
    SaveImage(0, GL_BACK, false, colorData);
  } else {
    //CAPTURE COLOR BUFFERS  (used to capture frame on draws)
    const std::vector<GLint>& drawBuffers = drawBuffersStateStash.Data();
    if (drawBuffers.empty()) { //OPENGL ES
      std::vector<uint8_t> colorData(capture_dims[2] * capture_dims[3] * 4);
      GLenum buffType = (fboStateStash.DrawName() > 0) ? GL_COLOR_ATTACHMENT0 : GL_BACK;
      if (ReadPixelsWrapper(capture_dims[0], capture_dims[1], capture_dims[2], capture_dims[3],
                            format, GL_UNSIGNED_BYTE, &colorData[0], buffType, false)) {
        SaveImage(fboStateStash.DrawName(), buffType, false, colorData);
      }
    } else { //OPENGL
      for (auto drawBufferElem : drawBuffers) {
        if (drawBufferElem == GL_NONE) {
          continue;
        }
        std::vector<uint8_t> colorData(capture_dims[2] * capture_dims[3] * 4);
        bool msaaFbo = IsFboAttachmentMsaa(drawBufferElem);
        if (ReadPixelsWrapper(capture_dims[0], capture_dims[1], capture_dims[2], capture_dims[3],
                              format, GL_UNSIGNED_BYTE, &colorData[0], drawBufferElem, msaaFbo)) {
          SaveImage(fboStateStash.DrawName(), drawBufferElem, msaaFbo, colorData);
        }
      }
    }
    //CAPTURE DEPTH
    if (dump_depth) {
      std::vector<GLfloat> depthData(capture_dims[2] * capture_dims[3]);
      bool msaaFbo = IsFboAttachmentMsaa(GL_DEPTH_ATTACHMENT);
      bool readSuccessful = ReadPixelsWrapper(capture_dims[0], capture_dims[1], capture_dims[2],
                                              capture_dims[3], GL_DEPTH_COMPONENT, GL_FLOAT,
                                              &depthData[0], GL_DEPTH_ATTACHMENT, msaaFbo);
      if (readSuccessful) {
        std::vector<uint8_t> depthColorData = DepthToRgbSpectrum(depthData, format == GL_RGBA);
        SaveImage(fboStateStash.DrawName(), GL_DEPTH_ATTACHMENT, msaaFbo, depthColorData);
      }
    }
    //CAPTURE STENCIL
    if (dump_stencil) {
      std::vector<uint32_t> depthStencilData(capture_dims[2] * capture_dims[3]);
      bool msaaFbo = IsFboAttachmentMsaa(GL_DEPTH_STENCIL_ATTACHMENT);
      bool readSuccessful = ReadPixelsWrapper(
          capture_dims[0], capture_dims[1], capture_dims[2], capture_dims[3], GL_DEPTH_STENCIL,
          GL_UNSIGNED_INT_24_8, &depthStencilData[0], GL_DEPTH_STENCIL_ATTACHMENT, msaaFbo);
      if (readSuccessful) {
        std::vector<uint8_t> stencilColorData = StencilToRgb(depthStencilData, format == GL_RGBA);
        SaveImage(fboStateStash.DrawName(), GL_STENCIL_ATTACHMENT, msaaFbo, stencilColorData);
      }
    }
  }
}

#ifdef GITS_PLATFORM_WINDOWS
void captureScreenshot(HWND hWND,
                       const std::filesystem::path& directory,
                       const std::string& file_name) {
  RECT rc;
  GetClientRect(hWND, &rc);
  int appWidth = rc.right - rc.left;
  int appHeight = rc.bottom - rc.top;

  SetWindowPos(hWND, HWND_TOP, 0, 0, appWidth, appHeight, SWP_SHOWWINDOW);

  HDC hDCdesktop = GetDC(NULL);
  if (hDCdesktop == NULL) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  HDC hDCdestination = CreateCompatibleDC(hDCdesktop);
  if (hDCdestination == NULL) {
    ReleaseDC(NULL, hDCdesktop);
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  HBITMAP hBMPdestination = CreateCompatibleBitmap(hDCdesktop, appWidth, appHeight);
  if (hBMPdestination == NULL) {
    ReleaseDC(NULL, hDCdesktop);
    DeleteDC(hDCdestination);
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  SelectObject(hDCdestination, hBMPdestination);

  BitBlt(hDCdestination, rc.left, rc.top, appWidth, appHeight, hDCdesktop, 0, 0, SRCCOPY);
  CImage img;
  img.Attach(hBMPdestination);

  std::filesystem::path finalPath = directory;
  std::filesystem::create_directories(finalPath);
  std::stringstream suffix;
  suffix << ".png";
  finalPath /= file_name + suffix.str();

  img.Save(finalPath.string().c_str(), Gdiplus::ImageFormatPNG);

  ReleaseDC(NULL, hDCdesktop);
  DeleteDC(hDCdestination);
  DeleteObject(hBMPdestination);
}
#endif

std::filesystem::path GetPathForImageDumping() {
  auto path = Configurator::Get().common.player.outputDir;
  if (path.empty()) {
    if (Configurator::IsRecorder()) {
      path = Configurator::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
    } else if (Configurator::IsPlayer()) {
      path = Configurator::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
    } else {
      LOG_ERROR << "Neither in player nor recorder!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  return path;
}

void FrameBufferSave(unsigned frameNumber) {
  // this option cannot be used with captureScreenshot option
  if (Configurator::Get().common.player.captureScreenshot) {
    return;
  }

  auto path = GetPathForImageDumping();

  std::stringstream fileName;
  fileName << "frame" << std::setw(8) << std::setfill('0') << frameNumber;
  capture_drawbuffer(path, fileName.str(),
                     !Configurator::Get().opengl.player.dontForceBackBufferGL);
}

#ifdef GITS_PLATFORM_WINDOWS
void ScreenshotSave(unsigned frameNumber, HWND hWND) {
  auto path = GetPathForImageDumping();

  std::stringstream fileName;
  fileName << "frame" << std::setw(8) << std::setfill('0') << frameNumber;
  captureScreenshot(hWND, path, fileName.str());
}
#endif

#ifndef BUILD_FOR_CCODE
void capture_bound_texture2D(GLenum target,
                             const std::filesystem::path& directory,
                             const std::string& file_name) {
  PackPixelStoreStateStash pixelStoreStateStash;
  std::map<GLenum, GLint> pixelStoreSetup;
  pixelStoreSetup[GL_PACK_ALIGNMENT] = 1;
  if (curctx::IsOgl()) {
    pixelStoreSetup[GL_PACK_SWAP_BYTES] = 0;
    pixelStoreSetup[GL_PACK_LSB_FIRST] = 0;
    pixelStoreSetup[GL_PACK_ROW_LENGTH] = 0;
    pixelStoreSetup[GL_PACK_IMAGE_HEIGHT] = 0;
    pixelStoreSetup[GL_PACK_SKIP_ROWS] = 0;
    pixelStoreSetup[GL_PACK_SKIP_IMAGES] = 0;
  }
  for (auto& elem : pixelStoreSetup) {
    drv.gl.glPixelStorei(elem.first, elem.second);
  }
  PackPixelBufferBindStateStash packPixelBufferStateStash;
  if (packPixelBufferStateStash.Data() != 0) {
    drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  }

  //Get texture dimension
  GLint texWidth;
  GLint texHeight;
  GLint internalFormat;
  GLint textureName;
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureName);
  if (textureName == 0) {
    return;
  }

  if (curctx::IsOgl()) {
    drv.gl.glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &texWidth);
    drv.gl.glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &texHeight);
    drv.gl.glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
  } else {
    CTextureStateObj* texObj =
        SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(textureName, target));
    texWidth = texObj->Data().track.mipmapData[0].width;
    texHeight = texObj->Data().track.mipmapData[0].height;
    internalFormat = texObj->Data().track.mipmapData[0].internalFormat;
  }

  // Create path.
  std::filesystem::path path = directory;
  std::filesystem::create_directories(path);

  //Capture
  if (internalFormat != GL_DEPTH24_STENCIL8 && internalFormat != GL_DEPTH_COMPONENT24 &&
      internalFormat != GL_DEPTH_COMPONENT16) {
    if (!curctx::IsOgl()) {
      //Prepare generic FBO APIs
      auto genFramebuffers = drv.gl.glGenFramebuffers;
      auto deleteFramebuffers = drv.gl.glDeleteFramebuffers;
      auto bindFramebuffer = drv.gl.glBindFramebuffer;
      auto framebufferTexture2D = drv.gl.glFramebufferTexture2D;
      auto checkFramebufferStatus = drv.gl.glCheckFramebufferStatus;
      if (drv.gl.HasExtension("GL_OES_framebuffer_object")) {
        genFramebuffers = drv.gl.glGenFramebuffersOES;
        deleteFramebuffers = drv.gl.glDeleteFramebuffersOES;
        bindFramebuffer = drv.gl.glBindFramebufferOES;
        framebufferTexture2D = drv.gl.glFramebufferTexture2DOES;
        checkFramebufferStatus = drv.gl.glCheckFramebufferStatusOES;
      }

      GLuint tmpFbo = 0;
      FboBindStateStash fboBindStateStash;
      genFramebuffers(1, &tmpFbo);
      bindFramebuffer(GL_FRAMEBUFFER, tmpFbo);

      framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, textureName, 0);
      GLenum fboStatus = checkFramebufferStatus(GL_FRAMEBUFFER);
      bool fboComplete = fboStatus == GL_FRAMEBUFFER_COMPLETE;
      if (!fboComplete) {
        LOG_WARNING << "FBO Icomplete - texture dump failed";
        return;
      }

      std::vector<char> data;
      unsigned storeDataSize = 4 * texWidth * texHeight;

      data.resize(storeDataSize);
      drv.gl.glGetError();
      drv.gl.glReadPixels(0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
      GLint error = drv.gl.glGetError();
      if (error != 0) {
        LOG_WARNING << "Texture content dump skipped";
        return;
      }
      path /= file_name + ".png";
      SavePng(path.string(), texWidth, texHeight, true, &data[0]);
      deleteFramebuffers(1, &tmpFbo);
      fboBindStateStash.Restore();
    } else {
      std::vector<uint8_t> texData(texWidth * texHeight * 4, 0);
      drv.gl.glGetTexImage(target, 0, GL_RGB, GL_UNSIGNED_BYTE, &texData[0]);
      path /= file_name + ".png";
      SavePng(path.string(), texWidth, texHeight, false, &texData[0]);
    }
  } else {
    if (curctx::IsOgl()) {
      std::vector<GLfloat> texDepthData(texWidth * texHeight, 0);
      drv.gl.glGetTexImage(target, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &texDepthData[0]);
      std::vector<uint8_t> texData = DepthToRgb(texDepthData, false);
      path /= file_name + "_depth.png";
      SavePng(path.string(), texWidth, texHeight, false, &texData[0]);
    }
  }
  packPixelBufferStateStash.Restore();
  pixelStoreStateStash.Restore();
}

void RestoreFramebufferEXT(
    GLuint targetFBO, GLenum format, GLsizei height, GLsizei width, CBinaryResource& resource) {
  GLint boundDrawFBO;
  GLint boundReadFBO;
  drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &boundDrawFBO);
  drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &boundReadFBO);

  GLenum tmpFboAttachment = InternalFormatToFboMatch(format);
  GLenum tmpTexFormat;
  GLenum tmpTexType;
  FboToImageMatch(tmpFboAttachment, tmpTexFormat, tmpTexType);

  //create texture with buffer data content and attach to FBO
  GLuint sourceTexture;
  drv.gl.glGenTextures(1, &sourceTexture);
  drv.gl.glBindTexture(GL_TEXTURE_2D, sourceTexture);
  drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, tmpTexFormat, tmpTexType,
                      resource.Data());

  GLuint sourceFBO;
  drv.gl.glGenFramebuffersEXT(1, &sourceFBO);
  drv.gl.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sourceFBO);
  drv.gl.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, tmpFboAttachment, GL_TEXTURE_2D,
                                   sourceTexture, 0);
  if (tmpFboAttachment != GL_COLOR_ATTACHMENT0) {
    drv.gl.glDrawBuffer(GL_NONE);
    drv.gl.glReadBuffer(GL_NONE);
  }
  GLenum fboStatus = drv.gl.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    LOG_ERROR
        << "Renderbuffer restoration failed (sourceFBO) - Framebuffer incomplete - FBO status: "
        << fboStatus;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  //move texture content to RBO
  drv.gl.glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
  drv.gl.glBindFramebufferEXT(GL_READ_FRAMEBUFFER, sourceFBO);
  drv.gl.glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, targetFBO);
  drv.gl.glBlitFramebufferEXT(0, 0, width, height, 0, 0, width, height,
                              FboToBlitMask(tmpFboAttachment), GL_NEAREST);
  CGits::Instance().DrawCountUp();

  LOG_INFO << "glBlitFramebufferEXT (FBO state restore) call emitted";

  //Clear temporary objects
  drv.gl.glDeleteFramebuffersEXT(1, &sourceFBO);
  drv.gl.glDeleteTextures(1, &sourceTexture);

  //bind originaly bound FBOs
  drv.gl.glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, boundDrawFBO);
  drv.gl.glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, boundReadFBO);
}

void RestoreFramebuffer(
    GLuint targetFBO, GLenum format, GLsizei height, GLsizei width, CBinaryResource& resource) {
  GLint boundDrawFBO;
  GLint boundReadFBO;
  drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundDrawFBO);
  drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundReadFBO);

  GLenum tmpFboAttachment = InternalFormatToFboMatch(format);
  GLenum tmpTexFormat;
  GLenum tmpTexType;
  FboToImageMatch(tmpFboAttachment, tmpTexFormat, tmpTexType);

  //create texture with buffer data content and attach to FBO
  GLuint sourceTexture;
  drv.gl.glGenTextures(1, &sourceTexture);
  drv.gl.glBindTexture(GL_TEXTURE_2D, sourceTexture);
  drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, tmpTexFormat, tmpTexType,
                      resource.Data());

  GLuint sourceFBO;
  drv.gl.glGenFramebuffers(1, &sourceFBO);
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, sourceFBO);
  drv.gl.glFramebufferTexture2D(GL_FRAMEBUFFER, tmpFboAttachment, GL_TEXTURE_2D, sourceTexture, 0);
  if (tmpFboAttachment != GL_COLOR_ATTACHMENT0) {
    drv.gl.glDrawBuffer(GL_NONE);
    drv.gl.glReadBuffer(GL_NONE);
  }
  GLenum fboStatus = drv.gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    LOG_ERROR
        << "Renderbuffer restoration failed (sourceFBO) - Framebuffer incomplete - FBO status: "
        << fboStatus;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  //move texture content to RBO
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFBO);
  drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFBO);
  drv.gl.glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                           FboToBlitMask(tmpFboAttachment), GL_NEAREST);
  CGits::Instance().DrawCountUp();

  LOG_INFO << "glBlitFramebuffer (FBO state restore) call emitted";

  //Clear temporary objects
  drv.gl.glDeleteFramebuffers(1, &sourceFBO);
  drv.gl.glDeleteTextures(1, &sourceTexture);

  //bind originaly bound FBOs
  drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, boundDrawFBO);
  drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, boundReadFBO);
}
#endif

unsigned texUnpackRowSize(GLenum format, GLenum type, GLint texWidth) {
  //Get texels per row
  GLint rowTexels = 0;
  if (curctx::IsOgl()) {
    drv.gl.glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowTexels);
  }
  if (rowTexels == 0) {
    rowTexels = texWidth;
    if (rowTexels == 0) {
      return 0;
    }
  }

  //Evaluate bytes per row
  GLint texelSizei = texelSize(format, type);
  GLint texelElements = texelElementsNum(format);
  unsigned compSize = texelSizei / texelElements;

  GLint alignment = 1;
  drv.gl.glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

  if (!isTexelDataConsistent(type) || (floor((float)texelSizei / (float)texelElements) == 0) ||
      //components which size is not n-times of 1 byte. (simplification of specs -
      // "If the number of bits per element is not 1, 2, 4, or 8 times then k = nl for all values of a")
      (compSize >= static_cast<unsigned>(alignment))) {
    return (unsigned)(texelSizei * rowTexels);
  } else {
    return (unsigned)(alignment * ceil((float)(texelSizei * rowTexels) / (float)alignment));
  }
}

unsigned TexDataSize(GLenum format, GLenum type, GLsizei width, GLsizei height, GLsizei depth) {
  //Get Pixel Store Params
  GLint rowOffset = 0;
  GLint pixOffset = 0;
  GLint imgHeight = 0;
  GLint imgOffset = 0;
  if (curctx::IsOgl() || curctx::IsEs3Plus()) {
#ifndef BUILD_FOR_CCODE
    if (CGits::Instance().IsStateRestoration()) {
      auto* schedulerStatePtr = SD().GetCurrentContextStateData().GetSchedulerStatePtr();
      if (schedulerStatePtr == nullptr) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      pixOffset = schedulerStatePtr->GetGLParamui(GL_UNPACK_SKIP_PIXELS);
      rowOffset = schedulerStatePtr->GetGLParamui(GL_UNPACK_SKIP_ROWS);
      imgHeight = schedulerStatePtr->GetGLParamui(GL_UNPACK_IMAGE_HEIGHT);
      imgOffset = schedulerStatePtr->GetGLParamui(GL_UNPACK_SKIP_IMAGES);
    } else {
#endif
      drv.gl.glGetIntegerv(GL_UNPACK_SKIP_ROWS, &rowOffset);
      drv.gl.glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &pixOffset);
      drv.gl.glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &imgHeight);
      drv.gl.glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &imgOffset);
#ifndef BUILD_FOR_CCODE
    }
#endif
  }

  // Get used tex data size.
  if (height == 1 && depth == 1 && rowOffset == 0) {
    // 1D
    return texUnpackRowSize(format, type, width);
  } else if (depth == 1 && imgOffset == 0) {
    // 2D
    GLint rowSize = texUnpackRowSize(format, type, width);
    GLint pixSize = texelSize(format, type);
    GLint pixRestSize = rowSize - pixSize * (pixOffset + width);
    return rowSize * (rowOffset + height) - pixRestSize;
  } else {
    // 3D
    return texUnpackRowSize(format, type, width) * (imgHeight > 0 ? imgHeight : height) *
           (imgOffset + depth);
  }
}

unsigned CompressedTexDataSize(GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize) {
  if (imageSize < 0) {
    imageSize = 0; //WA for Worms UM
  }

  if (!curctx::IsOgl()) {
    return imageSize;
  }

  GLint blockSize = 0;
  GLint blockWidth = 0;
  GLint blockHeight = 0;

#ifdef BUILD_FOR_CCODE
  ENotImplemented(EXCEPTION_MESSAGE);
#else
  if (curctx::Version() >= 430 || drv.gl.HasExtension("GL_ARB_compressed_texture_pixel_storage")) {
#endif
  drv.gl.glGetIntegerv(GL_UNPACK_COMPRESSED_BLOCK_SIZE, &blockSize);
  drv.gl.glGetIntegerv(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, &blockWidth);
  drv.gl.glGetIntegerv(GL_UNPACK_COMPRESSED_BLOCK_HEIGHT, &blockHeight);
#ifndef BUILD_FOR_CCODE
}
#endif

if (blockSize == 0 || blockWidth == 0 || blockHeight == 0) {
  return imageSize;
}

GLint skipPixels = 0;
GLint skipRows = 0;
GLint rowLength = 0;
drv.gl.glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
drv.gl.glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
drv.gl.glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
if (skipPixels == 0 && skipRows == 0 && rowLength == 0) {
  return imageSize;
}

GLint areaWidth = width;
GLint areaHeight = height;
if (depth != 1) {
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

if (rowLength > 0) {
  areaWidth = rowLength;
} else if (skipPixels > 0) {
  areaWidth += skipPixels;
}
if (skipRows > 0) {
  areaHeight += skipRows;
}

GLint blocksWidth = (GLint)ceil(areaWidth / blockWidth);
GLint blocksHeight = (GLint)ceil(areaHeight / blockHeight);
return blocksWidth * blocksHeight * blockSize;
}

GLenum MapAccessBitFieldToEnum(GLbitfield access) {
  if ((access & GL_MAP_WRITE_BIT) && (access & GL_MAP_READ_BIT)) {
    return GL_READ_WRITE;
  } else if (access & GL_MAP_WRITE_BIT) {
    return GL_WRITE_ONLY;
  } else if (access & GL_MAP_READ_BIT) {
    return GL_READ_ONLY;
  } else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

GLbitfield MapAccessEnumToBitField(GLenum access) {
  switch (access) {
  case GL_READ_WRITE:
    return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
  case GL_WRITE_ONLY:
    return GL_MAP_WRITE_BIT;
  case GL_READ_ONLY:
    return GL_MAP_READ_BIT;
  default:
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

unsigned BitmapDataSize(GLsizei width, GLsizei height) {
  GLint alignment = 4;
  GLint length = 0;

  if (curctx::IsOgl()) {
#ifndef BUILD_FOR_CCODE
    if (CGits::Instance().IsStateRestoration()) {
      auto* schedulerStatePtr = SD().GetCurrentContextStateData().GetSchedulerStatePtr();
      if (schedulerStatePtr == nullptr) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      alignment = schedulerStatePtr->GetGLParamui(GL_UNPACK_ALIGNMENT);
      length = schedulerStatePtr->GetGLParamui(GL_UNPACK_ROW_LENGTH);
    } else {
#endif
      drv.gl.glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
      drv.gl.glGetIntegerv(GL_UNPACK_ROW_LENGTH, &length);
#ifndef BUILD_FOR_CCODE
    }
#endif
  }
  if (length == 0) {
    length = width;
  }

  return static_cast<size_t>(height * alignment * ceil(length / (8.0f * alignment)));
}

unsigned getTexImageSize(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type) {
  unsigned texImageSize = 0;
  GLint lineLength = 0;
  GLint imageHeight = 0;
  GLint alignment = 0;
  GLint skipPixels = 0;
  GLint skipLines = 0;
  GLint skipImages = 0;
  GLint padding = 0;
  GLdouble groupSize = 0;
  GLint rowSize = 0;
  GLint imageSize = 0;

  if (width <= 0 || height <= 0 || depth <= 0 ||
      (height == 1 && depth != 1)) { // When height = 1, buffer is 2D. Hence depth should also be 1.
    LOG_ERROR << "Dimension(s) is not set correctly: width-" << width << ", height-" << height
              << ", depth-" << depth;
    throw ENotInitialized(EXCEPTION_MESSAGE);
  }

  // Get pixel pack params
  // Common param for all dimensions
  drv.gl.glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);

  // Get 1D params
  drv.gl.glGetIntegerv(GL_PACK_ROW_LENGTH, &lineLength);
  drv.gl.glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);

  // Get 2D params
  if (height > 1) {
    drv.gl.glGetIntegerv(GL_PACK_SKIP_ROWS, &skipLines);
    drv.gl.glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &imageHeight);
  }

  // Get 3D param
  if (depth > 1) {
    drv.gl.glGetIntegerv(GL_PACK_SKIP_IMAGES, &skipImages);
  }

  // Calculate image size
  GLint groupsPerLine = (lineLength > 0) ? lineLength : width;
  GLint rowsPerImage = (imageHeight > 0) ? imageHeight : height;

  if (type == GL_BITMAP) {
    groupSize = 1 / 8.0;
    rowSize = static_cast<GLint>((groupsPerLine + 7) *
                                 groupSize); // groupsPerLine % 8 bits will require another byte
  } else {
    groupSize = texelSize(format, type);
    rowSize = static_cast<GLint>(groupsPerLine * groupSize);
  }

  padding = rowSize & (alignment - 1);
  if (padding) {
    rowSize += alignment - padding;
  }
  imageSize = rowsPerImage * rowSize;

  texImageSize += imageSize * (skipImages + depth - 1);
  texImageSize += rowSize * (skipLines + height - 1);
  texImageSize += static_cast<GLsizei>(groupSize * (skipPixels + width));

  // add bitmap adjustment offset
  if (type == GL_BITMAP) {
    texImageSize +=
        static_cast<GLsizei>((skipPixels + width + 7) * groupSize) -
        (static_cast<GLsizei>(skipPixels * groupSize) + static_cast<GLsizei>(width * groupSize));
  }

  return texImageSize;
}

GLint BoundBuffer(GLenum target) {
  GLint boundBuffer = -1;
  if (!curctx::IsOgl11()) {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &boundBuffer);
  } else {
    boundBuffer = 0;
  }

  if (boundBuffer == -1) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return boundBuffer;
}

GLint BoundTexture(GLenum target) {
  GLenum bindingTextureEnum = 0;
  GLint boundTexture = -1;

  //all of GL_TEXTURE_CUBE_MAP_[POSITIVE / NEGATIVE]_[X / Y / Z] should be treated like GL_TEXTURE_CUBE_MAP
  if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
    target = GL_TEXTURE_CUBE_MAP;
  }

  bindingTextureEnum = GetBindingEnum(target);
  drv.gl.glGetIntegerv(bindingTextureEnum, &boundTexture);

  return boundTexture;
}

#ifndef BUILD_FOR_CCODE
void SaveProgramBinary(GLuint program, hash_t hash) {
  GLint length = 0;
  drv.gl.glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);

  GLenum format = 0;
  std::vector<char> data(sizeof(GLenum) + length);
  drv.gl.glGetProgramBinary(program, length, &length, &format, &data[sizeof(GLenum)]);
  *reinterpret_cast<GLenum*>(&data[0]) = format;

  // Use link number as hash for this resource.
  CLibrary::Get().ProgramBinaryManager().put(RESOURCE_BUFFER, &data[0], sizeof(GLenum) + length,
                                             hash);
}

void RestoreProgramBinary(GLuint program, hash_t hash) {
  std::vector<char> data = CLibrary::Get().ProgramBinaryManager().get(hash);

  GLenum* format = (GLenum*)data.data();
  const char* data_ptr = data.data() + sizeof(GLenum);

  drv.gl.glProgramBinary(program, *format, data_ptr, (GLsizei)data.size() - sizeof(GLenum));
}
#endif
PackPixelStoreStateStash::PackPixelStoreStateStash() {
  _data[GL_PACK_ALIGNMENT] = 0;
  if (curctx::IsOgl()) {
    _data[GL_PACK_SWAP_BYTES] = 0;
    _data[GL_PACK_LSB_FIRST] = 0;
    _data[GL_PACK_ROW_LENGTH] = 0;
    _data[GL_PACK_IMAGE_HEIGHT] = 0;
    _data[GL_PACK_SKIP_ROWS] = 0;
    _data[GL_PACK_SKIP_IMAGES] = 0;
  }
  for (auto& elem : _data) {
    drv.gl.glGetIntegerv(elem.first, &(elem.second));
  }
}

void PackPixelStoreStateStash::Restore() {
  for (auto& elem : _data) {
    drv.gl.glPixelStorei(elem.first, elem.second);
  }
}

UnPackPixelStoreStateStash::UnPackPixelStoreStateStash() {
  _data[GL_UNPACK_ALIGNMENT] = 0;
  if (curctx::IsOgl()) {
    _data[GL_UNPACK_SWAP_BYTES] = 0;
    _data[GL_UNPACK_LSB_FIRST] = 0;
    _data[GL_UNPACK_ROW_LENGTH] = 0;
    _data[GL_UNPACK_IMAGE_HEIGHT] = 0;
    _data[GL_UNPACK_SKIP_ROWS] = 0;
    _data[GL_UNPACK_SKIP_IMAGES] = 0;
  }
  for (auto& elem : _data) {
    drv.gl.glGetIntegerv(elem.first, &(elem.second));
  }
}

void UnPackPixelStoreStateStash::Restore() {
  for (auto& elem : _data) {
    drv.gl.glPixelStorei(elem.first, elem.second);
  }
}

PackPixelBufferBindStateStash::PackPixelBufferBindStateStash() : _data(0) {
  if (curctx::IsOgl()) {
    drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &_data);
  }
}

void PackPixelBufferBindStateStash::Restore() {
  if (!curctx::IsOgl()) {
    return;
  }
  GLint packBuffer = 0;
  drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
  if (_data != packBuffer) {
    drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, _data);
  }
}

DrawBuffersStateStash::DrawBuffersStateStash() {
  if (curctx::IsOgl() || curctx::IsEs3Plus()) {
    GLint maxDrawBuffers = 0;
    drv.gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    for (GLint i = 0; i < maxDrawBuffers; ++i) {
      GLint drawBuffer = 0;
      drv.gl.glGetIntegerv(GL_DRAW_BUFFER0 + i, &drawBuffer);
      _data.push_back(drawBuffer);
    }
  }
}

void DrawBuffersStateStash::Restore() {
  if (curctx::IsOgl()) {
    drv.gl.glDrawBuffers((GLsizei)_data.size(), (GLenum*)&_data[0]);
  }
}

ReadBufferStateStash::ReadBufferStateStash() : _data(0) {
  if (curctx::IsOgl()) {
    drv.gl.glGetIntegerv(GL_READ_BUFFER, (GLint*)&_data);
  }
}

void ReadBufferStateStash::Restore() {
  if (curctx::IsOgl()) {
    drv.gl.glReadBuffer(_data);
  }
}

FboBindStateStash::FboBindStateStash() : _extFboBlitSupport(false) {
  //Check FBO binding
  if (!curctx::IsEs1()) {
#ifndef BUILD_FOR_CCODE
    _extFboBlitSupport =
        drv.gl.HasExtension("GL_EXT_framebuffer_blit") || SD().IsCurrentContextCore();
#else
      _extFboBlitSupport = drv.gl.HasExtension("GL_EXT_framebuffer_blit");
#endif
    if (!_extFboBlitSupport) {
      _data[GL_FRAMEBUFFER] = 0;
      drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_data[GL_FRAMEBUFFER]);
    } else {
      _data[GL_DRAW_FRAMEBUFFER] = 0;
      drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_data[GL_DRAW_FRAMEBUFFER]);
      _data[GL_READ_FRAMEBUFFER] = 0;
      drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_data[GL_READ_FRAMEBUFFER]);
    }
  }
}

void FboBindStateStash::Restore() {
  auto bindFramebuffer = drv.gl.glBindFramebuffer;
  if (drv.gl.HasExtension("GL_OES_framebuffer_object")) {
    bindFramebuffer = drv.gl.glBindFramebufferOES;
  }

  //Check FBO binding
  for (auto& pair : _data) {
    bindFramebuffer(pair.first, pair.second);
  }
}

GLenum FboBindStateStash::ReadTarget() {
  if (curctx::IsEs1()) {
    return (GLenum)0;
  }
  return (_extFboBlitSupport) ? GL_READ_FRAMEBUFFER : GL_FRAMEBUFFER;
}
GLenum FboBindStateStash::DrawTarget() {
  if (curctx::IsEs1()) {
    return (GLenum)0;
  }
  return (_extFboBlitSupport) ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER;
}
GLint FboBindStateStash::ReadName() {
  if (curctx::IsEs1()) {
    return (GLenum)0;
  }
  return (_extFboBlitSupport) ? _data[GL_READ_FRAMEBUFFER] : _data[GL_FRAMEBUFFER];
}
GLint FboBindStateStash::DrawName() {
  if (curctx::IsEs1()) {
    return (GLenum)0;
  }
  return (_extFboBlitSupport) ? _data[GL_DRAW_FRAMEBUFFER] : _data[GL_FRAMEBUFFER];
}

RboBindStateStash::RboBindStateStash() : _data(0) {
  if (!curctx::IsEs1()) {
    drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING, &_data);
  }
}

void RboBindStateStash::Restore() {
  if (!curctx::IsEs1()) {
    drv.gl.glBindRenderbuffer(GL_RENDERBUFFER, _data);
  }
}

TextureBindStateStash::TextureBindStateStash(GLenum target) {
  _data.name = 0;
  _data.target = target;
  if (!curctx::IsEs1()) {
    drv.gl.glGetIntegerv(GetBindingEnum(_data.target), &_data.name);
  }
}

void TextureBindStateStash::Restore() {
  if (!curctx::IsEs1()) {
    drv.gl.glBindTexture(_data.target, _data.name);
  }
}

ScissorStateStash::ScissorStateStash() : _data(5) {
  drv.gl.glGetIntegerv(GL_SCISSOR_BOX, &_data[0]);
  _data[4] = (int)drv.gl.glIsEnabled(GL_SCISSOR_TEST);
}

void ScissorStateStash::Restore() {
  if ((bool)_data[4] != (bool)drv.gl.glIsEnabled(GL_SCISSOR_TEST)) {
    if (_data[4] > 0) {
      drv.gl.glEnable(GL_SCISSOR_TEST);
    } else {
      drv.gl.glDisable(GL_SCISSOR_TEST);
    }
  }
  drv.gl.glScissor(_data[0], _data[1], _data[2], _data[3]);
}

const GLenum BufferStateStash::bufferBindingTargets[] = {
    GL_ARRAY_BUFFER,        GL_ELEMENT_ARRAY_BUFFER, GL_PIXEL_PACK_BUFFER,
    GL_PIXEL_UNPACK_BUFFER, GL_UNIFORM_BUFFER,       GL_TEXTURE_BUFFER};

BufferStateStash::BufferStateStash() : _data(bufferTargetCount) {
  for (unsigned int i = 0; i < bufferTargetCount; i++) {
    GLenum target = bufferBindingTargets[i];
    drv.gl.glGetIntegerv(GetBindingEnum(target), (GLint*)&_data[i]);
  }
}

void BufferStateStash::Restore() {
  for (unsigned int i = 0; i < bufferTargetCount; i++) {
    GLenum target = bufferBindingTargets[i];
    drv.gl.glBindBuffer(target, _data[i]);
  }
}

MapBuffer::MapBuffer(GLenum target, GLint buffer) : _target(target), _ptr(nullptr) {
  if (curctx::IsOgl() || ESBufferState() != TBuffersState::CAPTURE_ALWAYS) {
    auto func_map = drv.gl.glMapBuffer;
    if (drv.gl.HasExtension("GL_OES_mapbuffer")) {
      func_map = drv.gl.glMapBufferOES;
    }

    GLenum mapType;
    if (curctx::IsOgl()) {
      mapType = GL_READ_ONLY;
    } else {
      mapType = GL_WRITE_ONLY;
    }

    _ptr = func_map(target, mapType);
    if (_ptr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
#ifndef BUILD_FOR_CCODE
  else { //ES api where read write only mappings not supported
    if (SD().GetCurrentSharedStateData().Buffers().Get(buffer) == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    _ptr = (void*)&SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().restore.buffer[0];
  }
#endif
}

MapBuffer::~MapBuffer() {
  try {
    if (curctx::IsOgl() || ESBufferState() != TBuffersState::CAPTURE_ALWAYS) {
      auto func_unmap = drv.gl.glUnmapBuffer;
      if (drv.gl.HasExtension("GL_OES_mapbuffer")) {
        func_unmap = drv.gl.glUnmapBufferOES;
      }
      func_unmap(_target);
    }
  } catch (...) {
    topmost_exception_handler("MapBuffer::~MapBuffer");
  }
}

//Returns table filled with colors of visible light spectrum, ranging from purple (first table index) to red (last index)
std::vector<uint32_t> GenerateRgbSpectrum() {
  std::vector<uint32_t> spectrum(256 * 5);
  for (size_t val = 0; val < spectrum.size(); val++) {
    uint32_t rgb = 0;
    uint8_t* pr = (uint8_t*)&rgb;
    uint8_t* pg = pr + 1;
    uint8_t* pb = pr + 2;
    if (val < 256) {
      auto v = static_cast<uint8_t>(255 - val);
      *pr = v;
      *pg = 0;
      *pb = 255; // purple -> blue
    } else if (val < 2 * 256) {
      auto v = static_cast<uint8_t>(val - 256);
      *pr = 0;
      *pg = v;
      *pb = 255; // blue -> cyan
    } else if (val < 3 * 256) {
      auto v = static_cast<uint8_t>(val - 2 * 256);
      *pr = 0;
      *pg = 255;
      *pb = 255 - v; // cyan -> green
    } else if (val < 4 * 256) {
      auto v = static_cast<uint8_t>(val - 3 * 256);
      *pr = v;
      *pg = 255;
      *pb = 0; // green -> yellow
    } else {
      auto v = static_cast<uint8_t>(val - 4 * 256);
      *pr = 255;
      *pg = 255 - v;
      *pb = 0; // yellow -> red
    }
    spectrum[val] = rgb;
  }
  return spectrum;
}

std::vector<uint8_t> DepthToRgbSpectrum(std::vector<GLfloat>& depthData, bool alpha) {
  static std::vector<uint32_t> spectrum = GenerateRgbSpectrum();

  //Linearization
  std::vector<GLfloat> linearDepthData(depthData.size(), 0);
  std::transform(depthData.begin(), depthData.end(), linearDepthData.begin(),
                 [=](GLfloat z) -> GLfloat {
                   float n = 1.0f;
                   float f = 10000.0f;
                   return (2.0f * n) / (f + n - z * (f - n));
                 });

  //Normalization
  std::vector<GLfloat> normDepthData(depthData.size(), 0);
  GLfloat min = *std::min_element(depthData.begin(), depthData.end());
  GLfloat max = *std::max_element(depthData.begin(), depthData.end());
  GLfloat range = max - min;
  if (range > 0) {
    std::transform(depthData.begin(), depthData.end(), normDepthData.begin(),
                   [=](GLfloat val) -> GLfloat { return ((val - min) / range); });
  }

  //Convert to RGB
  std::vector<uint8_t> colorData;
  for (auto& normalized : normDepthData) {
    //Assign normalized depth values to color, from 0.0f (red, near plane) to 1.0f (purple, far plane)
    int val = static_cast<int>((spectrum.size() - 1) * (1.0f - normalized));
    uint8_t r = 0, g = 0, b = 0; //black color serves as 'index out of range' indicator
    if (val >= 0 && val < static_cast<int>(spectrum.size())) {
      const uint8_t* specPtr = (const uint8_t*)(&spectrum[val]);
      r = *specPtr++;
      g = *specPtr++;
      b = *specPtr;
    }
    colorData.push_back(r);
    colorData.push_back(g);
    colorData.push_back(b);
    if (alpha) {
      colorData.push_back(255);
    }
  }

  return colorData;
}

/*
  std::vector<uint8_t> DepthToRgbGrey(std::vector<GLfloat>& depthData, bool alpha)
  {
    //Normalization
    std::vector<GLfloat> normDepthData(depthData.size(),0);
    GLfloat min = *std::min_element(depthData.begin(), depthData.end());
    GLfloat max = *std::max_element(depthData.begin(), depthData.end());
    GLfloat range = max - min;
    if (range>0)
      std::transform(depthData.begin(), depthData.end(), normDepthData.begin(),
        [=](GLfloat val) -> GLfloat { return ((val-min)/range); });

    //Convert to RGB
    std::vector<uint8_t> colorData;
    for (auto nomalized: normDepthData) {
      uint8_t compVal = (uint8_t)((1 - nomalized) * 255);
      for (int i=0; i<3; i++)
        colorData.push_back(compVal);
      if (alpha)
        colorData.push_back(255);
    }
    return colorData;
  }*/

std::vector<uint8_t> DepthToRgb(std::vector<GLfloat>& depthData, bool alpha) {
  std::vector<uint8_t> colorData;
  for (auto& depth : depthData) {
    const uint8_t* ptr = (const uint8_t*)(&depth);
    for (int i = 0; i < 3; i++, ptr++) {
      colorData.push_back(*ptr);
    }
    if (alpha) {
      colorData.push_back(255);
    }
  }
  return colorData;
}

std::vector<uint8_t> StencilToRgb(std::vector<uint32_t>& depthStencilData, bool alpha) {
  //Stencil values extraction (GL_UNSIGNED_INT_24_8 format)
  std::vector<uint8_t> stencilData(depthStencilData.size(), 0);
  std::transform(depthStencilData.begin(), depthStencilData.end(), stencilData.begin(),
                 [=](uint32_t depthStencil) -> uint8_t {
                   const uint8_t* ptr = (const uint8_t*)(&depthStencil);
                   return ptr[0];
                 });

  //"Normalization"
  std::vector<uint8_t> normStencilData(stencilData.size(), 0);
  auto min = *std::min_element(stencilData.begin(), stencilData.end());
  auto max = *std::max_element(stencilData.begin(), stencilData.end());
  auto range = max - min;
  if (range > 0) {
    std::transform(
        stencilData.begin(), stencilData.end(), normStencilData.begin(),
        [=](uint8_t val) -> uint8_t { return static_cast<uint8_t>(255 * (val - min) / range); });
  }

  //Convert to RGB
  std::vector<uint8_t> colorData;
  for (auto& stencil : normStencilData) {
    for (int i = 0; i < 3; i++) {
      colorData.push_back(stencil);
    }
    if (alpha) {
      colorData.push_back(255);
    }
  }
  return colorData;
}

#ifndef BUILD_FOR_CCODE
// If the drawable exists for given context, the function calls makeCurrent of respectice platform and returns true,
// otherwise it returns false
bool MakeCurrentIfDrawableExists(void* context) {
  bool status = false;
  bool retValue = 0;

  if (PtblNtvStreamApi() == PtblNativeAPI::WGL) {
    HDC hdc = (HDC)CStateDynamicNative::Get().GetHdcFromHglrc(context);

    if (hdc != nullptr) {
      if (Configurator::Get().common.mode == GITSMode::MODE_PLAYER) {
        retValue = ptbl_wglMakeCurrent(hdc, (HGLRC)context);
      }
#ifdef GITS_PLATFORM_WINDOWS
      if (Configurator::Get().common.mode == GITSMode::MODE_RECORDER) {
        retValue = drv.wgl.wglMakeCurrent(hdc, (HGLRC)context);
      }
#endif
      if (retValue) {
        status = true;
      }
    }
  } else if (PtblNtvStreamApi() == PtblNativeAPI::GLX) {
    GLXDrawable glXDrawDrawable = CStateDynamicNative::Get().GetglXDrawSurfaceFromContext(context);
    GLXDrawable glXReadDrawable = CStateDynamicNative::Get().GetglXReadSurfaceFromContext(context);

    if (glXDrawDrawable != 0 && glXReadDrawable != 0) {
      if (Configurator::Get().common.mode == GITSMode::MODE_PLAYER) {
        ptbl_glXMakeContextCurrent((Display*)GetNativeDisplay(), (GLXDrawable)glXReadDrawable,
                                   (GLXDrawable)glXDrawDrawable, (GLXContext)context);
      }
#ifdef GITS_PLATFORM_X11
      if (Configurator::Get().common.mode == GITSMode::MODE_RECORDER) {
        drv.glx.glXMakeContextCurrent((Display*)GetNativeDisplay(), (GLXDrawable)glXReadDrawable,
                                      (GLXDrawable)glXDrawDrawable, (GLXContext)context);
      }
#endif
      status = true;
    }
  } else if (PtblNtvStreamApi() == PtblNativeAPI::EGL) {
    EGLSurface eglDrawDrawable = CStateDynamicNative::Get().GetEglDrawSurfaceFromContext(context);
    EGLSurface eglReadDrawable = CStateDynamicNative::Get().GetEglReadSurfaceFromContext(context);
    if (eglDrawDrawable != nullptr && eglReadDrawable != nullptr) {
      if (Configurator::Get().common.mode == GITSMode::MODE_PLAYER) {
        ptbl_eglMakeCurrent(ptbl_GetEGLDisplay(), (EGLSurface)eglReadDrawable,
                            (EGLSurface)eglDrawDrawable, (EGLContext)context);
      }
      if (Configurator::Get().common.mode == GITSMode::MODE_RECORDER) {
        drv.egl.eglMakeCurrent(GetEGLDisplay(), (EGLSurface)eglReadDrawable,
                               (EGLSurface)eglDrawDrawable, (EGLContext)context);
      }
      status = true;
    }
  } else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  return status;
}

bool SetCurrentContext(void* context) {
  bool status = false;

  if (context != nullptr) {
    // For each platform call makeCurrent only if drawable exists, otherwise log and reset context to 0.
    // Finally set the context in CStateDynamic
    status = MakeCurrentIfDrawableExists(context);

    // Drawable does not exist
    if (!status) {
      LOG_WARNING << "Surface does not exist. Setting current context to 0.";
      context = nullptr;
    }

    SD().SetCurrentContext(context);
  }

  return status;
}

void* GetCurrentContextAPI() {
#ifdef GITS_PLATFORM_WINDOWS
  // use EGL when libEGL.dll is loaded
  if (drv.gl.Api() == CGlDriver::API_GLES1 || drv.gl.Api() == CGlDriver::API_GLES2) {
    return (void*)drv.egl.eglGetCurrentContext();
  }
  // use WGL when OpenGL32.dll is loaded
  else if (drv.gl.Api() == CGlDriver::API_GL || curctx::IsOgl()) {
    return (void*)drv.wgl.wglGetCurrentContext();
  }
  // this code should never be reached
  else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
#elif defined GITS_PLATFORM_X11
  if (curctx::IsOgl()) {
    return (void*)drv.glx.glXGetCurrentContext();
  } else {
    return (void*)drv.egl.eglGetCurrentContext();
  }
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}
#endif

std::set<GLuint> CurrentAttribsBuffers() {
  std::set<GLuint> buffers;
  GLint attribs = 0;
  drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attribs);

  for (int i = 0; i < attribs; ++i) {
    GLint enabled = GL_FALSE;
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    if (enabled == GL_TRUE) {
      GLint buf = 0;
      drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buf);
      buffers.insert(buf);
    }
  }

  GLint buf = 0;
  drv.gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buf);
  buffers.insert(buf);

  auto add_buffer = [&](GLenum target, GLenum enable) {
    if (drv.gl.glIsEnabled(enable)) {
      GLint buf = 0;
      drv.gl.glGetIntegerv(target, &buf);
      buffers.insert(buf);
    }
  };

  if (curctx::IsOgl()) {
    add_buffer(GL_VERTEX_ARRAY_BUFFER_BINDING, GL_VERTEX_ARRAY);
    add_buffer(GL_NORMAL_ARRAY_BUFFER_BINDING, GL_NORMAL_ARRAY);
    add_buffer(GL_COLOR_ARRAY_BUFFER_BINDING, GL_COLOR_ARRAY);
    add_buffer(GL_INDEX_ARRAY_BUFFER_BINDING, GL_INDEX_ARRAY);
    add_buffer(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, GL_TEXTURE_COORD_ARRAY);
    add_buffer(GL_EDGE_FLAG_ARRAY_BUFFER_BINDING, GL_EDGE_FLAG_ARRAY);
    add_buffer(GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING, GL_SECONDARY_COLOR_ARRAY);
    add_buffer(GL_FOG_COORD_ARRAY_BUFFER_BINDING, GL_FOG_COORD_ARRAY);
  }

  buffers.erase(0);
  return buffers;
}

#ifndef BUILD_FOR_CCODE
std::map<GLuint, hash_t> CurrentAttribsBuffersHash() {
  std::map<GLuint, hash_t> hashes;
  GLint old = 0;
  GLvoid* ptr = nullptr;
  drv.gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old);

  for (auto buffer : CurrentAttribsBuffers()) {
    GLint size = 0;
    drv.gl.glBindBuffer(GL_ARRAY_BUFFER, buffer);
    drv.gl.glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    auto func_map = drv.gl.glMapBuffer;
    auto func_unmap = drv.gl.glUnmapBuffer;
    if (drv.gl.HasExtension("GL_OES_mapbuffer")) {
      func_map = drv.gl.glMapBufferOES;
      func_unmap = drv.gl.glUnmapBufferOES;
    }

    if (load_egl_or_native("glMapBufferRange")) {
      ptr = drv.gl.glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_READ_BIT);
    } else {
      ptr = func_map(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    }

    hashes[buffer] = ComputeHash(ptr, size, THashType::XX);
    func_unmap(GL_ARRAY_BUFFER);
  }

  drv.gl.glBindBuffer(GL_ARRAY_BUFFER, old);
  return hashes;
}
#endif

TBuffersState ESBufferState() {
  static bool checked = false;
  static bool can_read = false;
  static TBuffersState buffers_state = TBuffersState::CAPTURE_ALWAYS;

  if (Configurator::Get().opengl.recorder.buffersState == TBuffersState::CAPTURE_ALWAYS) {
    return buffers_state;
  }

  if (!checked) {
    can_read = drv.gl.CanReadWriteonlyMappings();
  } else {
    return buffers_state;
  }
  checked = true;
  if (!can_read) {
    LOG_ERROR << "Buffers State option not supported";
    throw ENotSupported(EXCEPTION_MESSAGE);
  } else {
    buffers_state = Configurator::Get().opengl.recorder.buffersState;
  }
  return buffers_state;
}

bool IsGlGetTexImagePresentOnGLES() {
  static bool presenceChecked = false;
  static bool presence = false;

  if (!presenceChecked) {
#ifdef GITS_PLATFORM_WINDOWS
    bool useGlGetTexImageAndRestoreBuffersWhenPossibleES =
        Configurator::Get().opengl.recorder.useGlGetTexImageAndRestoreBuffersWhenPossibleES;

    if (useGlGetTexImageAndRestoreBuffersWhenPossibleES &&
        check_gl_function_availability("glGetTexImage")) {
      GLint oldBinding = 0;
      drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding);

      GLuint pixelsWrite = 0xAAAAAADD;
      GLuint pixelsRead = 0;

      GLuint texture = 0;
      drv.gl.glGenTextures(1, &texture);
      drv.gl.glBindTexture(GL_TEXTURE_2D, texture);
      drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 1, 1, 0, GL_DEPTH_STENCIL,
                          GL_UNSIGNED_INT_24_8, &pixelsWrite);
      drv.gl.glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &pixelsRead);

      if (pixelsRead == pixelsWrite) {
        LOG_INFO << "glGetTexImage seems to be present in OpenGL ES API";
        presence = true;
      } else {
        LOG_INFO << "glGetTexImage seems to be unsupported on this OpenGL ES driver";
      }

      drv.gl.glBindTexture(GL_TEXTURE_2D, oldBinding);
      drv.gl.glDeleteTextures(1, &texture);
    }
#endif
    presenceChecked = true;
  }

  return presence;
}

bool IsGlGetCompressedTexImagePresentOnGLES() {
  static bool presenceChecked = false;
  static bool presence = false;

  if (!presenceChecked) {
#ifdef GITS_PLATFORM_WINDOWS
    bool useGlGetTexImageAndRestoreBuffersWhenPossibleES =
        Configurator::Get().opengl.recorder.useGlGetTexImageAndRestoreBuffersWhenPossibleES;

    if (useGlGetTexImageAndRestoreBuffersWhenPossibleES &&
        check_gl_function_availability("glGetCompressedTexImage")) {
      GLint oldBinding = 0;
      drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding);

      GLint compressedFormatsCount = 0;
      drv.gl.glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressedFormatsCount);

      std::vector<GLint> compressedFormats(compressedFormatsCount);
      drv.gl.glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &compressedFormats[0]);

      GLint formatToCheck = 0;
      if (std::find(compressedFormats.begin(), compressedFormats.end(), GL_ETC1_RGB8_OES) !=
          compressedFormats.end()) {
        formatToCheck = GL_ETC1_RGB8_OES;
      } else if (std::find(compressedFormats.begin(), compressedFormats.end(),
                           GL_COMPRESSED_RGB8_ETC2) != compressedFormats.end()) {
        formatToCheck = GL_COMPRESSED_RGB8_ETC2;
      }

      if (formatToCheck == 0) {
        LOG_INFO << "Unable to find proper format for glGetCompressedTexImage presence check";
        presenceChecked = true;
        return presence;
      }

      std::vector<GLubyte> bytesWrite(8, 0xA5);
      std::vector<GLubyte> bytesRead(8, 0x00);

      GLuint texture = 0;
      drv.gl.glGenTextures(1, &texture);
      drv.gl.glBindTexture(GL_TEXTURE_2D, texture);
      drv.gl.glCompressedTexImage2D(GL_TEXTURE_2D, 0, formatToCheck, 4, 4, 0, 8, &bytesWrite[0]);
      drv.gl.glGetCompressedTexImage(GL_TEXTURE_2D, 0, &bytesRead[0]);

      presence = true;
      for (unsigned int i = 0; i < bytesWrite.size(); i++) {
        presence &= bytesWrite[i] == bytesRead[i];
      }

      if (presence) {
        LOG_INFO << "glGetCompressedTexImage seems to be present in OpenGL ES API";
      } else {
        LOG_INFO << "glGetCompressedTexImage seems to be unsupported on this OpenGL ES driver";
      }

      drv.gl.glBindTexture(GL_TEXTURE_2D, oldBinding);
      drv.gl.glDeleteTextures(1, &texture);
    }
#endif
    presenceChecked = true;
  }

  return presence;
}

bool IsGlGetTexAndCompressedTexImagePresentOnGLES() {
  return IsGlGetTexImagePresentOnGLES() && IsGlGetCompressedTexImagePresentOnGLES();
}

StatePrinter::RBO::RBO(GLint name) : _name(name) {
  RboBindStateStash rboBindStateStash;
  drv.gl.glBindRenderbuffer(GL_RENDERBUFFER, name);
  GLint param = 0;
  drv.gl.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &param);
  _internalFormat = param;
  drv.gl.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &param);
  _msaaSamples = param;
  rboBindStateStash.Restore();
}
void StatePrinter::RBO::Write() {
  LOG_TRACE << "GL_RENDERBUFFER - " << _name << " - Internal Format: " << _internalFormat
            << " - msaa: " << _msaaSamples;
}

StatePrinter::Texture::Texture(GLenum target, GLint name, GLenum texture)
    : _name(name), _target(target), _internalFormat(0), _msaaSamples(0), _texture(texture) {
  TextureBindStateStash texBindStateStash(target);
  if (curctx::IsOgl()) {
    drv.gl.glBindTexture(target, name);
    GLint param = 0;
    drv.gl.glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &param);
    _internalFormat = param;
    drv.gl.glGetTexLevelParameteriv(target, 0, GL_TEXTURE_SAMPLES, &param);
    _msaaSamples = param;
  }
#ifndef BUILD_FOR_CCODE
  else {
    CTextureStateObj* texObj =
        SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));
    _internalFormat = texObj->Data().track.mipmapData[0].internalFormat;
    _msaaSamples = texObj->Data().track.mipmapData[0].samples;
  }
#endif
  texBindStateStash.Restore();
}
void StatePrinter::Texture::Write() {
  if (_texture != 0) {
    LOG_TRACE << _target << " - " << _name << " Texture: " << _texture
              << " - Internal Format: " << _internalFormat << " - msaa: " << _msaaSamples;
  } else {
    LOG_TRACE << _target << " - " << _name << " - Internal Format: " << _internalFormat
              << " - msaa: " << _msaaSamples;
  }
}

StatePrinter::FBO::FBO(GLenum target, GLint name) : _name(name), _target(target) {
  //Get Attachments List
  std::vector<GLint> attachments;
  if (curctx::IsEs2Plus()) {
    attachments.push_back(GL_COLOR_ATTACHMENT0);
  } else if (curctx::IsOgl()) {
    GLint maxAttachments = 0;
    drv.gl.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttachments);
    for (int attach = 0; attach < maxAttachments; attach++) {
      GLint param = 0;
      drv.gl.glGetFramebufferAttachmentParameteriv(_target, GL_COLOR_ATTACHMENT0 + attach,
                                                   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);
      if (param != GL_NONE) {
        attachments.push_back(GL_COLOR_ATTACHMENT0 + attach);
      }
    }
  }
  if (!curctx::IsEs1()) {
    GLint param = 0;
    GLenum depthAttach = GL_DEPTH_STENCIL_ATTACHMENT;
    drv.gl.glGetFramebufferAttachmentParameteriv(_target, depthAttach,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);
    if (param == GL_NONE) {
      depthAttach = GL_DEPTH_ATTACHMENT;
      drv.gl.glGetFramebufferAttachmentParameteriv(_target, depthAttach,
                                                   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);
    }
    if (param != GL_NONE) {
      attachments.push_back(depthAttach);
    }
  }

  //Save attachments data
  for (auto attachm : attachments) {
    GLint atttype = 0;
    drv.gl.glGetFramebufferAttachmentParameteriv(_target, attachm,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &atttype);
    GLint attname = 0;
    drv.gl.glGetFramebufferAttachmentParameteriv(_target, attachm,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attname);
    if (atttype == GL_RENDERBUFFER) {
      _attachments[attachm].reset(new RBO(attname));
    } else if (atttype == GL_TEXTURE) {
      _attachments[attachm].reset(new Texture(GetTextureTarget(attname), attname, 0));
    }
  }
}
void StatePrinter::FBO::Write() {
  LOG_TRACE << _target << " - " << _name;
  for (auto& attachm : _attachments) {
    LOG_FORMAT_RAW
    LOG_TRACE << "  - " << attachm.first << " - ";
    attachm.second->Write();
  }
}

void StatePrinter::GLSLPrograms::Write() {
#ifndef BUILD_FOR_CCODE
  CGLSLProgramStateObj* programStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLPrograms().Get(_name);
  if (programStateObjPtr != nullptr) {
    int printedShaders = 0;
    std::vector<GLuint> attachedShaders = programStateObjPtr->AttachedShaders();
    if (attachedShaders.size() > 0) {
      for (auto shader : attachedShaders) {
        CGLSLShaderStateObj* shaderStateObjPtr =
            SD().GetCurrentSharedStateData().GLSLShaders().Get(shader);
        if (shaderStateObjPtr) {
          LOG_TRACE << "GL_PROGRAM: " << _name << " GL_SHADER:" << shaderStateObjPtr->Name()
                    << " SHADER FILE NAME: " << shaderStateObjPtr->GetShaderName();
          printedShaders++;
        }
      }
    }
    if (!printedShaders) {
      LOG_TRACE << "GL_PROGRAM: " << _name;
    }
  }
#else
    LOG_TRACE << "GL_PROGRAM: " << _name;
#endif
}

void StatePrinter::GLSLPipelines::Write() {
#ifndef BUILD_FOR_CCODE
  CGLSLPipelineStateObj* pipelineStateObjPtr =
      SD().GetCurrentContextStateData().GLSLPipelines().Get(_name);
  if (pipelineStateObjPtr != nullptr) {
    for (auto& stage : pipelineStateObjPtr->Data().track.stages) {
      const CGLSLProgramStateData& progData = *stage.second;
      LOG_TRACE << "GL_PIPELINE: " << _name << " GL_PROGRAM: " << progData.track.name;
    }
  }
#else
    LOG_TRACE << "GL_PIPELINE: " << _name;
#endif
}

void StatePrinter::ARBProgram::Write() {
  LOG_TRACE << _target << " - name: " << _name;
}

void StatePrinter::BoundBuffers::Write() {
  LOG_TRACE << "Bound Buffer: Target: " << _target << " Buffer: " << _buffer;
}

StatePrinter::StatePrinter() {
  CGits::Instance().traceGLAPIBypass = true;

  //FBOs
  if (!curctx::IsEs1()) {
    FboBindStateStash fboBindStateStash;
    if (fboBindStateStash.DrawName() != 0) {
      _objects.push_back(std::shared_ptr<Object>(
          new FBO(fboBindStateStash.DrawTarget(), fboBindStateStash.DrawName())));
    }
    if (fboBindStateStash.ReadName() != 0) {
      _objects.push_back(std::shared_ptr<Object>(
          new FBO(fboBindStateStash.ReadTarget(), fboBindStateStash.ReadName())));
    }
  }
  //Textures
  //GLenum texTargets[] = {GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_3D, GL_TEXTURE_RECTANGLE};
  GLenum texTargets[] = {GL_TEXTURE_1D,
                         GL_TEXTURE_2D,
                         GL_TEXTURE_3D,
                         GL_TEXTURE_1D_ARRAY,
                         GL_TEXTURE_2D_ARRAY,
                         GL_TEXTURE_RECTANGLE,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                         GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                         GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                         GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
  GLint activeTexUnit = 0;
  drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexUnit);
  GLint maxTexUnits = 0;
  if (curctx::IsEs1()) {
    drv.gl.glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTexUnits);
  } else {
    drv.gl.glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
  }

  for (GLenum target : texTargets) {
    for (int i = 0; i < maxTexUnits; i++) {
      drv.gl.glActiveTexture(GL_TEXTURE0 + i);
      GLint name;
      drv.gl.glGetIntegerv(GetBindingEnum(target), &name);
      if (name > 0) {
        _objects.push_back(std::shared_ptr<Object>(new Texture(target, name, GL_TEXTURE0 + i)));
      }
    }
  }

  drv.gl.glActiveTexture(activeTexUnit);

  if (!curctx::IsEs1()) {
    //GLSL Pipelines
    GLint glslPipeline = 0;
    drv.gl.glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &glslPipeline);
    if (glslPipeline > 0) {
      _objects.push_back(std::shared_ptr<Object>(new GLSLPipelines(glslPipeline)));
    }

    //GLSL Programs
    GLint glslProg = 0;
    drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &glslProg);
    if (glslProg > 0) {
      _objects.push_back(std::shared_ptr<Object>(new GLSLPrograms(glslProg)));
    }
  }
  //ARB Program
  if (curctx::IsOgl()) {
    GLenum arbTargets[] = {GL_FRAGMENT_PROGRAM_ARB, GL_VERTEX_PROGRAM_ARB};
    for (GLenum target : arbTargets) {
      GLint name;
      drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &name);
      if (name > 0) {
        _objects.push_back(std::shared_ptr<Object>(new ARBProgram(target, name)));
      }
    }
  }

#ifndef BUILD_FOR_CCODE
  //BoundBuffers
  auto& boundBuffers = SD().GetCurrentContextStateData().Bindings().BoundBuffers();
  for (auto& elem : boundBuffers) {
    if (elem.second != 0) {
      _objects.push_back(std::shared_ptr<Object>(new BoundBuffers(elem.first, elem.second)));
    }
  }
#endif

  CGits::Instance().traceGLAPIBypass = false;
}
void StatePrinter::PrintToLog() {
#ifndef BUILD_FOR_CCODE
  std::map<GLuint, hash_t> bufferHashses;
  if (Configurator::Get().opengl.player.traceGLBufferHashes[CGits::Instance().CurrentDrawCount()]) {
    bufferHashses = CurrentAttribsBuffersHash();
  }
#endif
  if (_objects.size() > 0) {
    LOG_TRACE << "/**********************CURRENT BOUND STATE************************/";
    LOG_TRACE << " ";
    for (auto& object : _objects) {
      object->Write();
      LOG_TRACE << " ";
    }
  }
#ifndef BUILD_FOR_CCODE
  if (Configurator::Get().opengl.player.traceGLBufferHashes[CGits::Instance().CurrentDrawCount()]) {
    LOG_TRACE << "Buffer objects used for rendering";
    for (const auto& elem : bufferHashses) {
      LOG_TRACE << elem.first << " - " << std::hex << elem.second;
    }
  }
#endif
  if (_objects.size() > 0) {
    LOG_TRACE << "/*****************************************************************/";
  }
}

template <>
std::string ToStr<GLenum>(const GLenum& value) {
  //This operator converts all unsigned int values higher then threshold (1000) into enum string. As we can't differentiate between GLuint
  //and GLenum types there is an issue that GLuint values higher then threshold are converted to GLenum string and lower GLenum are not converted at all.
  if (value > 1000) {
    return GetGLEnumString(value);
  }
  return std::to_string(value);
}

template <>
std::string ToStr<GLboolean>(const GLboolean& value) {
  return value ? "GL_TRUE" : "GL_FALSE";
}

static std::string toStrUCharPtr(const unsigned char* c) {
  std::stringstream ss;
  if (c != nullptr) {
    uint64_t pageSize = 4 * 1024;
    const unsigned char* pageEndPtr =
        (const unsigned char*)((((uint64_t)c) / pageSize + 1) * pageSize - 1);
    const unsigned char* ptr = c;

    // Check the rest of current memory page. If it contains chars with
    // values lower than 127 and there is null termination we assume that it
    // is a string. Otherwise we treat it as a pointer to data.
    while (ptr <= pageEndPtr) {
      if (*ptr == 0) {
        ss << c;
        return ss.str();
      } else if (*ptr > 127) {
        ss << (const void*)c;
        return ss.str();
      }
      ptr++;
    }
  }

  ss << (const void*)c;
  return ss.str();
}

template <>
std::string ToStr<unsigned char*>(unsigned char* const& value) {
  return toStrUCharPtr(value);
}

namespace {
std::string DescribePixelFormatFlags(DWORD dwFlags) {
  std::string retval;
  const std::unordered_map<DWORD, std::string> flagMap = {
      {PFD_DOUBLEBUFFER, "PFD_DOUBLEBUFFER"},
      {PFD_STEREO, "PFD_STEREO"},
      {PFD_DRAW_TO_WINDOW, "PFD_DRAW_TO_WINDOW"},
      {PFD_DRAW_TO_BITMAP, "PFD_DRAW_TO_BITMAP"},
      {PFD_SUPPORT_GDI, "PFD_SUPPORT_GDI"},
      {PFD_SUPPORT_OPENGL, "PFD_SUPPORT_OPENGL"},
      {PFD_GENERIC_FORMAT, "PFD_GENERIC_FORMAT"},
      {PFD_NEED_PALETTE, "PFD_NEED_PALETTE"},
      {PFD_NEED_SYSTEM_PALETTE, "PFD_NEED_SYSTEM_PALETTE"},
      {PFD_SWAP_EXCHANGE, "PFD_SWAP_EXCHANGE"},
      {PFD_SWAP_COPY, "PFD_SWAP_COPY"},
      {PFD_SWAP_LAYER_BUFFERS, "PFD_SWAP_LAYER_BUFFERS"},
      {PFD_GENERIC_ACCELERATED, "PFD_GENERIC_ACCELERATED"},
      {PFD_SUPPORT_DIRECTDRAW, "PFD_SUPPORT_DIRECTDRAW"},
      {PFD_DIRECT3D_ACCELERATED, "PFD_DIRECT3D_ACCELERATED"},
      {PFD_SUPPORT_COMPOSITION, "PFD_SUPPORT_COMPOSITION"},
      {PFD_DEPTH_DONTCARE, "PFD_DEPTH_DONTCARE"},
      {PFD_DOUBLEBUFFER_DONTCARE, "PFD_DOUBLEBUFFER_DONTCARE"},
      {PFD_STEREO_DONTCARE, "PFD_STEREO_DONTCARE"}};

  for (const auto& kv : flagMap) {
    const DWORD key = kv.first;
    const std::string& value = kv.second;

    if (dwFlags & key) {
      retval.append(value).append(" | ");
    }
  }

  // Trim the trailing " | ".
  retval.erase(retval.find_last_not_of(" |") + 1);

  return retval;
}

std::string DescribePixelType(BYTE iPixelType) {
  std::string retval;

  switch (iPixelType) {
  case PFD_TYPE_RGBA:
    retval = "PFD_TYPE_RGBA";
    break;
  case PFD_TYPE_COLORINDEX:
    retval = "PFD_TYPE_COLORINDEX";
    break;
  default:
    retval = "Unknown pixel type (";
    retval += std::to_string(iPixelType) + ")";
  }

  return retval;
}

std::string DescribeLayerType(BYTE iLayerType) {
  std::string retval;

  switch (iLayerType) {
  case PFD_MAIN_PLANE:
    retval = "PFD_MAIN_PLANE";
    break;
  case PFD_OVERLAY_PLANE:
    retval = "PFD_OVERLAY_PLANE";
    break;
  // It's negative so cast to avoid warnings about unsigned BYTE.
  case (BYTE)PFD_UNDERLAY_PLANE:
    retval = "PFD_UNDERLAY_PLANE";
    break;
  default:
    retval = "Unknown layer type (";
    retval += std::to_string(iLayerType) + ")";
  }

  return retval;
}
} // namespace

template <>
std::string ToStr<PIXELFORMATDESCRIPTOR>(const PIXELFORMATDESCRIPTOR& pfd) {
  std::ostringstream oss;
  oss << std::showbase;
  // clang-format off
  oss << "{ \n"
      << "pfd.nSize: " << pfd.nSize << ", \n"
      << "pfd.nVersion: " << pfd.nVersion << ", \n"
      << "pfd.dwFlags: " << DescribePixelFormatFlags(pfd.dwFlags) << ", \n"
      << "pfd.iPixelType: " << DescribePixelType(pfd.iPixelType) << ", \n"
      // Use to_string so they get interpreted as numbers, not chars.
      << "pfd.cColorBits: " << std::to_string(pfd.cColorBits) << ", \n"
      << "pfd.cRedBits: " << std::to_string(pfd.cRedBits) << ", \n"
      << "pfd.cRedShift: " << std::to_string(pfd.cRedShift) << ", \n"
      << "pfd.cGreenBits: " << std::to_string(pfd.cGreenBits) << ", \n"
      << "pfd.cGreenShift: " << std::to_string(pfd.cGreenShift) << ", \n"
      << "pfd.cBlueBits: " << std::to_string(pfd.cBlueBits) << ", \n"
      << "pfd.cBlueShift: " << std::to_string(pfd.cBlueShift) << ", \n"
      << "pfd.cAlphaBits: " << std::to_string(pfd.cAlphaBits) << ", \n"
      << "pfd.cAlphaShift: " << std::to_string(pfd.cAlphaShift) << ", \n"
      << "pfd.cAccumBits: " << std::to_string(pfd.cAccumBits) << ", \n"
      << "pfd.cAccumRedBits: " << std::to_string(pfd.cAccumRedBits) << ", \n"
      << "pfd.cAccumGreenBits: " << std::to_string(pfd.cAccumGreenBits) << ", \n"
      << "pfd.cAccumBlueBits: " << std::to_string(pfd.cAccumBlueBits) << ", \n"
      << "pfd.cAccumAlphaBits: " << std::to_string(pfd.cAccumAlphaBits) << ", \n"
      << "pfd.cDepthBits: " << std::to_string(pfd.cDepthBits) << ", \n"
      << "pfd.cStencilBits: " << std::to_string(pfd.cStencilBits) << ", \n"
      << "pfd.cAuxBuffers: " << std::to_string(pfd.cAuxBuffers) << ", \n"
      << "pfd.iLayerType: " << DescribeLayerType(pfd.iLayerType) << ", \n"
      << "pfd.bReserved: " << std::to_string(pfd.bReserved) << ", \n"
      << "pfd.dwLayerMask: " << pfd.dwLayerMask << ", \n"
      << "pfd.dwVisibleMask: " << pfd.dwVisibleMask << ", \n"
      << "pfd.dwDamageMask: " << pfd.dwDamageMask << " \n}";
  // clang-format on

  return std::move(oss).str();
}

std::string GetCurrentProgramShaderText(GLenum shtype) {
#ifndef BUILD_FOR_CCODE
  GLuint program = SD().GetCurrentContextStateData().Bindings().GLSLProgram();
  if (program == 0) {
    if (curctx::IsOgl()) {
      GLuint pipeline = SD().GetCurrentContextStateData().Bindings().GLSLPipeline();
      if (pipeline == 0) {
        return "";
      }
      drv.gl.glGetProgramPipelineiv(pipeline, shtype, reinterpret_cast<GLint*>(&program));
      if (program == 0) {
        return "";
      }
    } else {
      return "";
    }
  }
  auto programPtr = SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
  if (programPtr == nullptr) {
    return "";
  }
  auto& linked_shaders = programPtr->Data().track.linkedShaders;

  if (SD().GetCurrentSharedStateData().GLSLPrograms().Get(program)->Data().track.linked) {
    for (auto& linked_shader : linked_shaders) {
      if (linked_shader.second.track.type == shtype) {
        return linked_shader.second.track.source;
      }
    }
  }
#endif
  return "";
}

std::string GetShaderSource(GLint name) {
#ifndef BUILD_FOR_CCODE
  CGLSLShaderStateObj* shader = SD().GetCurrentSharedStateData().GLSLShaders().Get(name);
  if (shader) {
    return shader->Data().track.source;
  }
#endif
  return "";
}

void CleanResources() {
#ifndef BUILD_FOR_CCODE
  bool returnSCC;
  for (auto context : SD().GetContextVector()) {
    returnSCC = SetCurrentContext(context);
    if (returnSCC != false) {
      std::vector<GLuint> names;
      for (GLuint i = 1; i <= 2000; i++) {
        names.push_back(i);
      }

      drv.gl.glDeleteTextures((GLsizei)names.size(), &names[0]);
      drv.gl.glDeleteBuffers((GLsizei)names.size(), &names[0]);
      if (!curctx::IsEs1()) {
        for (auto name : names) {
          drv.gl.glUseProgram(0);
          drv.gl.glDeleteProgram(name);
          drv.gl.glDeleteShader(name);
        }
      }
      if (curctx::IsOgl()) {
        drv.gl.glDeleteProgramsARB((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteLists(names[0], (GLsizei)names.size());
      }

      if (curctx::IsOgl() || curctx::IsEs3Plus()) {
        drv.gl.glDeleteVertexArrays((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteFramebuffers((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteRenderbuffers((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteQueries((GLsizei)names.size(), &names[0]);
        if (curctx::IsOgl()) {
          drv.gl.glDeleteFramebuffersEXT((GLsizei)names.size(), &names[0]);
          drv.gl.glDeleteRenderbuffersEXT((GLsizei)names.size(), &names[0]);
        }
      } else {
        drv.gl.glDeleteFramebuffersOES((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteRenderbuffersOES((GLsizei)names.size(), &names[0]);
        drv.gl.glDeleteQueriesEXT((GLsizei)names.size(), &names[0]);
      }
      drv.gl.glFlush();
      drv.gl.glFinish();
    } else {
      LOG_WARNING << "Resources cleanup failed. SetCurrentContext returned 0.";
    }
  }
#endif
}

void DestroyContext(void* ctx) {
#ifndef BUILD_FOR_CCODE
#ifdef GITS_PLATFORM_WINDOWS
  // use EGL when libEGL.dll is loaded
  if (drv.gl.Api() == CGlDriver::API_GLES1 || drv.gl.Api() == CGlDriver::API_GLES2) {
    drv.egl.eglDestroyContext(drv.egl.eglGetDisplay((EGLNativeDisplayType)GetNativeDisplay()),
                              (EGLContext)ctx);
  }
  // use WGL when OpenGL32.dll is loaded
  else if (drv.gl.Api() == CGlDriver::API_GL || curctx::IsOgl()) {
    drv.wgl.wglDeleteContext((HGLRC)ctx);
  }
  // this code should never be reached
  else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
#elif defined GITS_PLATFORM_X11
  if (curctx::IsOgl()) {
    drv.glx.glXDestroyContext((Display*)GetNativeDisplay(), (GLXContext)ctx);
  } else {
    drv.egl.eglDestroyContext(drv.egl.eglGetDisplay((EGLNativeDisplayType)GetNativeDisplay()),
                              (EGLContext)ctx);
  }
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
#endif
}

void DestroyAllContexts() {
#ifndef BUILD_FOR_CCODE
  auto ctxsVector = SD().GetContextVector();
  for (auto ctx : ctxsVector) {
    DestroyContext(ctx);
  }

  if (!curctx::IsOgl()) {
    drv.egl.eglTerminate(drv.egl.eglGetDisplay((EGLNativeDisplayType)GetNativeDisplay()));
  }
#endif
}

void GetUniformArraySizeAndOffset(
    GLuint program, const GLchar* name, GLint location, GLint& arraySize, GLint& arrayIndex) {
  GLint uniform_num = 0, name_max = 0;
  drv.gl.glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_num);
  drv.gl.glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &name_max);

  auto chop_name = [](char* name, size_t size) {
    for (size_t i = size; i > 0; --i) {
      if (name[i - 1] == '.') {
        break;
      }
      if (name[i - 1] != '[') {
        continue;
      }
      name[i - 1] = 0;
      break;
    }
  };

  arrayIndex = -1;
  arraySize = -1;

  std::vector<char> name_chopped(name, name + strlen(name) + 1);
  chop_name(&name_chopped[0], name_chopped.size());

  std::vector<char> nameb(name_max + 1);
  for (int i = 0; i < uniform_num; ++i) {
    GLsizei name_len = 0;
    GLint size = 0;
    GLenum type = 0;
    drv.gl.glGetActiveUniform(program, i, name_max, &name_len, &size, &type, &nameb[0]);
    chop_name(&nameb[0], name_len);
    if (strcmp(&nameb[0], &name_chopped[0]) == 0) {
      auto base_loc = drv.gl.glGetUniformLocation(program, &nameb[0]);
      arrayIndex = location - base_loc;
      arraySize = size;
      break;
    }
  }
}

size_t GetPatchParameterValuesCount(GLenum pname) {
  switch (pname) {
  case GL_PATCH_DEFAULT_OUTER_LEVEL:
    // `float[4] values` defines the four outer tessellation levels.
    return 4;
  case GL_PATCH_DEFAULT_INNER_LEVEL:
    // `float[2] values` defines the two inner tessellation levels.
    return 2;
  default:
    // GL_PATCH_VERTICES should be used only in glPatchParameteri.
    // Other enums should not be used in glPatchParameter* functions at all.
    LOG_ERROR << "Unexpected parameter name in glPatchParameterfv: " << pname;
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }
}

#ifndef BUILD_FOR_CCODE
// TODO: For CCode, we could use this: https://stackoverflow.com/a/26218031
// `glGetTextureParameteriv(textureId, GL_TEXTURE_TARGET, &target);`
GLenum GetTargetOfTextureOrCrash(GLuint texName) {
  const CTextureStateObj* const texState = SD().GetCurrentSharedStateData().Textures().Get(texName);
  if (texState != nullptr) {
    return texState->Target();
  } else {
    LOG_ERROR << "Texture not found, likely an issue with state tracking.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}
#endif
bool isTrackTextureBindingWAUsed() {
#ifdef GITS_PLATFORM_WINDOWS
  return Configurator::Get().opengl.recorder.trackTextureBindingWA;
#else
    return false;
#endif
}
bool isSchedulefboEXTAsCoreWA() {
#ifdef GITS_PLATFORM_WINDOWS
  return Configurator::Get().opengl.recorder.schedulefboEXTAsCoreWA;
#else
    return false;
#endif
}
} // namespace OpenGL
} // namespace gits
