# Guide for AI Agents

This file helps AI assistants work effectively in the **GITS** (Graphics Intercept and Trace Solution) repository.

## What is GITS?

GITS is a capture-replay tool for Vulkan, OpenCL, Intel oneAPI Level Zero, OpenGL, and DirectX 12. It records API call sequences into binary **streams** and replays them via **gitsPlayer**. The codebase is C++ (core), Python (scripts and code generation), and CMake (build). Development focuses on enabling applications for Intel GPU driver validation.

## Where to Find Things

| Need | Location |
|------|----------|
| **Conventions, code style, naming** | [docs/development/project.md](docs/development/project.md) |
| **Terminology** (Stream, Recorder, Player, Configuration, Subcapture, CCode, Generator) | [docs/documentation/terminology.md](docs/documentation/terminology.md) |
| **Build instructions** | [docs/building.md](docs/building.md), [README.md](README.md) |
| **Codebase layout** | [docs/development/codebase-map.md](docs/development/codebase-map.md) |
| **Contributing** (PRs, commits, DCO) | [CONTRIBUTING.md](CONTRIBUTING.md) |
| **Usage** (record/replay, interceptor, layer) | [docs/usage.md](docs/usage.md) |

In-repo documentation under `docs/` is the source of truth when external links are unavailable.

## Critical Rules

1. **Do not edit generated files.** Files with the suffix `Auto` are generated (e.g. from mako templates or codegen). Change the sources or generators and regenerate instead.
2. **Follow project conventions.** Use [docs/development/project.md](docs/development/project.md): C++ style (Pascal case, `m_`/`g_` prefixes, 100-column line width, `.clang-format`/`.clang-tidy`), camelCase for folders/files (Python: snake_case), and API folders keep original API spelling/capitalization.
3. **One folder per API.** API-specific implementation lives under `DirectX/`, `Vulkan/`, `OpenCL/`, `LevelZero/`. Shared code is in `common/`; plugins in `plugins/`.

## Tech Stack

- **Build:** CMake
- **Core:** C++20
- **Scripts / codegen:** Python 3.10
- **Templates:** mako

When suggesting changes, match existing patterns in the same module and prefer the style described in the project guide for new code.
