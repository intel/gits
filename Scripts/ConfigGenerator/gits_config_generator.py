#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import os
import argparse
from pathlib import Path
from mako.template import Template


def mako_write(template_path, outpath, platform, install_path):
  template = Template(filename=template_path)
  rendered = template.render(platform=platform, install_path=install_path)
  rendered = re.sub(r"\r\n", r"\n", rendered)
  with open(outpath, 'w') as fout:
    fout.write(rendered)


def prepare_install_path(platform, installpath):
  install_path = installpath
  if platform == "win32":
    install_path = install_path.replace("\\", "\\\\")
  return install_path


def setup_parser():
    parser = argparse.ArgumentParser(description="Config generator")
    parser.add_argument('--templatepath', type=Path, help='Mako template path', required=True)
    parser.add_argument('--outputpath', type=Path, help='Output path', required=True)
    parser.add_argument('--platform', help='Target platform', required=True)
    parser.add_argument('--installpath', help='Installation path', required=True)
    return parser


def main(args=None):
  if not args:
    parser = setup_parser()
    args = parser.parse_args()
  template_path = os.path.abspath(args.templatepath)
  output_path = os.path.abspath(args.outputpath)
  install_path = prepare_install_path(args.platform, os.path.abspath(args.installpath))
  mako_write(template_path, output_path, args.platform, install_path)


if __name__ == '__main__':
  main()
