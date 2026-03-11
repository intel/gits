---
title: Codebase Map
icon: material/file-tree
---

# Codebase Map

This page gives a short map of the repository layout so you can find where implementation lives for each component.

## Shared runtime components

| Directory | Contents |
|-----------|----------|
| `common/` | Shared C++ code used across APIs (e.g. configuration, imgui_common). |
| `player/` | Shared player entry point and utilities (`playerMain.cpp`, `player.cpp`, window/display helpers, statistics). API-specific player code lives in each API folder. |
| `recorder/` | Shared recorder DLL entry point and utilities (`recorderDLL.cpp`, `recorderIface.cpp`). API-specific recorder code lives in each API folder. |
| `plugins/` | Plugin build and API-specific plugins (e.g. DirectX plugins under `plugins/DirectX/`). |
| `launcher/` | Launcher component (application used to configure capture and playback sessions). |
| `ccode/` | CCode runner — the standalone C++ project produced by stream-to-CCode export. Contains `main.cpp`, window/data services, and a `directx/` subdirectory with generated DX12 replay code. |

## API implementation (one folder per API)

Each graphics API has its own top-level directory with recorder, player, and API-specific layers or plugins.

| Directory | Contents |
|-----------|----------|
| `DirectX/` | DirectX 12 recorder, player, layers (e.g. resource_dumping, subcapture, ccode), and common utilities. Windows x64 only. |
| `Vulkan/` | Vulkan implementation. |
| `OpenGL/` | OpenGL implementation. |
| `OpenCL/` | OpenCL implementation. |
| `LevelZero/` | Intel oneAPI Level Zero implementation. |
| `ocloc/` | Intel offline shader compiler (OCLOC). |

## Scripts and documentation

| Directory | Contents |
|-----------|----------|
| `Scripts/` | Python scripts for code generation, docs (e.g. MkDocs), CI helpers, and other tooling. |
| `docs/` | User and developer documentation (MkDocs). Key entries: `index.md`, `building.md`, `usage.md`, `documentation/terminology.md`, `development/project.md`, `development/codebase-map.md`. |

## Build support

| Directory | Contents |
|-----------|----------|
| `CMakeLists.txt` | Top-level CMake project; defines options `WITH_VULKAN`, `WITH_OPENCL`, `WITH_LEVELZERO`, `WITH_DIRECTX` (Windows x64), `WITH_LAUNCHER`, etc. |
| `cmake/` | CMake modules and helpers |
| `requirements.txt` | Python dependencies for scripts and codegen |
| `mkdocs.yml` | MkDocs configuration for the documentation in `docs/` |
| `third_party/` | Vendored dependencies (e.g. imgui, yaml-cpp, lua, lz4, zstd, Detours, Vulkan-Headers, AgilitySDK, DirectStorage, DirectXTex, json, libpng, tbb, xess, and more). Managed via `dependencies.yaml`. |


## Build outputs

| Directory | Purpose |
|-----------|---------|
| `build/` | Typical CMake build directory (configurable). |
| `build/dist` | Typical CMake install directory (configurable). |

## Notes

- **Recorder/Player/Stream concepts** → [terminology](../documentation/terminology.md)
- **Conventions and style** → [project guide](project.md)
- **Generated files** → Any file whose name ends with `Auto`; do not edit by hand—change sources and regenerate.
