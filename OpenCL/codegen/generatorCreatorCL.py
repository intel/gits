#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================


from pycparser.ply.cpp import *
import pycparser.ply.lex as lex
import sys
import getopt
import re
import subprocess

sys.path.extend(['.', '..'])
from pycparser import c_parser, c_ast, parse_file
import pycparser

lexer = lex.lex()

extensions = [
    'INTEL', 'KHR', 'APPLE', 'EXT', 'AMD', 'NV'
    ]

headers = [
    './CL/cl.h',
    #'./CL/cl_d3d10.h',
    #'./CL/cl_d3d11.h',
    #'./CL/cl_ext.h',
    #'./CL/cl_gl.h',
    #'./CL/cl_gl_ext.h',
    #'./CL/cl_intel_dx9_media_sharing.h'
    #'./CL/cl_dx9_media_sharing.h'
    ]

functions = dict()
functions_name = list()

infile = 'generator_cl.py'
def read_file():
  infile_text = open(infile, 'r')
  try:
    for line in infile_text:
      table = re.split("=|\)|\(|,", line)
      for elem in table:
        if 'Function' in elem:
          idx = table.index(elem)
          if 'name' in table[idx+1]:
            functions_name.append(table[idx+2].strip("'"))
  finally:
    infile_text.close()

    #tmp = str(line)
    #table = tmp.strip().split("=").split("(").split(")")
    #print table

for header in headers:
    print header
    substituted = list()
    content = open(header, 'r').readlines()
    regex = re.compile(r'/\* ?(?!cl_)([a-z_]+)(\[[0-9]*\])? ?\*/')
    for line in content:
        line = regex.sub(r' \1', line)
        line = line.replace('*[]', '**')
        substituted.append(line)

    #Preprocess file and store in temporary file
    pp = Preprocessor(lexer)
    pp.add_path('utils\\fake_libc_include')

    macros = [ '__attribute__(x)' ]
    for macro in macros:
        pp.define(macro)

    pp.parse(''.join(substituted), header)

    pped = ""
    while True:
        t = pp.token()
        if not t: break
        if t.type != 'CPP_COMMENT':
          pped += t.value

    with open('tmp', 'w') as f:
        f.write(pped)

    # Function name is treated just like parameters.
    # Function name is considered return type value name.
    class ParamListVisitor(c_ast.NodeVisitor):
        def __init__(self):
            self.name = '???'
            self.arity = ''
            self.top_q = ''
            self.ptrs = ''
            self.tname = ''

        # def visit_Decl(self, node):
        #   self.name = node.name
        #   self.top_q = " ".join(node.quals)
        #   self.visit(node.type)

        def visit_TypeDecl(self, node):
            self.name = node.declname
            if self.name == None:
                self.name = ''
            self.top_q = " ".join(node.quals)
            self.visit(node.type)


        def visit_PtrDecl(self, node):
            self.ptrs += '*'
            self.visit(node.type)

        def visit_ArrayDecl(self, node):
            if node.dim != None:
                self.arity += '[' + node.dim.value + ']'
            self.visit(node.type)

        def visit_IdentifierType(self, node):
            self.tname = " ".join(node.names)

        def visit_FuncDecl(self, node):
            ret = FuncDefVisitor()
            ret.visit(node.args)
            _, c = node.type.children()[0]
            rettype = ' '.join(c.names)
            self.tname = rettype + ' (CL_CALLBACK *)('
            for arg in ret.args:
                self.tname += arg['type']
                if not arg['name'].startswith('arg'):
                    self.tname += ' ' + arg['name']
                self.tname += ', '
            self.tname = self.tname[0:-2] + ')'
            self.name = node.type.declname
            if self.name == None:
                self.name = ''


        def show_full(self):
            full = " ".join([self.top_q, self.tname + self.ptrs, self.name + self.arity])
            full = full.strip()
            return full

        def show_type(self):
            full = " ".join([self.top_q, self.tname + self.ptrs])
            full = full.strip()
            return full

    #parse it and output ast
    class FuncDefVisitor(c_ast.NodeVisitor):
        def __init__(self):
            self.decl = ''
            self.args = []

        def visit_Typedef(self, node):
            pass

        def visit_FuncDecl(self, node):
            ret = ParamListVisitor()
            ret.visit(node.type)
            whole = ret.show_full().strip('? ')

            self.args = []
            self.decl = whole + '('
            if node.args != None:
                self.visit(node.args)
            self.decl = self.decl.strip(', ')
            self.decl += ')'

            if ret.name not in functions:
                functions[ret.name] = {}
                functions[ret.name]['type'] = ret.show_type()
                functions[ret.name]['args'] = self.args

        def visit_ParamList(self, node):
            for param in node.params:
                argvisitor = ParamListVisitor()
                argvisitor.visit(param)
                arg = argvisitor.show_full()
                if arg.lower() != 'void':
                    self.decl += argvisitor.show_full() + ', '
                arg_def = {}
                arg_def['type'] = argvisitor.show_type()
                if argvisitor.name == '':
                    argvisitor.name = 'arg' + str(len(self.args))
                arg_def['name'] = argvisitor.name
                arg_def['arity'] = argvisitor.arity
                self.args.append(arg_def)

    ast = parse_file('tmp')
    v = FuncDefVisitor()
    v.visit(ast)
    os.unlink('tmp')


def arg_decl(fdata, name=False):
  args = fdata['args']
  content = "\nretV=RetDef(type='"+ fdata['type']+ "')"

  for i, arg in enumerate(args):
    content += ",\narg"+str(i+1)+"=ArgDef(name='"+arg["name"]+"',"
    if arg['type'] == 'void (CL_CALLBACK *)(cl_program program, void* user_data)*':
        content += "type='CallbackProgram',"
    elif arg['type'] == 'void (CL_CALLBACK *)(const char*, const void*, size_t, void*)*':
        content += "type='CallbackContext',"
    elif arg['type'] == 'void (CL_CALLBACK *)(cl_event, cl_int, void*)*':
        content += "type='CallbackEvent',"
    elif arg['type'] == 'void (CL_CALLBACK *)(cl_mem memobj, void* user_data)*':
        content += "type='CallbackMem',"
    elif arg['type'] == 'void (CL_CALLBACK *)(void*)*':
        content += "type='CallbackFunc',"
    else:
        content += "type='"+arg['type']+"',"
    content = content.strip(',')
    content += ')'
  return content

output = open(infile, 'a')
def make_drivers(functions, version):
  content = ''
  for key in sorted(functions.keys()):
    value = functions[key]
    function_exist = False
    for elem in functions_name:
      if key == elem:
        function_exist = True
    if function_exist == False:
      content += "Function(name='" + key +"',enabled=False,availableFrom='" + version + "',"
      if key.endswith(tuple(extensions)):
          content += "extension=True,"
      else:
          content += "extension=False,"
      if key.startswith('clCreate'):
          content += 'type=Creator'
      elif key.startswith('clRetain'):
          content += 'type=Retain'
      elif key.startswith('clRelease'):
          content += 'type=Release'
      elif key.endswith('Info'):
          content += 'type=Info'
      elif 'Enqueue' in key:
          content += 'type=Enqueue'
      else:
          content += 'type=None'
      content += ',' + arg_decl(value, True)
      content += "\n)\n\n"

  if infile != '' and content != '':
    infile_text = open(infile, 'a')
    try:
      infile_text.write(content)
    finally:
      infile_text.close()
  else:
    output.write(content)

read_file()
make_drivers(functions, sys.argv[1])
