// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "InputListener.h"
#include "keyEvents.h"
#include "tools.h"

#include <algorithm>

namespace InputListener {
int CInputListener::AddHotKey(const std::vector<uint>& keys) {
  CInputListener::HotKeyHolder hotKey(keys, _maxRegisteredHotKeyID);
  _pressedHistory[_maxRegisteredHotKeyID] = false;
  _hotKeys.push_back(std::move(hotKey));
  return _maxRegisteredHotKeyID++;
}
void CInputListener::AddHotKeyEvent(uint hotKeyId, HotKeyHolder::HotKeyEvent clickEvent) {
  auto hotKey = GetHotKey(hotKeyId);
  if (hotKey) {
    hotKey->AddClickedEvent(std::move(clickEvent));
  }
}

std::optional<CInputListener::HotKeyHolder> CInputListener::GetHotKey(const uint hotKeyId) {
  auto keyHolderIterator = std::find_if(begin(_hotKeys), end(_hotKeys),
                                        [hotKeyId](const CInputListener::HotKeyHolder& keyHolder) {
                                          return keyHolder.Id() == hotKeyId;
                                        });
  if (keyHolderIterator != end(_hotKeys)) {
    return *keyHolderIterator;
  }
  return std::nullopt;
}

void CInputListener::SetAsPressed(uint hotKeyId) {
  _pressedHistory[hotKeyId] = true;
}

#ifdef GITS_PLATFORM_WINDOWS

DWORD WINAPI KeyListenerThread(LPVOID lParam) {
  CInputListener& inputListener = *(CInputListener*)lParam;
  inputListener.RegisterHotKeys();
  MSG msg = {0};
  while (GetMessage(&msg, NULL, 0, 0) != 0) {
    switch (msg.message) {
    case -1:
      break;
    case WM_HOTKEY:
      uint hotKeyId = (uint)msg.wParam;
      auto keyHolder = inputListener.GetHotKey(hotKeyId);
      if (keyHolder) {
        inputListener.SetAsPressed(hotKeyId);
        keyHolder->CallClickedEvents();
      }
      break;
    }
  }

  inputListener.UnregisterHotKeys();
  return 0;
}

bool CInputListener::WasPressed(uint hotKeyId) {
  if (_useMessageLoop) {
    CALL_ONCE[&] {
      if (!_hotKeys.empty()) {
        StartHotKeyListener();
      }
    };
    bool isDown = _pressedHistory[hotKeyId];
    _pressedHistory[hotKeyId] = false;
    return isDown;
  } else {
    auto keyHolder = GetHotKey(hotKeyId);
    if (keyHolder) {
      return gits::AreAllKeysPressed(keyHolder->Keys());
    }
    return false;
  }
}
void CInputListener::StartHotKeyListener(bool useMessageLoop) {
  _useMessageLoop = useMessageLoop;
  if (_threadHandle == NULL) {
    _threadHandle = CreateThread(NULL, 0, KeyListenerThread, this, 0, NULL);
  }
}

CInputListener::~CInputListener() {
  if (_threadHandle != NULL) {
    PostThreadMessage(GetThreadId(_threadHandle), WM_QUIT, 0, 0);
    if (GetThreadId(_threadHandle) != GetCurrentThreadId()) {
      // The thread ids will be same when InputKeyListener calls ExitHotKeyPressed which in turn calls recorder.Close() for OCL
      WaitForSingleObject(_threadHandle, INFINITE);
    }
    CloseHandle(_threadHandle);
  }
}

void CInputListener::HotKeyHolder::UnregisterKey() {
  UnregisterHotKey(NULL, _id);
}

void CInputListener::HotKeyHolder::RegisterKey() {
  //divide keys to modifiers and other standard keys
  uint modifiers = 0;
  std::vector<uint> standardKey;
  for (uint key : _keys) {
    if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) {
      modifiers = MOD_SHIFT;
      continue;
    }
    if (key == VK_MENU || key == VK_LMENU || key == VK_RMENU) {
      modifiers = MOD_ALT;
      continue;
    }
    if (key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) {
      modifiers = MOD_CONTROL;
      continue;
    }
    if (key == VK_LWIN || key == VK_RWIN) {
      modifiers = MOD_WIN;
      continue;
    }
    standardKey.push_back(key);
  }
  //separate registration for all standard (alpha/numeric) + modifiers set,
  //using the same hotKeyId for all registrations.
  for (uint key : standardKey) {
    RegisterHotKey(NULL, _id, modifiers, key);
  }
}

#else // Other platform then WINDOWS.

bool CInputListener::WasPressed(uint hotKeyId) {
  auto keyHolder = GetHotKey(hotKeyId);
  if (keyHolder) {
    return gits::AreAllKeysPressed(keyHolder->Keys());
  }
  return false;
}
void CInputListener::StartHotKeyListener(bool useMessageLoop) {}
CInputListener::~CInputListener() {}

void CInputListener::HotKeyHolder::RegisterKey() {}
#endif
} // namespace InputListener
