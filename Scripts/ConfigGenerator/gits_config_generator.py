#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import argparse
from pathlib import Path
from mako.template import Template


def mako_write(template_path, outpath, platform, install_path, is_compute):
  template = Template(filename=template_path)
  rendered = template.render(platform=platform, install_path=install_path, is_compute=is_compute)
  rendered = re.sub(r"\r\n", r"\n", rendered)
  with open(outpath, 'w') as fout:
    fout.write(rendered)


def setup_parser():
    parser = argparse.ArgumentParser(description="Config generator")
    parser.add_argument('--templatepath', type=Path, help='Mako template path', required=True)
    parser.add_argument('--outputpath', type=Path, help='Output path', required=True)
    parser.add_argument('--platform', help='Target platform', required=True)
    parser.add_argument('--installpath', help='Installation path', required=True)
    parser.add_argument('--compute', help='Internal build flag', action='store_true')
    return parser


def main(args=None):
  if not args:
    parser = setup_parser()
    args = parser.parse_args()
  template_path = Path(args.templatepath).absolute()
  output_path = Path(args.outputpath).absolute()
  install_path = Path(args.installpath).absolute()
  if not output_path.parent.exists():
    output_path.parent.mkdir(parents=True, exist_ok=True)
  mako_write(str(template_path), str(output_path), args.platform, str(install_path), args.compute)


if __name__ == '__main__':
  main()
