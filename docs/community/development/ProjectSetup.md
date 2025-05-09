---
icon: material/cogs
---

## Conventions

Folders should be lower cased, unless required by existing names, e.g. APIs like `DirectX`, `OpenGL`

## Third Party Dependencies

Third party dependencies are managed by an internal script: `/Scripts/install_dependencies.py`, along with a corresponding `.cmake` file in the `/cmake`-folder and its inclusion in `/CMakeLists.txt`. 

The system is able to patch thirdparties as required, re-download in case of network issues and generally very flexible regarding third party sources and potential post-processing.

### install_dependency.py

To add a new dependency you have to

1. Create class representing the repository, e.g.  
  ```python
  class ArgsHXX(Repository):
    def set_commit(self):
        self.commit_id = "114200a9ad5fe06c8dea76e15d92325695cf3e34"

    def init(self):
        self.name = "argshxx"
        self.url = "https://github.com/Taywee/args.git"
  ```
1. Ensure that the repository is "enabled" for download
  ```python
  if args.with_all or args.with_argshxx:
      self.repos.append(ArgsHXX())
  ```
1. Add a commandline argument to make it callable from outside, e.g.  
  ```python
  root_parser.add_argument("--with-argshxx", action="store_true")
  ```

### Add a .cmake file

Create a corresponding `.cmake` file that installs the dependency and sets required CMake-Variables so the third party can be used, e.g. `/cmake/import_argshxx.cmake`:

```CMake
if(NOT DEFINED ARGS_SOURCE_DIR)
  install_dependencies("--with-argshxx")
  set(ARGS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/argshxx")
endif()
```

### Import the third party

To add the third party to the project it needs to be added to `/CMakeLists.txt`, e.g.

```CMake
include(import_argshxx)
```

### Optional: patches

Patches for third party is automatically applied if it is placed in a subfolder of the designated patch folder: `/third_party/patch/`. The name of the subfolder is the name of the third party, e.g.
```
/third_party/patch/argshxx/0001-tags-to-filter-hide-options-and-no-tests.patch
```