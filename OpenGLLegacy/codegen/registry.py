#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Minimal OpenGL/WGL/GLX/EGL XML registry parser using the Python standard
# library. Replaces the lxml-based reg.py from the Khronos OpenGL-Registry.
#
# Only the subset needed by metagenerator.py is implemented:
#   - Registry.loadFile(path)   -- parse an XML registry file
#   - Registry.enumdict         -- {key: ET.Element} for each <enum>
#   - Registry.cmddict          -- {key: ET.Element} for each <command>
#
# The dict key matches the addElementInfo convention from reg.py:
#   - (name, api)  when the element carries an 'api' attribute
#   - name         otherwise
# First entry wins; duplicates are silently ignored.

import xml.etree.ElementTree as ET
from pathlib import Path


class Registry:
    """Minimal registry that loads enum and command data from an XML file."""

    _DictKeyType = str | tuple[str, str]

    def __init__(self) -> None:
        self.enumdict: dict[Registry._DictKeyType, ET.Element] = {}
        self.cmddict: dict[Registry._DictKeyType, ET.Element] = {}

    def loadFile(self, path: str | Path) -> None:
        """Parse an XML registry file and populate enumdict and cmddict."""
        root = ET.parse(path).getroot()
        self._load_enums(root)
        self._load_commands(root)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _key(elem: ET.Element, name: str) -> str | tuple[str, str]:
        """Return the dict key for an element (mirrors reg.py addElementInfo)."""
        api = elem.get('api')
        if api is not None:
            return (name, api)
        return name

    def _load_enums(self, root: ET.Element) -> None:
        for enum in root.findall('enums/enum'):
            name = enum.get('name', '')
            key = self._key(enum, name)
            if key not in self.enumdict:
                self.enumdict[key] = enum

    def _load_commands(self, root: ET.Element) -> None:
        for cmd in root.findall('commands/command'):
            # reg.py injects 'name' from <proto><name> when the attribute is absent.
            if 'name' not in cmd.attrib:
                proto = cmd.find('proto')
                if proto is not None:
                    name_elem = proto.find('name')
                    if name_elem is not None:
                        cmd.attrib['name'] = name_elem.text or ''
            name = cmd.get('name', '')
            key = self._key(cmd, name)
            if key not in self.cmddict:
                self.cmddict[key] = cmd
