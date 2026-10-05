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

1. **Do not edit generated files.** Files with the suffix `Auto` are generated (e.g. from mako templates or codegen). Change the sources or generators and regenerate instead. Files with the suffix `Custom` are manually written companion/override files to the `Auto` files.
2. **Follow project conventions.** Use [docs/development/project.md](docs/development/project.md): C++ style (Pascal case, `m_`/`g_` prefixes, 100-column line width, `.clang-format`/`.clang-tidy`), camelCase for folders/files (Python: snake_case), and API folders keep original API spelling/capitalization.
3. **One folder per API.** API-specific implementation lives under `DirectX/`, `Vulkan/`, `OpenCL/`, `LevelZero/`. Shared code is in `common/`; plugins in `plugins/`.
4. **Chat and planning context should not bleed into code.** Don't add unnecessary references to conversations or planning documents, and don't adopt their terminology when standard GITS terminology exists. Code and comments should stand on their own.
5. **When writing a commit message, follow these rules:**
   - Separate subject from body with a blank line
   - Limit the subject line to 50 characters
   - Capitalize the subject line
   - Do not end the subject line with a period
   - Use the imperative mood in the subject line
   - Wrap the body at 72 characters
   - Use the body to explain what and why, not how
   - Start subject with relevant scope markers like [Vk], [DX12], [L0], [common], [docs], etc. Don't go overboard with them.
   - Do not start commit messages with BOM.
   

## Tech Stack

- **Build:** CMake
- **Core:** C++20
- **Scripts / codegen:** Python 3.10
- **Templates:** mako

When suggesting changes, match existing patterns in the same module and prefer the style described in the project guide for new code.


