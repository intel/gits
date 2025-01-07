// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   display.h
 * 
 * @brief Declaration of a base class for display implementation.
 * 
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <string>

namespace gits {

class CWindow;
class CWindowInfo;
class CPlayer;

/**
   * @brief Base class for graphic display implementation.
   * 
   * gits::CDisplay is a base class for graphic display implementation.
   * It provides methods to initiate display and for creating
   * Operating System window that will present captured calls.
   */
class CDisplay {
  CPlayer& _player;

public:
  CDisplay(CPlayer& player);
  virtual ~CDisplay();

  CPlayer& Player() const {
    return _player;
  }

  /**
     * @brief Initiates the display
     *
     * Method initiates display. It should be called only once.
     *
     * @param argc Number of not parsed command line parameters.
     * @param argv Array of not parsed command line parameters.
     *
     * @exception EOperationFailed Display initialization failed
     */
  virtual void Init(int& argc, char* argv[]) = 0;

  /**
     * @brief Creates library specific display window
     * 
     * Method creates library library specific display window.
     * 
     * @param title Title of the window to create
     * @param info Information of window to create
     * @param player Player to use
     *
     * @return Library specific display window.
     */
  virtual CWindow* WindowCreate(const std::string& title,
                                const CWindowInfo& info,
                                unsigned deviceId,
                                unsigned rendererId) const = 0;

  virtual CWindow* WindowMakeCurrent(unsigned deviceId, unsigned rendererId) const = 0;

  virtual void WindowDestroy(unsigned deviceId) const = 0;

  virtual void WindowDestroyAll() const = 0;

  /**
     * @brief Starts display environment main loop
     *
     * Method starts display environment main loop.
     */
  virtual void MainLoopRun() const = 0;

  virtual void MainLoopExit() const = 0;
};

} // namespace gits

#endif /* __DISPLAY_H__ */
