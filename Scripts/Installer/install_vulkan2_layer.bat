@echo off
REM ===================== begin_copyright_notice ============================
REM
REM Copyright (C) 2023-2026 Intel Corporation
REM
REM SPDX-License-Identifier: MIT
REM
REM ===================== end_copyright_notice ==============================

REM Registers the NEW Vulkan2 GITS recorder layer manifest (gits_recorder.json,
REM layer VK_LAYER_INTEL_vulkan_GITS_recorder_2). This is the Vulkan2 counterpart
REM of install_vulkan_layer.bat, which registers the legacy layer. Both layers
REM may be registered side by side (they are explicit layers); enable only one to
REM record for a given application.

set registry_path=HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers
set registry_path_32=HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers
set layer_json_name=gits_recorder.json

if "%1"=="Win32" goto Win32
if "%1"=="x64" goto x64

:Win32
(for /f "usebackq" %%a in (`reg query "%registry_path_32%" /S ^| find "%layer_json_name%"`) do (
  reg delete %registry_path_32% /v %%a /f
  echo The previous 32bit GITS Vulkan2 recorder layer has been removed from the registry
)) 2> NUL

reg add "%registry_path_32%" /v %~f2%layer_json_name% /t REG_DWORD /d 0
if %errorlevel%==0 (
  echo 32bit GITS Vulkan2 recorder layer has been added to the registry
) else (
  echo Couldn't add layer to registry
)
goto:EOF

:x64
(for /f "usebackq" %%a in (`reg query "%registry_path%" /S ^| find "%layer_json_name%"`) do (
  reg delete %registry_path% /v %%a /f
  echo The previous GITS Vulkan2 recorder layer has been removed from the registry
)) 2> NUL

reg add "%registry_path%" /v %~f2%layer_json_name% /t REG_DWORD /d 0
if %errorlevel%==0 (
  echo GITS Vulkan2 recorder layer has been added to the registry
) else (
  echo Couldn't add layer to registry
)
goto:EOF
