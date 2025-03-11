#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import os
from dataclasses import dataclass, field
import cxxheaderparser.simple
import cxxheaderparser.types

from find_windows_sdk import get_windows_sdk
from intermediates_creator import parse_structures, parse_functions, parse_interfaces, parse_enums, postprocess
from generator_dml import generate_dml_files
from generator_xess import generate_xess_dispatch_table
from generator_recorder import generate_recorder_files
from generator_coders import generate_coders_files
from generator_layer import generate_layer_files
from generator_trace import generate_trace_files
from generator_api_debug import generate_api_debug_files
from generator_to_string import generate_to_string_files
from generator_player import generate_player_files
from generator_subcapture import generate_subcapture_files
from generator_resource_dumping import generate_resource_dumping_files
from generator_skip_calls import generate_skip_calls_files
from intermediates import Api
from header_preprocessor import preprocess_header
from command_ids import build_command_ids
from plugin_generator import generate_plugin_artifacts

@dataclass
class CppHeader:
    path: str
    api: Api
    data: cxxheaderparser.simple.ParsedData = field(default=None)

    def __post_init__(self):
        preprocessed = preprocess_header(self.path)
        self.data = cxxheaderparser.simple.parse_string(preprocessed)

def main():
    if len(sys.argv) < 5:
        print('Usage: {} <inpath_d3d12> <inpath_dml> <inpath_xess> <inpath_dstorage> <outpath>'.format(sys.argv[0]))
        sys.exit(1)
        
    command_line = ' '.join(sys.argv)
    print('Command line:', command_line)

    inpath_d3d12 = sys.argv[1] + '/'
    inpath_dml = sys.argv[2] + '/'
    inpath_xess = sys.argv[3] + '/'
    inpath_dstorage = sys.argv[4] + '/'
    outpath = sys.argv[5] + '/'

    inpath_dxgi = os.path.join(get_windows_sdk(), "shared/")
    print('Using DXGI headers from: ' + inpath_dxgi)

    headers = [
        CppHeader(path=inpath_d3d12 + 'd3d12.h', api=Api.D3D12),
        CppHeader(path=inpath_d3d12 + 'd3d12sdklayers.h', api=Api.D3D12_DEBUG),
        CppHeader(path=inpath_d3d12 + 'd3dcommon.h', api=Api.COMMON),
        CppHeader(path=inpath_dxgi + 'dxgi.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgi1_2.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgi1_3.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgi1_4.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgi1_5.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgi1_6.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgicommon.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgiformat.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + 'dxgitype.h', api=Api.DXGI),
        CppHeader(path=inpath_dxgi + '../um/dxgidebug.h', api=Api.DXGI_DEBUG),
        CppHeader(path=inpath_dml + 'DirectML.h', api=Api.DML),
        CppHeader(path=inpath_xess + 'xess.h', api=Api.XESS),
        CppHeader(path=inpath_xess + 'xess_d3d12.h', api=Api.XESS),
        CppHeader(path=inpath_dstorage + 'dstorage.h', api=Api.DSTORAGE)
    ]

    enums = []
    for header in headers:
        enums += parse_enums(header)

    structures = []
    for header in headers:
        structures += parse_structures(header)

    functions = []
    for header in headers:
        functions += parse_functions(header)

    interfaces = []
    for header in headers:
        interfaces += parse_interfaces(header)

    postprocess(functions, interfaces, structures)

    context = {
        'enums': enums,
        'structures': structures,
        'functions': functions,
        'interfaces': interfaces
    }
    # Pre-load the Command IDs from command_ids.json
    # Set "update_command_ids_file = True" to add new Command IDs (i.e. when updating headers)
    context['command_ids'] = build_command_ids(context, os.path.join(outpath, 'codegen'),
                                               update_command_ids_file = True)
    generate_player_files(context, os.path.join(outpath, 'player'))
    generate_recorder_files(context, os.path.join(outpath, 'recorder'))
    generate_coders_files(context, os.path.join(outpath, 'common/coders'))
    generate_dml_files(context, os.path.join(outpath, 'common/coders'))
    generate_xess_dispatch_table(context, [os.path.join(outpath, 'player'), os.path.join(outpath, 'recorder')])
    generate_layer_files(context, os.path.join(outpath, 'common/layer_interface'))
    generate_trace_files(context, os.path.join(outpath, 'layers/trace'))
    generate_api_debug_files(context, os.path.join(outpath, 'layers/api_debug'))
    generate_to_string_files(context, os.path.join(outpath, 'common/utils/to_string'))
    generate_subcapture_files(context, os.path.join(outpath, 'layers/subcapture'))
    generate_resource_dumping_files(context, os.path.join(outpath, 'layers/resource_dumping'))
    generate_skip_calls_files(context, os.path.join(outpath, 'layers/skip_calls'))

    plugin_directories = [
        os.path.join(outpath, '../plugins/DirectX'),
        os.path.join(outpath, '../plugins/internal/DirectX')
    ]
    generate_plugin_artifacts(context, plugin_directories)

    sys.exit(0)

if __name__ == '__main__':
    main()
