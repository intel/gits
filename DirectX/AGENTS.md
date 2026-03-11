# Guide for AI Agents — DirectX/

## Do not edit Auto files
Files ending in `*Auto` are generated. Edit the corresponding template in
`codegen/templates/` or the generator script, then regenerate:

```
cmake --build <build-dir> --target DirectX_codegen
```

When both `*Auto` and `*Custom` variants of a file exist, your changes
belong in the `*Custom` file. The `*Auto` file will be overwritten on the
next codegen run.

## command_ids.json — never renumber existing entries
`codegen/command_ids.json` assigns stable numeric IDs to every recorded
command. Renumbering an existing entry corrupts any stream recorded before
the change. Only append new entries.

## Player layers require two-step registration
Implementing the layer interface in `layers/<name>/` is not enough. You must
also register the layer in `player/playerLayerManager.cpp` or it will never
be activated.
