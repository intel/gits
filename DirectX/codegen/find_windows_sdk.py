# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os
import glob
import winreg

def get_windows_sdk():
    sdk_root = find_windows_kits_root()
    if not sdk_root:
        raise FileNotFoundError('Windows Kits root directory not found.')

    sdk_dirs = glob.glob(os.path.join(sdk_root, 'Include/10.*'))
    sdk_dirs.sort(reverse=True)
    if not sdk_dirs:
        raise FileNotFoundError(f'Windows SDKs not found in {sdk_root}')
    
    return sdk_dirs[0]

def find_windows_kits_root():
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Microsoft\Windows Kits\Installed Roots') as key:
            return winreg.QueryValueEx(key, 'KitsRoot10')[0]
    except FileNotFoundError:
        return None