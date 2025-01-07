# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from markdown.extensions import Extension
from markdown.preprocessors import Preprocessor
import re

class StripCommentsPreprocessor(Preprocessor):
    COMMENT_RE = re.compile(r'<!--.*?-->', re.DOTALL)

    def run(self, lines):
        text = "\n".join(lines)
        text = self.COMMENT_RE.sub('', text)
        return text.split("\n")

class StripCommentsExtension(Extension):
    def extendMarkdown(self, md):
        md.preprocessors.register(StripCommentsPreprocessor(md), 'strip_comments', 25)

def makeExtension(**kwargs):
    return StripCommentsExtension(**kwargs)
