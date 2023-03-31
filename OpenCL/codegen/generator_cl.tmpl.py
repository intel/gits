#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

Creator=1
Retain=2
Release=4
Info=8
Build=16,
Set=32,
Enqueue=64,
NDRange=128

functions_table = []

def Function(**kwargs):
  functions_table.append(kwargs)


def ArgDef(**kwargs):
  return kwargs

def RetDef(**kwargs):
  return kwargs

def GetFunctions():
  return functions_table

