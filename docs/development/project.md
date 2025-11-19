---
title: Project Guide
icon: material/map-marker-path
---

## General information

- The project uses **CMake**.
- The main code is written in **C++ 20**.
- Scripts are written in **Python 3.10**.
- Code generation is done using **mako-templates**.

## Naming conventions

### Files, folders & structure

The general guide to naming conventions is **camelCase**. It's true for most items:

- **folders**
  - except: API's should maintain **the original spelling/capitalization** as much as possible.
- **files**
  - except: Python code files should be snake case'd.
- **binaries**

There's **one folder per API** that contains the functional implementation for that API.

Generated files should have the suffix `Auto`.

## C++

### Code style

The code style for GITS is based on the following rules:

| Element                   | Example           | Description                                  |
| ------------------------- | ----------------- | -------------------------------------------- |
| Constants                 | `CRITICAL_CLASS`  | all capitals, `_` between words in name      |
| Namespaces                | `gits`            | **camel case**, no `_` in name               |
| Classes, Enums, Structs   | `ClassyClass`     | **Pascal case**, no `_` in name              |
| Functions                 | `ClassyEnough()`  | **Pascal case**, no `_` in name              |
| (class member) Variables  | `m_ClassLevel`    | **Pascal case**, `m_` prefix, no `_` in name |
| (struct member) Variables | `FunkyBunchCount` | **Pascal case**, no `_` in name              |
| (global) Variables        | `g_MaxClassLevel` | **Pascal case**, `g_` prefix, no `_` in name |
| ------------------------- | ----------------- | -------------------------------------------- |
| Linewidth                 |                   | 100                                          |
| Clang-Format              |                   | \.clang-format                               |
| Clang-Tidy                |                   | \.clang-tidy                                 |

Newly contributed code to a module should adhere to the style of the existing code. However there's an ongoing effort to update the code style of old modules.
New modules/projects should always use the code style.

