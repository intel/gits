// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
    ID_INIT_START = 1,       /**< @brief new sequence state initialization start */
    ID_INIT_END = 2,         /**< @brief new sequence state initialization end */
    ID_FRAME_START = 3,      /**< @brief frame capture has started */
    ID_FRAME_END = 4,        /**< @brief frame capture has ended */
    ID_PRE_RECORD_START = 5, /**< @brief prerecording phase start*/
    ID_PRE_RECORD_END = 6,   /**< @brief prerecording phase end*/
    ID_PLAYER_RECORDER_SYNC = 7,
    // Moved to OpenGL tokens
    ID_REC_SCREEN_RESOLUTION = 9,
    // Moved to OpenGL tokens
    ID_CCODE_FINISH = 11, // DEPRECATED: Kept for backward compatibility only
    ID_MARKER_UINT64 = 12,

    ID_OPENGL = 1 * 0x10000,
    ID_GL_HELPER_TOKENS = 2 * 0x10000,
    ID_WGL = 3 * 0x10000,
    ID_GLX = 5 * 0x10000,
    ID_EGL = 6 * 0x10000,
    ID_OPENCL = 7 * 0x10000,
    ID_VULKAN = 10 * 0x10000,
    ID_LEVELZERO = 11 * 0x10000,
    ID_OCLOC = 12 * 0x10000,
    ID_DirectX = 13 * 0x10000,
  };
  virtual ~CToken() = 0;

  virtual unsigned Id() const = 0;
  virtual unsigned Type() const {
    return 0;
  }
  virtual void Run() = 0;
  virtual void Exec(){};
  virtual void StateTrack(){};
  virtual uint64_t Size() const {
    return CId::Size;
  }

  void Serialize(CBinOStream& stream);
  static CToken* Deserialize(CBinIStream& stream, CToken* (*ctor)(CId));

private:
  virtual void Write(CBinOStream& stream) const = 0;
  virtual void Read(CBinIStream& stream) = 0;
  bool _isSerialized = false;

public:
  bool IsSerialized() const {
    return _isSerialized;
  }
  static void* operator new(size_t size);
  static void operator delete(void* pointer);
};

class CTokenMarker : public CToken {
  TId _id;

public:
  CTokenMarker(TId id);
  virtual unsigned Id() const {
    return _id;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Run();
};

class CTokenMarkerUInt64 : public CToken {
protected:
  uint64_t _value = 0;

public:
  // APIs can use values higher than 0x10000
  static const uint64_t COMMON_RESERVED = 0x10000;

  CTokenMarkerUInt64() = default;
  CTokenMarkerUInt64(uint64_t value);
  virtual unsigned Id() const {
    return ID_MARKER_UINT64;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Run();
  virtual uint64_t Size() const;
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
  virtual void Run();
  virtual uint64_t Size() const;
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
  virtual void Run();
  virtual uint64_t Size() const;
};

} // namespace gits
