@echo off

REM ===================== begin_copyright_notice ============================
REM
REM Copyright (C) 2023-2024 Intel Corporation
REM
REM SPDX-License-Identifier: MIT
REM
REM ===================== end_copyright_notice ==============================

goto check

:usage
echo " usage: build_ccode.bat <type> "
echo " type:                         "
echo "   vs     - build using vs     "
echo "   ninja  - build using ninja  "
goto:EOF

:check
if "%1"=="" goto usage
if "%1"=="vs" goto vs_build
if "%1"=="ninja" goto ninja_build
if "%1"=="vs_empty" goto vs_empty_build
goto usage

:vs_build
call :setup_env
cmake . -B build_vs
cmake --build build_vs --config RelWithDebInfo
cd build_vs/RelWithDebInfo
move *.exe ../../ >NUL
move *.lib ../../ >NUL
move *.pdb ../../ >NUL
cd ../../
goto:EOF

:ninja_build
call :setup_env
cmake . -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cd build_ninja
ninja
move *.exe ../ >NUL
move *.lib ../ >NUL
move *.pdb ../ >NUL
cd ../
goto:EOF

:vs_empty_build
call :setup_env
cmake . -B build_vs_empty -DEMPTY_BUILD=1
cmake --build build_vs_empty --config RelWithDebInfo
cd build_vs_empty/RelWithDebInfo
move *.exe ../../ >NUL
move *.lib ../../ >NUL
move *.pdb ../../ >NUL
cd ../../
goto:EOF

:setup_env
if not defined DevEnvDir (
  FOR /F "tokens=* USEBACKQ" %%F IN (`where /r "C:\Program Files\Microsoft Visual Studio" vcvarsall.bat 2^>nul ^|^| where /r "C:\Program Files (x86)\Microsoft Visual Studio" vcvarsall.bat 2^>nul`) DO (
    set "envpath=%%F"
	goto runenv
  )
  :runenv
  if "%envpath%"=="" echo Cannot setup environment! && goto :eof
  call "%envpath%" x64
)
