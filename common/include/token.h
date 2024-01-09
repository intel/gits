// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   token.h
 *
 * @brief Declaration of token (subject of capture/playback operation).
 *
 */

#pragma once

#include "id.h"
#include "tools.h"

namespace gits {
class CBinOStream;
class CBinIStream;
class CCodeOStream;

/**
   * @brief Capture/playback token
   *
   * gits::CToken class contains unique identifier of the token.
   * This is a base class for each entities that may be captured and played back
   * (e.g frame start, function calls, frame end).
   */
class CToken {
public:
  enum TId {
    ID_HELPER_TOKENS = 0 * 0x10000,
    ID_INIT_START,       /**< @brief new sequence state initialization start */
    ID_INIT_END,         /**< @brief new sequence state initialization end */
    ID_FRAME_START,      /**< @brief frame capture has started */
    ID_FRAME_END,        /**< @brief frame capture has ended */
    ID_PRE_RECORD_START, /**< @brief prerecording phase start*/
    ID_PRE_RECORD_END,   /**< @brief prerecording phase end*/
    ID_PLAYER_RECORDER_SYNC,
    ID_MAKE_CURRENT_THREAD,
    ID_REC_SCREEN_RESOLUTION,
    ID_MAKE_CURRENT_THREAD_NO_CTX_SWITCH,
    ID_CCODE_FINISH,

    ID_OPENGL = 1 * 0x10000,
    ID_GL_HELPER_TOKENS = 2 * 0x10000,
    ID_WGL = 3 * 0x10000,
    ID_GLX = 5 * 0x10000,
    ID_EGL = 6 * 0x10000,
    ID_OPENCL = 7 * 0x10000,
    ID_VULKAN = 10 * 0x10000,
    ID_LEVELZERO = 11 * 0x10000,
    ID_OCLOC = 12 * 0x10000,
  };
  virtual ~CToken() = 0;

  virtual unsigned Id() const = 0;
  virtual unsigned Type() const {
    return 0;
  }
  virtual void Run() = 0;
  virtual void Exec(){};
  virtual void StateTrack(){};

  void Serialize(CBinOStream& stream);
  void Serialize(CCodeOStream& stream);
  static CToken* Deserialize(CBinIStream& stream, CToken* (*ctor)(CId));

private:
  virtual void Write(CBinOStream& stream) const = 0;
  virtual void Write(CCodeOStream& stream) const = 0;
  virtual void Read(CBinIStream& stream) = 0;
  bool _isSerialized = false;

public:
  bool IsSerialized() const {
    return _isSerialized;
  }
  static void* operator new(size_t size);
  static void operator delete(void* pointer);
};

class CTokenFrameNumber : public CToken {
  TId _id;
  uint32_t _frameNumber;

public:
  CTokenFrameNumber(TId id, unsigned frameNumber);
  CTokenFrameNumber(TId id);

  virtual unsigned Id() const {
    return _id;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

class CTokenPlayerRecorderSync : public CToken {
  uint64_t _timeStamp;

public:
  CTokenPlayerRecorderSync();
  virtual unsigned Id() const {
    return ID_PLAYER_RECORDER_SYNC;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& /*stream*/) const {}
  virtual void Run();
};

// In multithreaded playback switches thread, in flattened one switches context.
class CTokenMakeCurrentThread : public CToken {
protected:
  int _threadId = 0;

public:
  CTokenMakeCurrentThread() = default;
  CTokenMakeCurrentThread(int threadid);
  virtual unsigned Id() const {
    return ID_MAKE_CURRENT_THREAD;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& /*stream*/) const {}
  virtual void Run();
};

// In multithreaded playback switches thread, in flattened one does nothing.
class CTokenMakeCurrentThreadNoCtxSwitch : public CToken {
protected:
  int _threadId = 0;

public:
  CTokenMakeCurrentThreadNoCtxSwitch() = default;
  CTokenMakeCurrentThreadNoCtxSwitch(int threadid);
  virtual unsigned Id() const {
    return ID_MAKE_CURRENT_THREAD_NO_CTX_SWITCH;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& /*stream*/) const {}
  virtual void Run();
};

class CTokenScreenResolution : public CToken {
  int32_t _screenWidth = 0;
  int32_t _screenHeight = 0;

public:
  CTokenScreenResolution() = default;
  CTokenScreenResolution(int width, int height);
  virtual unsigned Id() const {
    return ID_REC_SCREEN_RESOLUTION;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& /*stream*/) const {}
  virtual void Run();
};

} // namespace gits
