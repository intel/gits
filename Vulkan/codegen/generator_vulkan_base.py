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


# Vulkan API function level
PrototypeLevel = 0
GlobalLevel = 1
InstanceLevel = 2

enums_table = []
functions_table = []
structs_table = []


def Enum(**kwargs):
  enums_table.append(kwargs)

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
