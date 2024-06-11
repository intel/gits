#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import enum
from enum import IntFlag

# TODO: use `@verify(NAMED_FLAGS)` when Python 3.11 becomes available.
class FuncType(IntFlag):
    """Flag for API call classification."""

    NONE = 0
    PARAM = 1
    QUEUE_SUBMIT = 2
    CREATE_IMAGE = 4
    CREATE_BUFFER = 8
    COMMAND_BUFFER_SET = 16
    COMMAND_BUFFER_BIND = 32
    COMMAND_BUFFER_PUSH = 64
    BEGIN_RENDER_PASS = 128
    END_RENDER_PASS = 256
    DRAW = 512
    BLIT = 1024
    DISPATCH = 2048
    NEXT_SUBPASS = 4096


class FuncLevel(enum.Enum):  # Direct Enum import would clash with Vulkan Enums.
    """Vulkan API function level."""

    PROTOTYPE = 0
    GLOBAL = 1
    INSTANCE = 2


enums_table = []
functions_table = []
structs_table = []


def Enum(**kwargs):
  found = [i for i,x in enumerate(enums_table) if x['name']==kwargs['name']]
  if not found:
    enums_table.append(kwargs)
  else:
    it = found[0]
    enums_table[it]['enumerators'] = enums_table[it]['enumerators'][:-1] + kwargs['enumerators'] + [enums_table[it]['enumerators'][-1]]

def Function(**kwargs):
  functions_table.append(kwargs)

def Struct(**kwargs):
  structs_table.append(kwargs)

def ArgDef(**kwargs):
  return kwargs

def RetDef(**kwargs):
  return kwargs

def VarDef(**kwargs):
  return kwargs

def GetEnums():
  return enums_table

def GetFunctions():
  return functions_table

def GetStructs():
  return structs_table
