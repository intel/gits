---
icon: simple/opengl
title: Plugins
---

OpenGL plugins extend capture and replay for the new **OpenGL** backend. [OpenGL Legacy](OpenGLLegacy.md) does not load plugins. Built plugins install under `Plugins/OpenGL/`. For shared concepts (`config.yml`, environment overrides, interceptor-only runs), see [Plugins](../Plugins.md).

# Usage

Enable plugins under `Common.Recorder` or `Common.Player` using the `Plugins` list. Each entry must match the name returned by the library (`IPlugin::getName()`), which should match the `Info.Name` value in that plugin’s `config.yml`.

``` yaml
Common:
  Recorder:
    Plugins: [] # List of plugins to enable during capture
```

``` yaml
Common:
  Player:
    Plugins: [] # List of plugins to enable during playback
```

Example: enable `Statistics` during OpenGL playback:

``` yaml
Common:
  Player:
    Plugins: ['Statistics']
```

## Plugin names

Use the **Name** column from the [summary table](#opengl-plugins). Full defaults and comments are in each plugin’s `config.yml` beside the plugin library.

## Configuration overrides

Scalar `Config:` keys can be overridden with `GITS_PLUGIN_*` environment variables. See [Environment overrides](../Plugins.md#environment-overrides) in the general plugins guide.

# OpenGL plugins

| Name | Folder | Summary |
|------|--------|---------|
| `Benchmark` | `benchmark` | Writes CPU swap-to-swap frame times to a CSV file and logs the average FPS. |
| `Statistics` | `statistics` | Aggregates OpenGL API call statistics to a YAML report. |

**Benchmark** measures CPU time between swaps. **Statistics** counts API usage. They are complementary, not interchangeable.


