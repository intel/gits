---
icon: material/puzzle
title: Plugins
---

GITS supports **plugins** for both capture and replay on DirectX 12 and Vulkan. Plugins are dynamic libraries loaded at runtime by `gitsPlayer.exe` or `gitsRecorder.dll`. Each plugin implements [`IPlugin`](../../plugins/IPlugin.h) and exposes a layer or service hooked into the recorder or player.

Built plugins install under an API-specific tree:

| API | Install directory |
|-----|-------------------|
| DirectX 12 | `Plugins/DirectX/` |
| Vulkan | `Plugins/Vulkan/` |

See the per-API catalogs for the full plugin list and API-specific enablement:

- [DirectX plugins](DirectX/Plugins.md)
- [Vulkan plugins](Vulkan/Plugins.md)

# On-disk layout

Each plugin folder contains:

- `plugin.dll`: Main library containing the plugin logic.
- `config.yml`: Configuration file (defaults, documentation, and `Info` metadata).
- `dependencies/`: Optional directory with extra DLLs the plugin depends on.

# config.yml

Every plugin ships a `config.yml` beside `plugin.dll` with two top-level sections:

```yaml
Info:
  Name: 'HelloPlugin'       # Must match IPlugin::getName()
  Description: '...'

Config:
  SomeOption: true          # Plugin-specific keys
```

The **Name** in `Info` must match the string returned by `IPlugin::getName()` and the name you list in GITS configuration when enabling the plugin. Defaults and detailed comments live in each plugin’s `config.yml`.

At runtime, plugins load this file through `gits::LoadPluginConfig` in [`plugins/common/pluginUtils.h`](../../plugins/common/pluginUtils.h), which applies [environment overrides](#environment-overrides) before the plugin reads `Config` keys.

# Enabling plugins

Which YAML keys enable plugins depends on the API backend:

| API | Recorder | Player |
|-----|----------|--------|
| DirectX 12 | `DirectX.Recorder.Plugins` | `DirectX.Player.Plugins` |
| Vulkan | `Common.Recorder.Plugins` | `Common.Player.Plugins` |

Each list entry must match `IPlugin::getName()` / `Info.Name`. See [DirectX plugins](DirectX/Plugins.md#usage) and [Vulkan plugins](Vulkan/Plugins.md#usage) for examples.

# Environment overrides

Plugin settings in each folder’s `config.yml` can be overridden with environment variables without editing the install tree. This applies to **DirectX and Vulkan** plugins the same way.

Variables use the form `GITS_PLUGIN_<PLUGIN>_<KEY>=<value>`:

| Piece | Rule | Example |
|-------|------|---------|
| Prefix | `GITS_PLUGIN_` | |
| Plugin | `Info.Name` / `IPlugin::getName()`, uppercased | `HelloPlugin` → `HELLOPLUGIN` |
| Key | YAML key under `Config`, uppercased | `PrintFrames` → `PRINTFRAMES` |

> Only **scalar keys directly under `Config:`** are overridden. Nested maps, sequences (for example hook lists), and other structures are not supported yet.

Only variables that match a key present in the loaded `config.yml` are read; other `GITS_PLUGIN_*` variables are ignored.

Example for **HelloPlugin** during DirectX playback:

```cmd
set GITS_PLUGIN_HELLOPLUGIN_PRINTFRAMES=false
set GITS_PLUGIN_HELLOPLUGIN_PRINTGPUSUBMISSIONS=false
gitsPlayer --DirectX.Player.Plugins="[HelloPlugin]" path\to\stream.gits2
```

Example for **HelloPlugin** during Vulkan playback:

```cmd
set GITS_PLUGIN_HELLOPLUGIN_PRINTQUEUESUBMITS=false
gitsPlayer --Common.Player.Plugins="[HelloPlugin]" path\to\stream.gits2
```

When an override is applied, `gitsPlayer` logs the environment variable name and value at INFO level.

# Interceptor only

Plugins can run without recording a stream. Disable the recorder and load a plugin as usual:

```yml
Common:
  Recorder:
    Enabled: false
```
