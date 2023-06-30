#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Methods types
Param=1
QueueSubmit=2
CreateImage=4
CreateBuffer=8
CmdBufferSet=16
CmdBufferBind=32
CmdBufferPush=64
BeginRenderPass=128
EndRenderPass=256


# Vulkan API function level
PrototypeLevel = 0
GlobalLevel = 1
InstanceLevel = 2

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
