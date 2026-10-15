# Guide for AI Agents - Modern OpenGL Backend



## Naming

`OpenGL2/` is a temporary development path. The backend is expected to eventually become `OpenGL/`
(only after user decides so). Do not put `OpenGL2` into new code comments, log messages, or
long-lived architecture names.

## Stream compatibility

This backend is still WIP and there is no promise of stream compatibility yet. We are free to alter
the on-disk format. For now, adding any backwards compatibility logic makes no sense.

## Priorities

Make applications work with GITS recording and replay. Preserving the application's exact OpenGL
state and synchronization behavior is nice to have, not a requirement. When preserving it would
substantially complicate the code and a small change still lets the application run and replay
correctly, prefer simpler implementation over meticulously preserving the state.

## Behavioral contracts

- Preserve lazy per-function resolution: real DLL export first, then the real platform proc-address
  function. Cache valid pointers at dynamic lookup time while the bootstrap context is still valid.
- `Command.enabled` is the only support gate. All runtime commands must still forward; only enabled,
  fully tested commands receive typed capture, serialization, and player artifacts.
- Disabled commands pass through unchanged and warn once. Do not claim replay fidelity for omitted
  calls.
- The interceptor owns exports, bootstrap, and proc substitution. The recorder owns wrappers,
  layers, real-driver dispatch, configuration, and streams.
- Thread-local recursion depth suppresses nested capture; it is not synchronization. Protect shared
  services locally.
- The encoder is always the final Post layer.
- Use ordinary `TODO:` comments for concrete temporary or deferred code. Include the action and its
  trigger; do not use TODOs as a second command-support list.

When a change revises an accepted architectural decision, update the corresponding rationale in the
architecture page in the same change.