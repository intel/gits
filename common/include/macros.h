// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// HELPER MACROS
#define EXPAND(x)   x
#define ESC(...)    __VA_ARGS__
#define GET_CSTR(x) #x

// EVALUATES NUMBER OF PASSED ARGUMETNS. RETURNS RESULT PRECEDED WITH PREFIX.
// WORKS ONLY FOR 1 UP TO 30 ARGUMENTS
#define GET_ARGS_NUM(...)                                                                          \
  EXPAND(GET_ARGS_NUM_IMPL(__VA_ARGS__, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,    \
                           16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#define GET_ARGS_NUM_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,   \
                          _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, N, \
                          ...)                                                                     \
  N

// FOR_EACH(FUNC, BASEARG, ARG1,.....ARGN) - FOR X=1..N CALL FUNC(BASEARG, ARGX)
#define FOR_EACH(FUNC, BASEARG, ...)                                                               \
  PREP_FOR_EACH(FOR_EACH, GET_ARGS_NUM(__VA_ARGS__), EXPAND((FUNC, BASEARG, __VA_ARGS__)))
#define PREP_FOR_EACH(f, d, a)      PREP_FOR_EACH_IMPL(f, d, a)
#define PREP_FOR_EACH_IMPL(f, d, a) f##d a

#define FOR_EACH1(FUNC, BASEARG, ARG1)       FUNC(BASEARG, ARG1)
#define FOR_EACH2(FUNC, BASEARG, ARG1, ARG2) FUNC(BASEARG, ARG1) FUNC(BASEARG, ARG2)
#define FOR_EACH3(FUNC, BASEARG, ARG1, ARG2, ARG3)                                                 \
  FUNC(BASEARG, ARG1) FUNC(BASEARG, ARG2) FUNC(BASEARG, ARG3)
#define FOR_EACH4(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4)                                           \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2) FUNC(BASEARG, ARG3) FUNC(BASEARG, ARG4)
#define FOR_EACH5(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5)                                     \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3) FUNC(BASEARG, ARG4) FUNC(BASEARG, ARG5)
#define FOR_EACH6(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)                               \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4) FUNC(BASEARG, ARG5) FUNC(BASEARG, ARG6)
#define FOR_EACH7(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)                         \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5) FUNC(BASEARG, ARG6) FUNC(BASEARG, ARG7)
#define FOR_EACH8(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)                   \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6) FUNC(BASEARG, ARG7) FUNC(BASEARG, ARG8)
#define FOR_EACH9(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)             \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7) FUNC(BASEARG, ARG8) FUNC(BASEARG, ARG9)
#define FOR_EACH10(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10)     \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8) FUNC(BASEARG, ARG9) FUNC(BASEARG, ARG10)
#define FOR_EACH11(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11)                                                                          \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9) FUNC(BASEARG, ARG10) FUNC(BASEARG, ARG11)
#define FOR_EACH12(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12)                                                                   \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10) FUNC(BASEARG, ARG11) FUNC(BASEARG, ARG12)
#define FOR_EACH13(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13)                                                            \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11) FUNC(BASEARG, ARG12) FUNC(BASEARG, ARG13)
#define FOR_EACH14(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14)                                                     \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12) FUNC(BASEARG, ARG13) FUNC(BASEARG, ARG14)
#define FOR_EACH15(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15)                                              \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13) FUNC(BASEARG, ARG14) FUNC(BASEARG, ARG15)
#define FOR_EACH16(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16)                                       \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14) FUNC(BASEARG, ARG15) FUNC(BASEARG, ARG16)
#define FOR_EACH17(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17)                                \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15) FUNC(BASEARG, ARG16) FUNC(BASEARG, ARG17)
#define FOR_EACH18(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18)                         \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16) FUNC(BASEARG, ARG17) FUNC(BASEARG, ARG18)
#define FOR_EACH19(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19)                  \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17) FUNC(BASEARG, ARG18) FUNC(BASEARG, ARG19)
#define FOR_EACH20(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20)           \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18) FUNC(BASEARG, ARG19) FUNC(BASEARG, ARG20)
#define FOR_EACH21(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21)    \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19) FUNC(BASEARG, ARG20) FUNC(BASEARG, ARG21)
#define FOR_EACH22(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22)                                                                          \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20) FUNC(BASEARG, ARG21) FUNC(BASEARG, ARG22)
#define FOR_EACH23(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23)                                                                   \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21) FUNC(BASEARG, ARG22) FUNC(BASEARG, ARG23)
#define FOR_EACH24(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24)                                                            \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22) FUNC(BASEARG, ARG23) FUNC(BASEARG, ARG24)
#define FOR_EACH25(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25)                                                     \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23) FUNC(BASEARG, ARG24) FUNC(BASEARG, ARG25)
#define FOR_EACH26(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25, ARG26)                                              \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23)                                                                             \
  FUNC(BASEARG, ARG24) FUNC(BASEARG, ARG25) FUNC(BASEARG, ARG26)
#define FOR_EACH27(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25, ARG26, ARG27)                                       \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23)                                                                             \
  FUNC(BASEARG, ARG24)                                                                             \
  FUNC(BASEARG, ARG25) FUNC(BASEARG, ARG26) FUNC(BASEARG, ARG27)
#define FOR_EACH28(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28)                                \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23)                                                                             \
  FUNC(BASEARG, ARG24)                                                                             \
  FUNC(BASEARG, ARG25)                                                                             \
  FUNC(BASEARG, ARG26) FUNC(BASEARG, ARG27) FUNC(BASEARG, ARG28)
#define FOR_EACH29(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29)                         \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23)                                                                             \
  FUNC(BASEARG, ARG24)                                                                             \
  FUNC(BASEARG, ARG25)                                                                             \
  FUNC(BASEARG, ARG26)                                                                             \
  FUNC(BASEARG, ARG27) FUNC(BASEARG, ARG28) FUNC(BASEARG, ARG29)
#define FOR_EACH30(FUNC, BASEARG, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10,     \
                   ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21,    \
                   ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29, ARG30)                  \
  FUNC(BASEARG, ARG1)                                                                              \
  FUNC(BASEARG, ARG2)                                                                              \
  FUNC(BASEARG, ARG3)                                                                              \
  FUNC(BASEARG, ARG4)                                                                              \
  FUNC(BASEARG, ARG5)                                                                              \
  FUNC(BASEARG, ARG6)                                                                              \
  FUNC(BASEARG, ARG7)                                                                              \
  FUNC(BASEARG, ARG8)                                                                              \
  FUNC(BASEARG, ARG9)                                                                              \
  FUNC(BASEARG, ARG10)                                                                             \
  FUNC(BASEARG, ARG11)                                                                             \
  FUNC(BASEARG, ARG12)                                                                             \
  FUNC(BASEARG, ARG13)                                                                             \
  FUNC(BASEARG, ARG14)                                                                             \
  FUNC(BASEARG, ARG15)                                                                             \
  FUNC(BASEARG, ARG16)                                                                             \
  FUNC(BASEARG, ARG17)                                                                             \
  FUNC(BASEARG, ARG18)                                                                             \
  FUNC(BASEARG, ARG19)                                                                             \
  FUNC(BASEARG, ARG20)                                                                             \
  FUNC(BASEARG, ARG21)                                                                             \
  FUNC(BASEARG, ARG22)                                                                             \
  FUNC(BASEARG, ARG23)                                                                             \
  FUNC(BASEARG, ARG24)                                                                             \
  FUNC(BASEARG, ARG25)                                                                             \
  FUNC(BASEARG, ARG26)                                                                             \
  FUNC(BASEARG, ARG27)                                                                             \
  FUNC(BASEARG, ARG28) FUNC(BASEARG, ARG29) FUNC(BASEARG, ARG30)