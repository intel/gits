---
icon: octicons/package-dependencies-24
title: Dependencies

---

# Dependencies

Third party dependencies are managed by an internal system: 

- `/Scripts/install_dependencies.py` manages the install & update,
- `/cmake/thirdparty_system.cmake` contains the cmake installation logic and
- `/third_party/dependencies.yaml` stores all dependencies + required information.

Dependencies can be either *git repositories* or *nuget packages*. Patches are automatically applied if they are placed in the appropriate folder `/third_party/patch/<dependency_name>`.

## Adding a dependency

To add a new dependency:

1. Add a new entry to the the `third_party/dependencies.yaml`.
2. Create a new `cmake/import_<dependency_name>.cmake` file.
3. Include the `.cmake` file in the appropriate `CMakeLists.txt`.


### Dependencies.yaml

Every dependency is registered in `/third_party/dependencies.yaml`, here's an example:

```yaml
dependencies:
  NugetPackages:
    - name: AgilitySDK
      argument: agility-sdk
      package_url: https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/
      version: 1.717.1-preview
      os: WINDOWS
      folder_to_extract: build/native/
  GitRepositories:
    - name: argshxx
      url: https://github.com/Taywee/args.git
      commit_id: 114200a9ad5fe06c8dea76e15d92325695cf3e34
    - name: yaml-cpp
      argument: yamlcpp
      url: https://github.com/jbeder/yaml-cpp.git
      branch: 0.8.0
    - name: Detours
      url: https://github.com/microsoft/Detours.git
      branch: main
      commit_id: 4b8c659f549b0ab21cf649377c7a84eb708f5e68
      os: WINDOWS
```

- The optional `os` attribute allows to limit the installation to certain platforms
- The optional `argument` allows to define a simple/unique dependency name that can be used when passing it as an argument to the python install script, e.g. `--withagility-sdk`, `--with-yamlcpp`, `--with-argshxx` and `--with-detours` for the above example.


### CMake
For each dependency a .`cmake` file needs to be created (e.g. `import_imgui.cmake`) that adds the dependency to the install system and sets up required cmake variables, ...

#### Dependency without post-install function

If the dependency _only_ has to be installed and some simple cmake variables have to be set up the cmake function is straight forward, e.g. `import_argshxx.cmake`:

```cmake
if(NOT DEFINED ARGS_SOURCE_DIR)
  add_thirdparty_arg("--with-argshxx")
  set(ARGS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/argshxx")
endif()
```

The first call registers the dependency for installation. The name has to match the dependency name in the `dependencies.yaml`.

#### Dependency with post-install function

In a more complex scenario the dependency requires a setup step that is based on the content of the installed dependency, e.g. setting cmake properties of files requires the files to be present. This can be realized by registering the dependency for install along with a cmake function that should be called post install, e.g. `import_detours.cmake`:

```cmake
if(NOT DEFINED DETOURS_ROOT)
  add_thirdparty_arg_setup("--with-detours" init_detours)
  set(DETOURS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Detours/src")
endif()

function(init_detours)
  add_library(detours STATIC)
  
  target_sources(detours
  PRIVATE
    ${DETOURS_ROOT}/detours.cpp
    ${DETOURS_ROOT}/detours.h
    ${DETOURS_ROOT}/disasm.cpp
    ${DETOURS_ROOT}/modules.cpp
  )
  set_target_properties(detours PROPERTIES FOLDER External)
  
  include_directories(SYSTEM ${DETOURS_ROOT})
endfunction()
```

