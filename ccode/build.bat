@echo off

REM ===================== begin_copyright_notice ============================
REM
REM Copyright (C) 2023-2026 Intel Corporation
REM
REM SPDX-License-Identifier: MIT
REM
REM ===================== end_copyright_notice ==============================

setlocal
cd /d "%~dp0"
cmake . -B build
if errorlevel 1 exit /b %ERRORLEVEL%
cmake --build build
exit /b %ERRORLEVEL%
