// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Free of glTypes.h, so that its implementation can include Xlib, whose Bool and XVisualInfo
// conflict with glTypes.h's. Drawables are GLXDrawable values.

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

struct _XDisplay;
typedef struct _XDisplay Display;

namespace gits::opengl {

// Substitutes the player's X connection and drawables for the recorded ones in GLX calls. Each
// recorded X window gets a player window of its own, which follows the window's recorded
// geometry, or is sized from glViewport calls when none was recorded for it. GLX windows, pbuffers
// and pixmaps are substituted by the ones the player creates in their place. Each GLX pixmap gets
// an X pixmap of its own, of the recorded X pixmap's size.
class GlxDrawableService {
public:
  static GlxDrawableService& Get();

  // The player's X connection. X errors on it are logged instead of ending the process.
  Display* GetDisplay();
  int GetDefaultScreen();
  // Frees memory GLX returned, with XFree.
  static void Free(void* data);
  // The player drawable of a recorded drawable: the substitute of a GLX window or pbuffer, or
  // else a player window, as a GLX drawable's pixel format is its X window's visual. A caller
  // binding the window, by making a context current on it or creating a GLX window for it,
  // passes the `visualId` it needs, and other callers 0. A window is created on first use, with
  // the default visual for 0. The first binding with a different visual replaces it by a new
  // one with that visual and hides it until the end of replay, as X windows cannot change their
  // visual, which loses what was set on it before, such as its swap interval. 0 resolves to 0.
  uint64_t Resolve(uint64_t recordedDrawable, uint64_t visualId = 0);
  // False for a recorded pbuffer or GLX pixmap, true for an X or GLX window.
  bool IsWindow(uint64_t recordedDrawable);
  // Makes `liveDrawable` the substitute of a recorded GLX window or pbuffer until removed. A GLX
  // window's `recordedWindow` is the recorded X window it was created for, and 0 for a pbuffer.
  void SetSubstitute(uint64_t recordedDrawable, uint64_t liveDrawable, uint64_t recordedWindow);
  // Creates an X pixmap of the size recorded for the X pixmap `recordedPixmap`, or returns 0 and
  // logs an error when none was recorded. Its depth is that of `visualInfo`, an XVisualInfo, or
  // the default screen's without one, as GLX takes a pixmap of any depth for a configuration
  // without a visual. The recorded pixmap's depth is not recorded.
  uint64_t CreatePixmap(uint64_t recordedPixmap, const void* visualInfo);
  // Makes `liveGlxPixmap`, created for the X pixmap `pixmap` from CreatePixmap, the substitute of
  // a recorded GLX pixmap until removed, which frees `pixmap`. Frees `pixmap` right away when
  // `liveGlxPixmap` is 0.
  void SetPixmapSubstitute(uint64_t recordedGlxPixmap, uint64_t liveGlxPixmap, uint64_t pixmap);
  void RemoveSubstitute(uint64_t recordedDrawable);
  // Applies the recorded geometry of a drawable to its player window, or to the window it gets.
  void SetGeometry(uint64_t recordedDrawable, int x, int y, int width, int height, bool visible);
  // Hides the player windows of recorded X windows reported destroyed and forgets them, so that a
  // later use of a recorded window gets a new player window. The windowing library keeps hidden
  // windows until the end of replay.
  void RemoveWindows(const std::vector<uint64_t>& recordedWindows);
  // Sizes the player window `liveDrawable` for a `width` x `height` viewport, unless geometry was
  // recorded for it: the first one sets its size, which later ones only grow, as applications
  // usually first set the viewport to the whole window.
  void FitViewport(uint64_t liveDrawable, int width, int height);

private:
  GlxDrawableService() = default;

  struct PlayerWindow {
    uint64_t NativeDisplay{};
    uint64_t Drawable{};
    uint64_t VisualId{};
    bool Bound{false};
    int X{};
    int Y{};
    int Width{640};
    int Height{480};
    // Visible by default, as a stream without geometry has nothing to show it later.
    bool Visible{true};
    bool HasGeometry{false};
    bool Fitted{false};
  };

  struct Substitute {
    uint64_t Drawable{};
    uint64_t RecordedWindow{};
    uint64_t Pixmap{};
  };

  void Create(PlayerWindow& window, uint64_t visualId);
  void FreePixmap(uint64_t pixmap);

  std::mutex m_Mutex;
  std::unordered_map<uint64_t, PlayerWindow> m_Windows;
  std::unordered_map<uint64_t, Substitute> m_Substitutes;
};

} // namespace gits::opengl
