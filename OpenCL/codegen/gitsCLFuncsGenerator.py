#!python
#!/usr/bin/env python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import cl_constants
import cl_generator
import cl_utils
import generator_cl

import shutil
import os
import sys

if len(sys.argv) < 2:
    print('Usage: {} outpath'.format(sys.argv[0]))
    sys.exit(1)
PATHOUT = sys.argv[1]

functions = generator_cl.GetFunctions()
cl_utils.process(functions)
blacklist = (
    'nanf',
)
for function in blacklist:
    print(' '.join(['Blacklisted:', function]))
    del functions[function]

enums = generator_cl.GetEnums()
cl_utils.process_enums(enums)

arguments = generator_cl.GetArguments()


print('Generating {file}...'.format(file=cl_constants.SWITCH_NAME))
cl_generator.generate_switch(functions)
print('Generating {file}...'.format(file=cl_constants.IDS_NAME))
cl_generator.generate_ids(functions)
print('Generating {file} and {file2}...'.format(file=cl_constants.TOKENS_HEADER_NAME, file2=cl_constants.TOKENS_SOURCE_NAME))
cl_generator.generate_tokens(functions)
print('Generating {file} and {file2}...'.format(file=cl_constants.WRAPPER_HEADER_NAME, file2=cl_constants.WRAPPER_INTERFACE_HEADER_NAME))
cl_generator.generate_wrapper_headers(functions)
print('Generating {file}...'.format(file=cl_constants.WRAPPER_SOURCE_NAME))
cl_generator.generate_wrapper_source(functions)
print('Generating {file}...'.format(file=cl_constants.DRIVERS_NAME))
cl_generator.generate_drivers(functions, enums)
print('Generating {file}...'.format(file=cl_constants.PREPOST_NAME))
cl_generator.generate_prepost(functions)
print('Generating {file}...'.format(file=cl_constants.PLAYER_RUNWRAP_NAME))
cl_generator.generate_runwrap(functions)
print('Generating {file}...'.format(file=cl_constants.PLAYER_STATETRACK_NAME))
cl_generator.generate_state_tracking(functions)
print('Generating {file}...'.format(file=cl_constants.RECORDER_SUBWRAPPERS_NAME))
cl_generator.generate_recorder_subwrappers(functions)
print('Generating {file}...'.format(file=cl_constants.RECEXECWRAP_NAME))
cl_generator.generate_recexecwrap(functions)
print('Generating {file}...'.format(file=cl_constants.DEF_NAME))
cl_generator.generate_def(functions)
print('Generating {file} and {file2}...'.format(file=cl_constants.ARGUMENTS_HEADER_NAME, file2=cl_constants.ARGUMENTS_SOURCE_NAME))
cl_generator.generate_arguments(enums, arguments)
print('Generating {file}...'.format(file=cl_constants.HEADER_NAME))
cl_generator.generate_header(enums)
print('Generating {file}...'.format(file=cl_constants.LUA_CONSTANTS_NAME))
cl_generator.generate_lua_constants(enums)

def move_file(filename):
    path = os.path.join(PATHOUT, filename)
    print('Moving {} to {}...'.format(filename, path))
    shutil.move(filename, path)

def move_lua_file(filename):
    path = os.path.join(PATHOUT, filename)
    print('Moving {} to {}...'.format(filename, path))
    shutil.move(filename, path)

def copy_file(filename):
    path = os.path.join('../common/include', filename)
    print('Copying {} to {}...'.format(filename, path))
    shutil.copy2(filename, path)

copy_file(cl_constants.IDS_NAME)
move_file(cl_constants.SWITCH_NAME)
move_file(cl_constants.DRIVERS_NAME)
move_file(cl_constants.HEADER_NAME)
move_file(cl_constants.TOKENS_HEADER_NAME)
move_file(cl_constants.TOKENS_SOURCE_NAME)
move_file(cl_constants.ARGUMENTS_HEADER_NAME)
move_file(cl_constants.ARGUMENTS_SOURCE_NAME)
move_file(cl_constants.WRAPPER_HEADER_NAME)
move_file(cl_constants.WRAPPER_INTERFACE_HEADER_NAME)
move_file(cl_constants.WRAPPER_SOURCE_NAME)
move_file(cl_constants.PREPOST_NAME)
move_file(cl_constants.DEF_NAME)
move_lua_file(cl_constants.LUA_CONSTANTS_NAME)

print('Done.')
