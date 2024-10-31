# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import os
import subprocess
import threading

def stream_output(stream, prefix):
    for line in iter(stream.readline, ''):
        print(f"{prefix}{line.strip()}")
    stream.close()

# Get the directory of the current script
current_script_dir = os.path.dirname(os.path.abspath(__file__))

# Change the working directory to two directories up
new_working_dir = os.path.abspath(os.path.join(current_script_dir, '..', '..'))
os.chdir(new_working_dir)

module_dir = os.path.join(current_script_dir, 'mkdocs_plugins')

# Add the Scripts/docs directory to the Python path
sys.path.insert(0, module_dir)

# Set the PYTHONPATH environment variable
env = os.environ.copy()
env['PYTHONPATH'] = module_dir + os.pathsep + env.get('PYTHONPATH', '')

# Run MkDocs and stream the output
process = subprocess.Popen(['mkdocs', 'serve'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

# Create threads to handle stdout and stderr
stdout_thread = threading.Thread(target=stream_output, args=(process.stdout, "STDOUT: "))
stderr_thread = threading.Thread(target=stream_output, args=(process.stderr, "STDERR: "))

# Start the threads
stdout_thread.start()
stderr_thread.start()

try:
    # Wait for the process to complete
    process.wait()
    # Wait for the threads to complete
    stdout_thread.join()
    stderr_thread.join()
except KeyboardInterrupt:
    process.terminate()
    print("Process terminated by user.")
