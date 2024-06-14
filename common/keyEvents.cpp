// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   keyEvents.cpp
 *
 */

#include "keyEvents.h"
#include "platform.h"
#include "gits.h"

#if defined GITS_PLATFORM_WINDOWS

#include <windows.h>

#define VK_EQUALS 0xBB
#define VK_MINUS  0xBD

#define VK_LSHIFT   0xA0
#define VK_RSHIFT   0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU    0xA4
#define VK_RMENU    0xA5

#define VK_BACK_QUOTE 0xC0
#define VK_BACK_SLASH 0xDC

#define VK_CLOSE_BRACKET 0xDD
#define VK_COMMA         0xBC
#define VK_OPEN_BRACKET  0xDB
#define VK_PERIOD        0xBE

#define VK_QUOTE     0xDE
#define VK_SEMICOLON 0xBA
#define VK_SLASH     0xBF

#define VK_SCROLL_LOCK 0x91
#define VK_NUM_LOCK    0x90

#endif

void gits::InitKeyMap(std::map<std::string, unsigned>& keyMap) {

#if defined GITS_PLATFORM_WINDOWS
  keyMap["backspace"] = VK_BACK;
  keyMap["tab"] = VK_TAB;
  keyMap["clear"] = VK_CLEAR;
  keyMap["enter"] = VK_RETURN;
  keyMap["shift"] = VK_SHIFT;
  keyMap["ctrl"] = VK_CONTROL;
  keyMap["alt"] = VK_MENU;
  keyMap["lshift"] = VK_LSHIFT;
  keyMap["lctrl"] = VK_LCONTROL;
  keyMap["lalt"] = VK_LMENU;

  keyMap["rshift"] = VK_RSHIFT;
  keyMap["rctrl"] = VK_RCONTROL;
  keyMap["ralt"] = VK_RMENU;

  keyMap["pause"] = VK_PAUSE;
  keyMap["caps"] = VK_CAPITAL;
  keyMap["capslock"] = VK_CAPITAL;
  keyMap["esc"] = VK_ESCAPE;
  keyMap["space"] = VK_SPACE;
  keyMap["pageup"] = VK_PRIOR;
  keyMap["pagedown"] = VK_NEXT;
  keyMap["end"] = VK_END;
  keyMap["home"] = VK_HOME;
  keyMap["left"] = VK_LEFT;
  keyMap["up"] = VK_UP;
  keyMap["right"] = VK_RIGHT;
  keyMap["down"] = VK_DOWN;
  keyMap["select"] = VK_SELECT;
  keyMap["execute"] = VK_EXECUTE;
  keyMap["print"] = VK_SNAPSHOT;
  keyMap["printscreen"] = VK_SNAPSHOT;
  keyMap["insert"] = VK_INSERT;
  keyMap["delete"] = VK_DELETE;
  keyMap["help"] = VK_HELP;
  keyMap["0"] = '0';
  keyMap["1"] = '1';
  keyMap["2"] = '2';
  keyMap["3"] = '3';
  keyMap["4"] = '4';
  keyMap["5"] = '5';
  keyMap["6"] = '6';
  keyMap["7"] = '7';
  keyMap["8"] = '8';
  keyMap["*"] = '8'; //use the "8" key for * as well
  keyMap["9"] = '9';
  keyMap["a"] = 'A';
  keyMap["b"] = 'B';
  keyMap["c"] = 'C';
  keyMap["d"] = 'D';
  keyMap["e"] = 'E';
  keyMap["f"] = 'F';
  keyMap["g"] = 'G';
  keyMap["h"] = 'H';
  keyMap["i"] = 'I';
  keyMap["j"] = 'J';
  keyMap["k"] = 'K';
  keyMap["l"] = 'L';
  keyMap["m"] = 'M';
  keyMap["n"] = 'N';
  keyMap["o"] = 'O';
  keyMap["p"] = 'P';
  keyMap["q"] = 'Q';
  keyMap["r"] = 'R';
  keyMap["s"] = 'S';
  keyMap["t"] = 'T';
  keyMap["u"] = 'U';
  keyMap["v"] = 'V';
  keyMap["w"] = 'W';
  keyMap["x"] = 'X';
  keyMap["y"] = 'Y';
  keyMap["z"] = 'Z';
  keyMap["lwin"] = VK_LWIN;
  keyMap["rwin"] = VK_RWIN;
  keyMap["apps"] = VK_APPS;
  keyMap["num0"] = VK_NUMPAD0;
  keyMap["num1"] = VK_NUMPAD1;
  keyMap["num2"] = VK_NUMPAD2;
  keyMap["num3"] = VK_NUMPAD3;
  keyMap["num4"] = VK_NUMPAD4;
  keyMap["num5"] = VK_NUMPAD5;
  keyMap["num6"] = VK_NUMPAD6;
  keyMap["num7"] = VK_NUMPAD7;
  keyMap["num8"] = VK_NUMPAD8;
  keyMap["num9"] = VK_NUMPAD9;
  keyMap["num*"] = VK_MULTIPLY;
  keyMap["num+"] = VK_ADD;
  keyMap["num-"] = VK_SUBTRACT;
  keyMap["num."] = VK_DECIMAL;
  keyMap["num/"] = VK_DIVIDE;
  keyMap["f1"] = VK_F1;
  keyMap["f2"] = VK_F2;
  keyMap["f3"] = VK_F3;
  keyMap["f4"] = VK_F4;
  keyMap["f5"] = VK_F5;
  keyMap["f6"] = VK_F6;
  keyMap["f7"] = VK_F7;
  keyMap["f8"] = VK_F8;
  keyMap["f9"] = VK_F9;
  keyMap["f10"] = VK_F10;
  keyMap["f11"] = VK_F11;
  keyMap["f12"] = VK_F12;
  keyMap["f13"] = VK_F13;
  keyMap["f14"] = VK_F14;
  keyMap["f15"] = VK_F15;
  keyMap["f16"] = VK_F16;
  keyMap["f17"] = VK_F17;
  keyMap["f18"] = VK_F18;
  keyMap["f19"] = VK_F19;
  keyMap["f20"] = VK_F20;
  keyMap["f21"] = VK_F21;
  keyMap["f22"] = VK_F22;
  keyMap["f23"] = VK_F23;
  keyMap["f24"] = VK_F24;
  keyMap["numlock"] = VK_NUMLOCK;
  keyMap["scrolllock"] = VK_SCROLL;

  keyMap["~"] = VK_BACK_QUOTE;
  keyMap["\\"] = VK_BACK_SLASH;
  keyMap["|"] = VK_BACK_SLASH;
  keyMap["]"] = VK_CLOSE_BRACKET;
  keyMap["["] = VK_OPEN_BRACKET;

  keyMap[""] = VK_CLOSE_BRACKET;
  keyMap[""] = VK_OPEN_BRACKET;

  keyMap[" "] = VK_COMMA;
  keyMap["<"] = VK_COMMA;
  keyMap["."] = VK_PERIOD;
  keyMap[">"] = VK_PERIOD;
  keyMap["="] = VK_EQUALS;
  keyMap["+"] = VK_EQUALS;
  keyMap["_"] = VK_MINUS;
  keyMap["-"] = VK_MINUS;

  keyMap["numlock"] = VK_NUM_LOCK;

  keyMap["\""] = VK_QUOTE;
  keyMap["'"] = VK_QUOTE;

  keyMap["scrolllock"] = VK_SCROLL_LOCK;
  keyMap[":"] = VK_SEMICOLON;
  keyMap[";"] = VK_SEMICOLON;

  keyMap["/"] = VK_SLASH;
  keyMap["?"] = VK_SLASH;

#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

bool gits::IsKeyPressed(unsigned key) {
#if defined GITS_PLATFORM_WINDOWS
  return (GetAsyncKeyState(key) & 0x8000) != 0;
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}
bool gits::AreAllKeysPressed(std::vector<unsigned> keys) {
  for (auto key : keys) {
    if (!IsKeyPressed(key)) {
      return false;
    }
  }
  if (keys.size() > 0) {
    return true;
  }
  return false;
}

uint32_t gits::GetKeyVal(const std::string& str) {
  static std::map<std::string, unsigned> keyMap;
  if (keyMap.empty()) {
    InitKeyMap(keyMap);
  }
  if (keyMap.find(str) == keyMap.end()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  } else {
    return keyMap[str];
  }
}
