#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import os
import string
import platform


def blocks(file, size=65536):
    while True:
        b = file.read(size)
        if not b:
            break
        yield b


def lines_count(file):
    with open(file, "r") as f:
        return sum(bl.count("\n") for bl in blocks(f))


# finds places in master file, where splits should occur
# these should be in places where 2 conditions are satisfied.
# a) line is inside a function at global scope (ie, it is encolsed
#    in a single pair of { }
# b) line is far enough in the file (depending on number of lines
#    in a file and number of output files.
# returned is a list of pairs where first element is line number
# where the split should be performed and second element
# is name of the function which this line belongs to
def scope_analyze(path, breakCount):
    breaks = []

    nestLevel = 0
    lineNo = 1

    linesTotal = lines_count(path)
    breakEvery = linesTotal // breakCount

    prev_line = ''
    current_function = ''
    prevLevel = 0

    needsBreak = False
    with open(path, "r") as ccodeFile:
        for line in ccodeFile:
            if lineNo % breakEvery == 0:
                needsBreak = True

            # OpenCL outputs buffers contents as comments and sometimes they can
            # contain '{' or '}'. That messes up the counters, so if the line
            # contains a comment, ignore any character after '//'.
            commentPos = line.find('//')
            if commentPos != -1:
                lineTrimmed = line[:commentPos]
            else:
                lineTrimmed = line
            opens = lineTrimmed.count('{')
            closes = lineTrimmed.count('}')
            balance = opens - closes

            if balance != 0:
                prevLevel = nestLevel
                nestLevel += balance

                if prevLevel == 0 and nestLevel == 1:
                    # strip 'void ' and '()\n'
                    current_function = prev_line[5:-3]

            if needsBreak and nestLevel == 2 and opens == 1 and closes == 0:
                # Break lines only before further nesting to avoid unexpected variable visibility problems
                breaks.append([lineNo - 1, current_function])
                needsBreak = False

            prev_line = line
            lineNo += 1
    return breaks


# given a list of break points construct a dict mapping function name
# to indices of breakpoints this function contains
def calc_continuations(break_points):
    i = 0
    conts = {}
    for bk in break_points:
        if bk[1] not in conts:
            conts[bk[1]] = []

        conts[bk[1]].append(i)
        i += 1

    return conts


def divide_file(cppFilePath, break_points):
    def out_name(org, i):
        return '{:s}{:04d}.cpp'.format(org[:-4], i)

    continuations = calc_continuations(break_points)
    # print(continuations)
    current_line = 1
    current_file = 1
    output_file = open(out_name(cppFilePath, current_file), 'w')
    prev_func = ''

    break_points.append([-1, break_points[-1][1]])
    with open(cppFilePath, "r") as ccodeFile:
        for line in ccodeFile:
            output_file.write(line)
            # We are at a breakpoint; this means we are in a function mentioned
            # in the breakpoint. If this is the first breakpoint in the
            # function, we add the code of all continuations here as well.
            if current_line == break_points[0][0]:
                current_file += 1
                bk = break_points.pop(0)

                if bk[1] != prev_func:
                    for cnt in continuations[bk[1]]:
                        output_file.write("  void {:s}_cont_{:04d}();\n".format(bk[1], cnt+2))
                        output_file.write("  {:s}_cont_{:04d}();\n".format(bk[1], cnt+2))

                prev_func = bk[1]

                output_file.write('}\n')
                output_file.close()
                output_file = open(out_name(cppFilePath, current_file), 'w')
                output_file.write('#include "gitsApi.h"\n')
                output_file.write('#include "stream_externs.h"\n')
                output_file.write('#include "helperVk.h"\n')
                output_file.write('#include "helperCL.h"\n')
                output_file.write("#ifdef WITH_LEVELZERO\n")
                output_file.write('#include "helperL0.h"\n')
                output_file.write("#endif\n")
                output_file.write('#include "helperGL.h"\n')
                output_file.write("void {:s}_cont_{:04d}()\n".format(bk[1], current_file))
                output_file.write('{\n')
            current_line += 1

    output_file.close()
    return current_file


def getFileMainPart(filepath):
    lastSlashPos = 0
    if filepath.rfind("\\") > -1:
        lastSlashPos = filepath.rfind("\\")
        return filepath[lastSlashPos+1:-11]
    elif filepath.rfind("/") > -1:
        lastSlashPos = filepath.rfind("/")
        return filepath[lastSlashPos+1:-11]
    else:
        return filepath[lastSlashPos:-11]


# ################################## MAIN ################################ #

args = sys.argv
if platform.system() == "Windows":
    usage = """
  Usage:
  CCodeDivider.py <file for division path> <divisor>
      <file for division path>: path to divided .cpp file
      <divisor>:  number of output files

  """
else:
    usage = """
  Usage:
  CCodeDivider.py <file for division path> <divisor> <optional CMakeLists.txt>
      <file for division path>: path to divided .cpp file
      <divisor>:  number of output files
      <optional CMakeLists.txt>: CCode CMakeLists.txt file

  """


# Get input parameters
if not 2 < len(args) < 5:
    print(usage)
    print("Incorrect number of arguments: " + str(len(args)))
    sys.exit(1)

cppFilePath = args[1]
fileDivisor = int(args[2])
projectPath = ""

if len(args) == 4:
    projectPath = args[3]
    if not os.path.exists(projectPath):
        print(usage)
        print('Argument 3 incorrect - no such file')
        sys.exit(1)

if not os.path.exists(cppFilePath):
    print(usage)
    print('Argument 1 incorrect - no such file')
    sys.exit(1)

if fileDivisor < 2:
    print('<divisor> has to be at least 2')
    sys.exit(1)

if sys.version_info < (3, 6):
    print("ERROR: This script is compatible and tested with Python 3.6."
          "It is not compatible with older versions of Python."
          "Please upgrade your Python interpreter.")
    sys.exit(1)

if sys.version_info >= (3, 7):
    print("WARNING: This script is compatible and tested with Python 3.6."
          "It looks like you are using a newer version of Python.")

# Divide CCode
break_points = scope_analyze(cppFilePath, fileDivisor - 1)
fileDivisor = min(fileDivisor, len(break_points) + 1)
divide_file(cppFilePath, break_points)


# Update CMakeLists.txt file (unix)
if platform.system() != "Windows" and os.path.exists(projectPath):
    projectFile = open(projectPath, 'r')
    lines = projectFile.readlines()
    lineNr = 0
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.find("_frames") != -1 and line.find(".cpp") != -1:
            lineNr = i
            lines.pop(lineNr)
        else:
            i = i + 1

    subFileNr = 1
    if lineNr != 0:
        while subFileNr <= fileDivisor:
            newline = "    CCodeProject/" + getFileMainPart(cppFilePath) + '_frames{:04d}.cpp\n'.format(subFileNr)
            lines.insert(lineNr, newline)
            subFileNr = subFileNr+1
        projectFile.close()
        projectFile = open(projectPath, 'w')
        projectFile.writelines(lines)
    else:
        print("Can't find *_frames.cpp in CMakeLists.txt - no update made")