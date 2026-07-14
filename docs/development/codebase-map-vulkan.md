---
title: Vulkan Backend Codebase Map
icon: material/file-tree
---

# Vulkan Backend Codebase Map

This page maps the **new Vulkan backend** under `Vulkan/`, which uses a layer-based design similar to the DirectX 12 backend. It replaces the older implementation in `VulkanLegacy/` (still built by default alongside `Vulkan/`; see [codebase-map.md](codebase-map.md)). Everything below uses `namespace gits::vulkan` (lowercase); the legacy backend uses `gits::Vulkan` (capital).

As with most of GITS, any file ending in `Auto` is generated from a [mako](https://www.makotemplates.org/) template in `Vulkan/codegen/templates/` — never edit an `Auto` file by hand, edit the template (or the generator script / `*Custom` companion file) and regenerate. Files ending in `Custom` are hand-written companions that fill gaps the generator can't handle generically (unions, opaque/conditional structs, meta-commands with no real Vulkan call behind them, etc.).

## Top-level layout

| Directory | Contents |
|-----------|----------|
| `codegen/` | Python code generation: parses `vk.xml` into an intermediate representation and renders every `*Auto` file from mako templates. |
| `common/` | Code shared across the interceptor, native Vulkan layer, recorder, player and GITS layers/subcapture: the `Command`/`Layer`/`LayerGroup` object model, dispatch tables, argument/command byte-coders, and small cross-cutting services. |
| `interceptor/` | The DLL that impersonates the system Vulkan loader, intercepting calls made by the recorded application. |
| `layer/` | The native Vulkan **explicit layer** (Khronos loader mechanism) — an alternative interception front-door to the interceptor DLL. |
| `layers/` | Optional, configuration-gated GITS layers: `api_debug` (Vulkan error checking), `resource_dumping` (screenshots), `trace` (human-readable apicall logging). |
| `player/` | The replay-side (`gitsPlayer`) Vulkan implementation: command runners, player-side layer chain, and Vulkan-specific replay bookkeeping services. |
| `recorder/` | The recording-side implementation: capture session orchestration, recorder-side layer chain, and Vulkan-specific recording bookkeeping services. |
| `subcapture/` | Trimming a recorded (or replayed) stream down to a configured frame range, including state analysis/restoration. |

## The "layer" abstraction

In GITS terms, a **layer** is a set of `Pre(command)`/`Post(command)` functions run around every intercepted or replayed Vulkan apicall, chained together by a `LayerGroup`. This is the same concept used by the DirectX 12 backend. The base types live in `common/layer_interface/`:

- **`Command`** (command.h) — base class for one intercepted/replayed apicall: private `CommandId m_Id` (via `GetId()`; the `enum class CommandId` itself is generated into `commandIdsAuto.h`, with one stable numeric ID per command sourced from `command_ids.json`), plus public `m_Key`/`m_ThreadId`/`m_Skip` metadata. Generated subclasses (`commandsAuto.h`, one per Vulkan command, e.g. `vkCreateInstanceCommand`) store every parameter as an `m_`-prefixed `Argument<T>`/`PointerArgument<T>`/`HandleArgument<T>` member. Hand-written meta-commands with no real Vulkan call behind them (`StateRestoreBeginCommand`, `FrameEndCommand`, `MappedDataMetaCommand`, window create/update markers, etc.) live in `commandsCustom.h`.
- **`Layer`** (layerAuto.h, generated) — declares a no-op virtual `Pre`/`Post` overload for every `Command` type; concrete layers (encoder, recording, trace, api_debug, analyzer, customization, ...) override only what they care about.
- **`LayerGroup`** (layerGroup.h) — an owner/registry of named layers with a pure-virtual `loadLayers()`, so subclasses (`CaptureLayerManager`, `PlayerLayerManager`, `ResourceDumpingLayerGroup`, ...) can conditionally instantiate layers based on `Configurator` settings.
- **Dispatch tables** (dispatchTableAuto.h) — `VkGlobalLevelDispatchTable`/`VkInstanceLevelDispatchTable`/`VkDeviceLevelDispatchTable` structs of real driver `PFN_vk*` pointers (Vulkan has no vtable, so GITS models the loader's trampoline/terminator chain explicitly), looked up per-handle by `DispatchTablesHolder` (dispatchTablesHolder.h).

A recorded apicall therefore flows as: real function called → argument snapshot into a `Command` → handle-key translation → **Pre** layers → real driver call (via dispatch table) → output-handle registration → **Post** layers (the encoder layer, which serializes the command, always runs last). Replay mirrors this: stream `CommandId` → `Command` decode → handle-key resolution → **Pre** layers → real driver call → output-handle registration → **Post** layers.

## Code generation (`codegen/`)

The pipeline parses the Khronos `vk.xml` registry into a typed intermediate representation (IR), then renders every `*Auto` file from mako templates against that IR.

| File | Purpose |
|------|---------|
| intermediates.py | IR schema as dataclasses: `Parameter`, `Command`, `Member`, `Structure`, `Union`, `Handle`, `Enum`, `Bitmask`, `Flag` — type shape, const/pointer/array/length info, handle/struct/union classification, platform guards. |
| intermediates_creator.py | Parses `vk.xml` into the IR, resolving `alias=` chains and extension-contributed values; `postprocess` cross-references everything (transitive handle/struct/union membership, per-command dispatch level, output-handle params, structs-with-handles). |
| generator.py | Entry point/orchestrator: runs the parse+postprocess pipeline, builds the shared `context` (enums, bitmasks, unions, structures, handles, commands, command IDs), and invokes every `generate_*_files()` module, finishing with `plugin_generator`. |
| generator_helpers.py | Shared low-level utilities: declaration/length/enum-value parsing, dispatch-table/platform-`#ifdef` helpers, and the common `generate_file()` mako-render-and-write-and-clang-format routine used by every generator. |
| generator_layer.py | Generates `common/layer_interface/`: `commandIdsAuto.h`, `commandsAuto.h`, `layerAuto.h`, `dispatchTableAuto.h` — the core `Command`/`Layer`/dispatch-table object model. |
| generator_coders.py | Generates `common/coders/`: binary blob size/encode/decode logic per struct/command, plus shared handle-key collection/remapping helpers reused by recorder, player and subcapture. |
| generator_recorder.py | Generates `recorder/`: `wrappersAuto.*` (per-command recording wrapper), `encoderLayerAuto.*` (the layer serializing calls into the stream), `handleArgumentUpdatersAuto.*` (recorder-side handle-key collection). |
| generator_player.py | Generates `player/`: `commandRunnersAuto.*` (decode-and-replay per command), `vulkanCommandFactoryAuto.cpp` (command-id → runner dispatch), `handleArgumentUpdatersPlayerAuto.*` (player-side handle-key resolution). |
| generator_trace.py | Generates `layers/trace/`: `traceLayerAuto.*` plus `enumToStrAuto.*`/`printBitmasksAuto.*`/`printEnumsAuto.*`/`printUnionsAuto.*`/`printStructuresAuto.*`/`printPnextAuto.*` — human-readable printing for every enum/struct/union. |
| generator_api_debug.py | Generates `layers/api_debug/logVkErrorLayerAuto.*`, the layer that checks `VkResult` codes and logs errors. |
| generator_vk_layer.py | Generates the native Vulkan explicit-layer manifest `layer/VkLayer_vulkan_GITS_recorder.json`. |
| generator_interceptor.py | Generates `interceptor/interceptorAuto.cpp`, the interceptor DLL's exported entry points. |
| generator_subcapture.py | Generates `subcapture/`: `recordingLayerAuto.*` (records in-range commands) and `analyzerLayerAuto.*` (feeds the subcapture dependency analysis). |
| command_ids.py / command_ids.json | Assigns each command a **permanent** numeric ID, appending new commands to the persisted JSON cache and never renumbering existing ones — required so regenerating code never invalidates already-recorded streams. |
| plugin_generator.py | Discovers `<plugin>/codegen/generator.py` under `plugins/Vulkan/*`, letting out-of-tree plugins reuse the already-parsed IR to generate their own code. |

CMake wires this in as a real build-time target (`Vulkan_codegen`, see `codegen/CMakeLists.txt`), depended on by every compiled target that consumes `Auto` files. What each template actually generates is described in the section for the directory it's generated into, rather than here.

## Shared runtime (`common/`)

| Subdirectory | Contents |
|--------------|----------|
| `common/layer_interface/` | Core layer-chaining abstractions: `Command` base type, `Layer`/`LayerGroup`, dispatch tables (see [The "layer" abstraction](#the-layer-abstraction) above). |
| `common/coders/` | Argument/command byte-encoding pipeline: `argumentCoders`/`Custom`, `commandCoders`/`Custom`, `commandSerializers`/`Custom`/`Factory`. |
| `common/services/` | `HandleMapService` — bidirectional Vulkan handle ⟷ `GITSKey` map, with a lenient lookup path and use-after-destroy diagnostics. |
| `common/utils/` | `PluginService` — loads external `plugin.dll` modules implementing `IPlugin` from a `Plugins/Vulkan/` directory next to the executable. |

`arguments.h` defines the wrapper templates used as `Command` members (`Argument<T>`, `PointerArgument<T>`, `HandleArgument<T>`, `BufferArgument`, `DescriptorTemplateDataArgument`, ...). Most of the actual byte-level (de)serialization is generated: `argumentCodersAuto.*` emits per-struct `GetSize`/`Encode`/`Decode` (plus pointer/array overloads and `pNext`-chain walkers) for every Vulkan struct, and `commandCodersAuto.*` builds the equivalent per-command framing on top of them. `argumentCoders.{h,cpp}` provides hand-written primitives shared by generated and custom coders (flat-array `GetSizeT`/`EncodeT`, `pNext`-chain walkers, string codecs); `argumentCodersCustom.{h,cpp}` hand-codes structs a generic byte-copy can't handle, such as `VkWriteDescriptorSet` (a union selected by `descriptorType`) and `VkImageCreateInfo` (conditional `pQueueFamilyIndices`). Embedded *handles* bypass the byte blob entirely and instead flow through `Command::HandleKeys`/`CollectHandleKeys`/`ResolveHandleKeys` so they can be remapped independently of the raw argument bytes. Generated `commandSerializersAuto.h` emits a `<Name>Serializer : stream::CommandSerializer` per command; `commandSerializersFactory.h` declares `CreateCommandSerializer(Command*)`, implemented by generated `commandSerializersFactoryAuto.cpp` as a switch on `CommandId` that builds the right `<Name>Serializer` — hand-written (`commandSerializersCustom.h`) for meta-commands, generated for every real `vk*` command.

## Interception mechanisms (`interceptor/`, `layer/`)

| Directory | Contents |
|-----------|----------|
| `interceptor/` | DLL-shim mechanism. `interceptorAuto.cpp` exports `vkCreateInstance`/`vkGetInstanceProcAddr`/etc. mimicking the real loader — `Initialize()` bootstraps the real driver, `vkCreateInstance`/`vkCreateDevice`/`vkGet*ProcAddr` are special-cased, and every other command is a trampoline into the matching wrapper; generated `vulkanPrePostAuto.cpp` provides those per-command wrapped functions. Builds `Vulkan_interceptor` (renamed `vulkan-1` on MSVC, so it's picked up in place of the real loader). |
| `layer/` | Native Khronos explicit-layer mechanism. VkLayer_vulkan_GITS_recorder.cpp (hand-written) implements the standard layer trampoline pattern (`vkNegotiateLoaderLayerInterfaceVersion`, walking `VK_LAYER_LINK_INFO` chains); the generated `.json` manifest declares `VK_LAYER_INTEL_vulkan_GITS_recorder` and its binary path so the Vulkan loader can discover and load it. |

GITS supports two independent, interchangeable entry points into the Vulkan call stream. The **interceptor** impersonates the loader itself (`vulkan-1.dll` on Windows), opening the real driver library and resolving the genuine `vkGetInstanceProcAddr` before wrapping it. The **native layer** instead registers as a proper Khronos explicit layer, useful when DLL replacement isn't feasible or when coexisting with other layers (e.g. validation). Both converge on the same dispatch point, `IRecorderWrapper::GetFunctionWrapper(name)`, populated by the generated `vulkanPrePostAuto.cpp` — so the two mechanisms are just different "front doors" onto identical recording/layering logic.

## Optional layers (`layers/`)

| Directory | Contents |
|-----------|----------|
| `layers/api_debug/` | Fully generated `LogVkErrorLayer` (`logVkErrorLayerAuto.*`) — for every `VkResult`-returning command, logs an error if the call failed (and, in player mode, if the result diverges from the recorded one). |
| `layers/resource_dumping/` | `ResourceDumpingLayerGroup` conditionally loads `ScreenshotsLayer`, which hooks swapchain/queue creation and `vkQueuePresentKHR` to trigger GPU copies of the presented image (via `SwapchainImagesDumper`) and asynchronous PNG writes to disk (via `stb`). |
| `layers/trace/` | Hand-written apicall trace-line assembly (`CommandPrinter`, `printCustom`, `printStructuresCustom`, `traceLayerCustom`, `traceLayerGroup`) plus the generated per-type printers and `TraceLayer`. |

Each directory is grouped by a `LayerGroup` subclass whose `loadLayers()` conditionally calls `addLayer()` based on `Configurator` settings (e.g. `ResourceDumpingLayerGroup` only instantiates `ScreenshotsLayer` if screenshots are enabled). Active layers then receive `Pre`/`Post` calls for every matching command type. The trace layer assembles one readable log line per apicall (timestamp, command key, thread id, object key, function name, streamed arguments, return value, frame/draw counters), rendering object handles as stable `O<key>` identifiers so logs are comparable across capture and replay; it also subscribes to the global `MessageBus` so other log messages interleave into the same trace file.

## Recorder (`recorder/`)

| File/Class | Purpose |
|------------|---------|
| `vulkanRecorderInterface.h` — `IRecorderWrapper` | Abstract interface + exported `GITSRecorderVulkan2()` entry point, loaded by the interceptor/native layer to obtain the recorder singleton. |
| `vulkanRecorder.{h,cpp}` — `RecorderWrapper` | Thin forwarder to `CaptureManager`; hosts the lazily-constructed global instance (with `atexit` teardown on Linux). |
| `captureManager.{h,cpp}` — `CaptureManager` | Process-wide singleton owning the `stream::OrderingRecorder`, dispatch tables, the three tracking services below, unique key generators, and a `RecursionGuard` preventing re-entrant driver calls from being double-recorded. |
| `captureLayerManager.{h,cpp}` — `CaptureLayerManager` | Builds the ordered Pre/Post layer chains (customization, log-error, trace, screenshot, plugins), with `EncoderLayer` always last in Post. |
| `captureCustomizationLayer.{h,cpp}` — `CaptureCustomizationLayer` | Recorder-side `Layer` hooking surface creation, memory allocation/mapping, queue submit, and descriptor-template calls to feed the tracking services and emit extra meta-commands. |
| `wrappersAuto.{h,cpp}` | Per-command `<Name>Wrapper` function: builds the `Command`, virtualizes handles, runs Pre-layers, calls the real driver, registers output handles, runs Post-layers; also declares `g_FunctionWrappers`, a name→function-pointer map the interceptor uses to resolve which wrapper to call. |
| `encoderLayerAuto.{h,cpp}` | `EncoderLayer : Layer` — one `Post()` per command, serializing the just-executed call into the stream (with special handling to inject a `FrameEnd` after `vkQueuePresentKHR`); the layer `captureLayerManager` always runs last in Post. |
| `handleArgumentUpdaters.h` / `handleArgumentUpdatersAuto.{h,cpp}` / `handleArgumentUpdatersCustom.{h,cpp}` | Per-struct `CollectHandleKeys`/`UpdateHandle` translating live handles ⟷ `GITSKey`s via `HandleMapService`; Custom variants handle nested handles inside `VkWriteDescriptorSet`/`VkPushDescriptorSetInfo`/device-group arrays that the generated code can't handle generically. |
| `descriptorUpdateTemplateService.{h,cpp}` | Caches `VkDescriptorUpdateTemplateEntry` layouts so the opaque `pData` blob passed to `vkUpdateDescriptorSetWithTemplate`/push variants can be serialized correctly. |
| `mapTrackingService.{h,cpp}` | Tracks host-visible memory allocations/mappings and dirty pages, so raw CPU writes into mapped GPU memory are captured even though they aren't Vulkan apicalls. |
| `windowTrackingService.{h,cpp}` | Tracks surface↔window↔swapchain relationships and live window geometry/visibility across presents. |

A live Vulkan call resolves (via `GetFunctionWrapper`/`GITSRecorderVulkan2`) to a generated wrapper in `wrappersAuto.cpp`, which snapshots arguments into a `<vkFunc>Command`, guards against recursive re-entry, translates input handles to `GITSKey`s, runs Pre-layers, calls the real driver through the dispatch table (unless skipped), mints keys for new output handles, then runs Post-layers in a fixed order (log-error, customization, trace, screenshot, plugins, with `EncoderLayer` — which actually serializes the command into the stream — deliberately last). `CaptureManager` is a lazily-constructed singleton whose lifetime is the recording session; whether recording actually happens is gated by `Configurator`'s `common.recorder.enabled` flag, so trace/screenshot/plugin layers can run independently of stream recording. The three tracking services capture state changes that have no discrete Vulkan apicall behind them (mapped-memory writes, window geometry, descriptor-template layouts).

## Player (`player/`)

| File/Class | Purpose |
|------------|---------|
| `playerManager.{h,cpp}` — `PlayerManager` | Top-level singleton: loads the Vulkan driver and dispatch tables, owns `PlayerLayerManager` and all `*Service` objects, gates whether commands actually execute. |
| `playerLayerManager.{h,cpp}` — `PlayerLayerManager` | Builds the ordered Pre/Post layer chains (customization, error-log, trace, screenshots, subcapture/analyzer/recording, plugins). |
| `replayCustomizationLayer.{h,cpp}` — `ReplayCustomizationLayer` | Player-only layer: rebuilds dispatch tables on instance/device creation, remaps window/surface handles, handles offscreen swapchain acquire/present, and inserts catch-up waits when replay-time fence/event/semaphore/query state lags the recorded state. |
| `vulkanCommandFactory.h` / `vulkanCommandFactoryAuto.cpp` | Turns a stream `CommandId` into a `stream::CommandRunner*`. |
| `commandRunnersAuto.{h,cpp}` | Per-command `<Name>Runner : CommandRunner` — decodes itself, resolves handle keys, runs Pre-layers, calls the real driver, registers output handles, runs Post-layers. |
| `commandRunnersCustom.{h,cpp}` | Hand-written runners for meta-commands (state-restore begin/end, frame-end, markers, window create/update, mapped-data writes). |
| `handleArgumentUpdaters.h` / `handleArgumentUpdatersPlayerAuto.{h,cpp}` / `handleArgumentUpdatersCustom.{h,cpp}` | Handwritten template functions plus generated concrete `UpdateHandle`/`UpdateOutputHandle` overloads per struct, translating handle arguments via `HandleMapService`; Custom handles opaque/conditional structures and multi-handle enumeration outputs. |
| `descriptorUpdateTemplateService.{h,cpp}` | Player-side counterpart storing template entry layouts so handles inside `pData` blobs can be remapped. |
| `fencePendingSignalService.{h,cpp}` | Tracks which fences have a pending, unobserved signal, so catch-up waits never block on a fence that will never signal within the replayed range. |
| `mapTrackingService.{h,cpp}` | Tracks per device+memory live host pointer/size while mapped, so recorded mapped-memory writes can be replayed against the current pointer. |
| `swapchainImageSyncService.{h,cpp}` | Implements `vkAcquireNextImageKHR`/`2KHR` replay: fake-submits in offscreen mode, or rewinds present/re-acquire in normal mode to match the recorded image index. |
| `windowService.{h,cpp}` | Creates/manages the real OS window per platform and maps recording-time window/instance handles to replay-time ones. |

The generic stream reader feeds each recorded command's id to `VulkanCommandFactory::CreateCommand`, returning a generated `<Name>Runner` (or a hand-written meta-runner). Each runner decodes its arguments, resolves handles to live values, runs Pre-layers, invokes the real driver through the correct dispatch table, registers new output handles, then runs Post-layers — mirroring the recorder-side flow. `PlayerLayerManager` even instantiates some of the same classes used on the recording side (`RecordingLayer`, `AnalyzerLayer`, `SubcaptureLayer`) to support re-recording a sub-range of a replayed stream. Several `*Service` classes exist because Vulkan exposes no query for certain async/timing-dependent state that replay still needs to reason about (pending fence signals, live host pointers for mapped memory, swapchain acquire/present ordering) — the [subcapture](#subcapture-subcapture) services solve the analogous problem on the recording side.

## Subcapture (`subcapture/`)

| File/Class | Purpose |
|------------|---------|
| `subcaptureLayer.{h,cpp}` — `SubcaptureLayer` | Top-level layer hooked into nearly every command's `Post()`; owns `SubcaptureRange`, `SubcaptureRecorder`, `StateTrackingService`, and (depending on mode) `AnalyzerService` or `AnalyzerResults`; triggers `StateTrackingService::RestoreState()` at the configured restore point. |
| `subcaptureRange.{h,cpp}` — `SubcaptureRange` | Parses the configured frame range (`"5"`, `"3-6"`); `IsRestorePoint()`/`InRange()` gate analysis/recording. |
| `subcaptureRecorder.{h,cpp}` — `SubcaptureRecorder` | Thin `stream::StreamWriter` wrapper for the trimmed output stream; stays closed (no-op) during the analysis pass. |
| `analyzerLayerAuto.{h,cpp}` | `AnalyzerLayer : Layer` — marks every handle touched by a command as "used" via `AnalyzerService::NotifyObject(s)`, feeding the analysis pass below. |
| `analyzerService.{h,cpp}` — `AnalyzerService` | Analysis-pass only: collects referenced object keys, expands the full dependency closure, and writes a YAML analysis file (with a completion marker written last, for crash safety). |
| `analyzerResults.{h,cpp}` — `AnalyzerResults` | Recording-pass only: loads a complete analysis file if present; `RestoreObject(key)` gates whether an object is restored (falls back to "restore everything" if optimization is off or no valid file exists). |
| `recordingLayerCustom.cpp` + `recordingLayerAuto.{h,cpp}` — `RecordingLayer` | Records in-range commands into the trimmed stream. The generated part supplies a per-command `Post()` recording hook for every command, and — outside the capture range — it buffers `vkCmd*` calls per command buffer for later use in the state-restore sequence. The custom part emits `FrameEndCommand` markers and finalizes the stream once the range ends. |
| `stateTrackingService.{h,cpp}` — `StateTrackingService` | Central `ObjectState` store and the restore-emission engine; `RestoreState()` emits state-setup commands in three ordered sweeps (objects → pipelines → command buffers). |
| `objectState.h` | Base `ObjectState` (`Key`, `ParentKey`, `DependencyKeys`, creation command info) plus per-type states (`DeviceMemoryState`, `ImageState`, `BufferState`, `SemaphoreState`, `FenceState`, ...) holding mutable runtime state absent from the creation command. |
| `commandBufferLifecycleService.{h,cpp}` | Tracks command-buffer allocate/begin/end/reset lifecycle, since a CB's replayable content is whatever its last unreset recording contains. |
| `descriptorSetUpdateService.{h,cpp}` | Tracks per-binding "last write wins" descriptor content, since descriptor sets are mutable bind-only objects with no single creation snapshot. |
| `imageLayoutService.{h,cpp}` | Tracks each image's current `VkImageLayout` (buffered per-CB, applied at submit), since Vulkan requires explicit layout transitions with no implicit tracking. |
| `mappedMemoryService.{h,cpp}` | Tracks map/unmap state and a shadow buffer of host-visible memory contents, since raw writes into mapped pointers aren't apicalls. |
| `queryPoolStateService.{h,cpp}` | Tracks per-query reset/used state so a first read after restore doesn't hit a device-lost error. |
| `syncStateService.{h,cpp}` | Tracks fence/semaphore signaled state across submit/present/acquire, so the first wait after restore doesn't hang. |
| `gpuReadbackHelper.{h,cpp}` — `IGpuReadbackHelper`/`GpuReadbackHelper` | Reads current GPU-resident (`DEVICE_LOCAL`) buffer/image contents back to a staging buffer so they can be embedded in the trimmed stream as upload commands; injected as an interface from the player module to avoid a circular library dependency. |

Subcapture is a two-phase, optionally two-pass design. An optional **analysis pass** (only run when optimization is enabled and no valid analysis file exists) constructs `SubcaptureLayer` with the recorder closed, swapping in `AnalyzerService`; per-command hooks (`analyzerLayerAuto`) notify it of every object referenced in-range, and at range end it writes out the full dependency closure as YAML. The always-run **recording pass** loads that analysis (via `AnalyzerResults`) into `StateTrackingService`, and at the configured restore point emits the minimal (or full) set of creation/bind/content-upload commands needed to reconstruct live Vulkan state — gated per-object by the analysis results — before letting `RecordingLayer` record the in-range commands into the trimmed stream. Each `*Service` isolates one piece of Vulkan state that can't be reconstructed from an object's creation call alone; `GpuReadbackHelper` fills the remaining gap for `DEVICE_LOCAL` resource contents that never pass through host-visible memory.

## Notes

- **Generated files** → any file ending in `Auto`; edit the mako template in `Vulkan/codegen/templates/` (or the generator script) and regenerate, never the `Auto` file itself.
- **`command_ids.json`** → assigns permanent numeric IDs to recorded commands; never renumber an existing entry, only append (see the codegen table above).
- **Legacy backend** → `VulkanLegacy/` implements the older `gits::Vulkan` backend, not this one.
- **Conventions and style** → [project guide](project.md)
- **General repository layout** → [codebase-map.md](codebase-map.md)
