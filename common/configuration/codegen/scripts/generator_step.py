# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os

from enum import Enum


class Step(Enum):
    ARGUMENTS = "Argumentparser"
    CONFIG = "Configuration"
    ENUMS = "Enum"
    DOCS_ENUMS = "DocsEnum"
    DOCS_CONFIG = "DocsConfiguration"
    DEFAULT_CONFIG = "DefaultConfiguration"

    def __str__(self):
        return self.value

    @staticmethod
    def from_string(value):
        try:
            return Step(value)
        except ValueError:
            return None
