#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import os

def replace_dml_declare_interface(in_lines):
    out_lines = []
    for line in in_lines:
        if line.startswith('interface DML_DECLARE_INTERFACE'):
            pattern = r'interface DML_DECLARE_INTERFACE\(.*\)'
            replacement = r'class'
            line = re.sub(pattern, replacement, line)
        elif line.startswith('    IFACEMETHOD_'):
            pattern = r'IFACEMETHOD_\((\w*), (\w*)\)'
            replacement = r'\1 __stdcall \2'
            line = re.sub(pattern, replacement, line)
        elif line.startswith('    IFACEMETHOD'):
            pattern = r'IFACEMETHOD\((\w*)\)'
            replacement = r'HRESULT __stdcall \1'
            line = re.sub(pattern, replacement, line)

        out_lines.append(line)
    
    return out_lines

def preprocess_xess_files(in_lines):
    out_lines = []
    for line in in_lines:
        if line.startswith('XESS_PACK_'):
            pattern = r'(XESS_PACK_(?:B|E)\(\))'
            replacement = r'//\1'
            line = re.sub(pattern, replacement, line)
        elif line.startswith('XESS_API'):
            pattern = r'XESS_API '
            replacement = r''
            line = re.sub(pattern, replacement, line)
        elif 'sizecheck__LINE__' in line:
            pattern = r'(.*sizecheck__LINE__.*)'
            replacement = r'//\1'
            line = re.sub(pattern, replacement, line)
        out_lines.append(line)
    return out_lines

def replace_dstorage_declare_interface(header_src):
    pattern = r'DECLARE_INTERFACE_IID_\(\s*(\w+),\s*(\w+),\s*.*\)'
    replacement = r'\n\nclass \1: public \2'
    return re.sub(pattern, replacement, header_src)

def remove_c_style_interface(in_lines):
    out_lines = []
    inside_c_style_interface = False
    for line in in_lines:
        if line.startswith('#if defined(__cplusplus)'):
            pass
        elif line.startswith('#else 	/* C style interface */'):
            inside_c_style_interface = True
        elif line.startswith('#endif 	/* C style interface */'):
            inside_c_style_interface = False
        elif inside_c_style_interface == True:
            pass
        else:
            out_lines.append(line)
            
    return out_lines
    

def remove_not_msc_ver(in_lines):
    out_lines = []
    inside_if = False
    inside_else = False
    for line in in_lines:
        if line.startswith('#if defined(_MSC_VER) || !defined(_WIN32)'):
            inside_if = True
        elif inside_if and line.startswith('#else'):
            inside_if = False
            inside_else = True
        elif inside_else and line.startswith('#endif'):
            inside_else = False
        elif inside_else:
            pass            
        else:
            out_lines.append(line)

    return out_lines
    

def encode_sal(in_lines):

    sals = [
        '_Field_z_ _Maybenull_',
        '__in_ecount', '__in_ecount_opt', '__RPC__deref_out',
        '__RPC__deref_out_opt', '__RPC__in', '_Always_', '_COM_Outptr_',
        '_COM_Outptr_opt_', '_COM_Outptr_opt_result_maybenull_',
        '_Field_size_bytes_full_', '_Field_size_bytes_full_opt_',
        '_Field_size_full_', '_Field_size_full_opt_', '_Field_z_', '_In_',
        '_In_opt_', '_In_range_', '_In_reads_', '_In_reads_bytes_',
        '_In_reads_bytes_opt_', '_In_reads_opt_', '_In_z_', '_Inout_',
        '_Inout_opt_', '_Inout_updates_bytes_', '_Out_', '_Out_opt_',
        '_Out_writes_', '_Out_writes_all_', '_Out_writes_all_opt_',
        '_Out_writes_bytes_', '_Out_writes_bytes_opt_', '_Out_writes_bytes_to_',
        '_Out_writes_opt_', '_Out_writes_to_opt_',
        '_Outptr_opt_result_bytebuffer_', '_Field_size_', '_In_opt_count_',
        '_In_count_', '_Field_size_opt_', '_Maybenull_', '_Field_size_bytes_',
        '_In_opt_z_',
        '_Out_writes_to_'
    ]

    out_lines = []
    for line in in_lines:
        for sal_name in sals:
            reg = sal_name + '( |\(.*?\)+)( *?const | +)?'
            match = re.search(reg, line)
            if (match):
                sal_orig = match[0]

                sal = sal_orig
                const = False
                reg = 'const +$'
                if re.search(reg, sal):
                    sal = re.sub(reg, '', sal)
                    const = True

                sal = sal.strip()

                escapes = {
                    '(' : '___1___',
                    ')' : '___2___',
                    '*' : '___3___',
                    ' ' : '___4___',
                    '"' : '___5___',
                    '-' : '___6___',
                    ',' : '___7___'
                }

                for c in escapes:
                    sal = sal.replace(c, escapes[c])
                sal = '_' + sal + '_'

                if const:
                    sal = 'const ' + sal

                line = line.replace(sal_orig, sal)

        out_lines.append(line)
        
    return out_lines
    

def comment_out_defines(in_lines):

    out_lines = []
    is_define = False
    for line in in_lines:
        if line.startswith('#define'):
            is_define = True
        if is_define:
            line = '//' + line
        if is_define and re.search('\\\\$', line):
            pass
        else:
            is_define = False
        out_lines.append(line)
        
    return out_lines
    

def replace(in_lines):

    replacements = {
        'MIDL_INTERFACE' : 'class //MIDL_INTERFACE',
        'STDMETHODCALLTYPE' : '__stdcall',
        'STDAPI' : 'HRESULT __stdcall',
        'WINAPI' : '__stdcall',
        'DEFINE_ENUM_FLAG_OPERATORS' : '//DEFINE_ENUM_FLAG_OPERATORS',
        '#ifndef ' : '//#ifndef ', 
        '#endif' : '//#endif',
        '#if ' : '//#if ',
        '#ifdef ' : '//#ifdef ',
        '#error' : '//#error',
        '#elif' : '//#elif', 
        '#else' : '//#else',
        'typedef interface' : '//typedef interface', 
        'EXTERN_C' : '//EXTERN_C',
        'DEFINE_GUID' : '//DEFINE_GUID',
        '#undef' : '//#undef',
        '#define' : '//#define',
        '#define' : '//#define',
        'ID3D10Blob' : 'ID3DBlob',
        'LPCVOID' : 'const void *',
        'LPVOID' : 'void *'
    }
    
    out_lines = []
    for line in in_lines:
        for key in replacements:
            if (line.find(key) >=0):
                line = line.replace(key, replacements[key])
        out_lines.append(line)
        
    return out_lines

def remove_declare_interface(in_lines):
    out_lines = []
    inside = False
    for line in in_lines:
        if line.startswith('DECLARE_INTERFACE'):
            inside = True
        elif inside and line.startswith('};'):
            inside = False
        elif inside:
            pass            
        else:
            out_lines.append(line)

    return out_lines
    

def preprocess_header(header_file_path):
    lines = []
    with open(header_file_path, 'r') as fin:
        header_src = fin.read()
        header_src = replace_dstorage_declare_interface(header_src)

        lines = header_src.splitlines(keepends=True)
        lines = remove_c_style_interface(lines)
        lines = remove_not_msc_ver(lines)
        lines = comment_out_defines(lines)
        #lines = remove_declare_interface(lines)
        lines = replace(lines)        
        lines = encode_sal(lines)
        lines = replace_dml_declare_interface(lines)
        lines = preprocess_xess_files(lines)
    
    str = ''
    for line in lines:
        str += line

    return str
