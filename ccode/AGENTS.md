# AGENTS.md – CCode (GITS export) project

This file gives AI agents context so they can work effectively in this repo.

## What this project is

- **CCode** is a C++ export from **GITS** (Intel’s tool for capture and replay of graphics API calls).
- This tree is a **replayer**: it builds an executable that restores D3D12 + Intel extension state and replays frames from a recorded trace.
- **Generated code** (from GITS) lives under **`generated/`**. The rest is the **CCode runtime** (entrypoint, services, DirectX helpers and API wrappers).

## Do not modify generated files

- **Do not edit** anything under **`generated/`** (e.g. `commands.cpp`, `commands_staterestore_*.cpp`, `commands_frame_*.cpp`, `objects.h`, `config.cmake`, `data.bin`).
- Those files are produced by GITS and will be overwritten on re-export. Changes belong in the runtime (see below).

## Where to make changes

| Area | Path | Purpose |
|------|------|--------|
| Entrypoint | `main.cpp` | Startup, logging, `data.bin` path, `StateRestore()` / `RunFrames()` flow |
| Data I/O | `dataService.h`, `dataService.cpp` | Opening and reading `data.bin` for replay |
| Window / UI | `windowService.h`, `windowService.cpp` | Window creation / management if needed for replay |
| DirectX helpers | `directx/*.h`, `directx/*.cpp` (except wrappers) | Descriptor heaps, GPU addresses, map tracking, heap allocation, utils |
| API wrappers | `directx/wrappers/ccodeApiWrappers*.h`, `ccodeApiWrappers*.cpp` | Thin wrappers called by generated code to invoke D3D12 and Intel extension APIs; change only when extending or fixing the API surface used by GITS output |
| Build | `CMakeLists.txt` | Sources, includes, libraries, post-build (e.g. Agility SDK copy), format step |
| Formatting | `clang_format.py` | Script used by CMake to format generated C++ on first configure |

## Tech stack

- **Language**: C++20.
- **Build**: CMake 3.13+; executable name comes from `generated/config.cmake` (e.g. `b1-Win64-Shipping`).
- **Graphics**: D3D12 (Agility SDK in `third_party/AgilitySDK/`), Intel extensions (`third_party/IntelExtensions/`, `igdext64.lib`).
- **Logging**: plog. **CLI**: args.hxx (included; minimal use in current `main`).

## Important concepts

- **StateRestore**: Replays all “state” commands (create device, heaps, resources, descriptor views, etc.) from the trace; implemented as many `StateRestore_XXXXX_YYYYY()` functions in generated code.
- **RunFrames**: Replays per-frame work (e.g. `Frame_0()`, `Frame_1()`), each implemented by generated `Frame_N_XXXXX_YYYYY()` that call D3D12/Intel APIs (e.g. `Present`).
- **Objects**: Generated `objects.h` defines global D3D12/DXGI object pointers (`g_O12`, `g_O16`, `g_O24`, …). Generated code refers to them by numeric ID (e.g. `O30`, `g_O169`); the runtime and wrappers use the same globals.
- **data.bin**: Binary trace data (e.g. resource contents); path is set via `DATA_BIN_PATH` and/or next to the executable at run time.

## Build and run

- Configure and build with CMake (e.g. `out/build/x64-Debug`). First configure runs `clang_format.py` on generated C++.
- Runtime needs **`data.bin`** at the path from `DATA_BIN_PATH` or next to the executable, and **D3D12 Agility SDK** DLLs under `D3D12/` next to the exe (CMake POST_BUILD copies them).

## Code style (GITS conventions)

Runtime code follows GITS code style:

| Element | Example | Description |
|--------|---------|-------------|
| Constants | `CRITICAL_CLASS` | All capitals, `_` between words in name |
| Namespaces | `gits` | **camel case**, no `_` in name |
| Classes, Enums, Structs | `ClassyClass` | **Pascal case**, no `_` in name |
| Functions | `ClassyEnough()` | **Pascal case**, no `_` in name |
| (class member) Variables | `m_ClassLevel` | **Pascal case**, `m_` prefix, no `_` in name |
| (struct member) Variables | `FunkyBunchCount` | **Pascal case**, no `_` in name |
| (global) Variables | `g_MaxClassLevel` | **Pascal case**, `g_` prefix, no `_` in name |

Additional runtime habits: singleton services (`DataService::Get()`), `directx::` namespace for DX helpers, plog for logging. Include `directx/directx.h` and the relevant service headers when touching D3D12/Intel objects. Keep the runtime independent of trace-specific details (frame counts, object IDs); those stay in `generated/`.

## GITS contract (runtime API called by generated code)

These APIs are invoked from **generated** code emitted by GITS. Changing their signatures or semantics will break the exporter until GITS is updated; document any new hooks here and in `.cursor/rules/`.

- `ScreenshotService::Get().RegisterSwapChain(id, swapChain, device, queue)` — after swap chain creation; `id` is the object ID (e.g. 169 for g_O169).
- `ScreenshotService::Get().CaptureFrame(id)` — after each Present for that swap chain.
