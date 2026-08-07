---
icon: simple/vulkan
title: Plugins
---

Vulkan plugins extend capture and replay for the **Vulkan** backend. Built plugins install under `Plugins/Vulkan/`. For shared concepts (`config.yml`, environment overrides, interceptor-only runs), see [Plugins](../Plugins.md).

# Usage

Enable plugins under `Common.Recorder` or `Common.Player` using the `Plugins` list. Each entry must match the name returned by the DLL (`IPlugin::getName()`), which should match the `Info.Name` value in that plugin’s `config.yml`.

```yaml
Common:
  Recorder:
    Plugins: [] # List of plugins to enable during capture
```

```yaml
Common:
  Player:
    Plugins: [] # List of plugins to enable during playback
```

Example: enable `HelloPlugin` during Vulkan playback:

```yaml
Common:
  Player:
    Plugins: ['HelloPlugin']
```

On **Linux**, the Vulkan plugin service currently skips loading plugins and logs that Vulkan plugins are not yet supported on that platform. Use **Windows** for Vulkan plugin workflows today.

## Plugin names

Use the **Name** column from the [summary table](#vulkan-plugins). Full defaults and comments are in each plugin’s `config.yml` beside `plugin.dll`.

## Configuration overrides

Scalar `Config:` keys can be overridden with `GITS_PLUGIN_*` environment variables. See [Environment overrides](../Plugins.md#environment-overrides) in the general plugins guide.

# Vulkan plugins

| Name | Folder | Summary |
|------|--------|---------|
| `HelloPlugin` | `hello_plugin` | Example plugin; logs queue presents and queue submits when enabled. |
| `Benchmark` | `benchmark` | Writes CPU present-to-present frame times to a CSV file and logs average FPS. |
| `Statistics` | `statistics` | Aggregates Vulkan API call statistics to a YAML report. |

**Benchmark** measures CPU time between presents. **Statistics** counts API usage. They are complementary, not interchangeable.


