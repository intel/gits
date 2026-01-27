# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Format all the .h and .cpp files in the generated/ directory using clang-format
# Invoked from CMake during the first configuration

#!/usr/bin/env python3
import subprocess
import multiprocessing
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

def format_file(file_path):
    """Format a single file"""
    try:
        subprocess.run(['clang-format', '-i', '-style=file', str(file_path)], 
                      capture_output=True, check=True)
        return True
    except:
        return False

def main():
    # Find all .h and .cpp files in generated/ directory
    generated_dir = Path("generated")
    if not generated_dir.exists():
        print("No 'generated' directory found")
        return -1
    
    files = []
    files.extend(generated_dir.glob('**/*.h'))
    files.extend(generated_dir.glob('**/*.cpp'))
    if not files:
        print("No C++ files found in 'generated' directory")
        return -1
    
    # Format using all CPU cores
    with ThreadPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
        results = list(executor.map(format_file, files))
    
    success_count = sum(results)
    print(f"Formatted {success_count}/{len(files)} files")

    return 0 if success_count == len(files) else -1

if __name__ == "__main__":
    main()